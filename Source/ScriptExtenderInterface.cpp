#include "stdafx.h"
#include "ScriptExtenderInterface.h"
#include "PrivateProfileRedirector.h"
#include <strsafe.h>

#pragma warning(disable: 4005) // macro redefinition
#pragma warning(disable: 4244) // conversion from 'x' to 'y', possible loss of data
#pragma warning(disable: 4267) // ~

#include <common/IPrefix.h>
#if defined _WIN64
#include <skse64_common/skse_version.h>
#include <skse64_common/Relocation.h>
#include <skse64_common/SafeWrite.h>
#include <skse64/ScaleformCallbacks.h>
#include <skse64/ScaleformMovie.h>
#include <skse64/PluginAPI.h>
#include <skse64/GameAPI.h>
#include <skse64/ObScript.h>

#pragma comment(lib, "skse64/x64/Release/skse64_1_5_39.lib")
#pragma comment(lib, "skse64/x64/Release/skse64_common.lib")
#pragma comment(lib, "skse64/x64/Release/skse64_loader_common.lib")
#pragma comment(lib, "skse64/x64_v141/Release/common_vc14.lib")
#else
#include <skse/skse_version.h>
#include <skse/SafeWrite.h>
#include <skse/ScaleformCallbacks.h>
#include <skse/ScaleformMovie.h>
#include <skse/PluginAPI.h>
#include <skse/GameAPI.h>
#include <skse/Hooks_ObScript.h>
#include <skse/CommandTable.h>
#include <skse/GameMenus.h>

#pragma comment(lib, "skse/Release/skse.lib")
#pragma comment(lib, "skse/Release/loader_common.lib")
#pragma comment(lib, "common/Release VC9/common_vc9.lib")
#endif

#undef SEAPI
#define SEAPI(retType) retType __cdecl

#define SELOG(format, ...) 	\
PrivateProfileRedirector::GetInstance().Log(L##format, __VA_ARGS__);	\
_MESSAGE(format, __VA_ARGS__)	\

//////////////////////////////////////////////////////////////////////////
SEAPI(bool) SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info)
{
	// Set info
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = PrivateProfileRedirector::GetLibraryNameA();
	info->version = PrivateProfileRedirector::GetLibraryVersionInt();

	// Save handle
	PluginHandle pluginHandle = skse->GetPluginHandle();

	// Check SKSE version
	if (skse->skseVersion != PACKED_SKSE_VERSION)
	{
		SELOG("[SKSE] This plugin is not compatible with this version of SKSE");
		return false;
	}

	// Get the scale form interface and query its version.
	// Don't return, I don't use scaleform anyway.
	SKSEScaleformInterface* scaleform = static_cast<SKSEScaleformInterface*>(skse->QueryInterface(kInterface_Scaleform));
	if (!scaleform)
	{
		SELOG("[SKSE] Couldn't get scaleform interface");
		//return false;
	}
	if (scaleform->interfaceVersion < SKSEScaleformInterface::kInterfaceVersion)
	{
		SELOG("[SKSE] Scaleform interface too old (%d, expected %d)", (int)scaleform->interfaceVersion, (int)SKSEScaleformInterface::kInterfaceVersion);
		//return false;
	}
	return RedirectorSEInterface::GetInstance().OnQuery(pluginHandle, scaleform);
}
SEAPI(bool) SKSEPlugin_Load(const SKSEInterface* skse)
{
	RedirectorSEInterface& instance = RedirectorSEInterface::GetInstance();
	if (instance.OnLoad())
	{
		PrivateProfileRedirector::GetInstance().Log(L"[SKSE] %s %s loaded", PrivateProfileRedirector::GetLibraryNameW(), PrivateProfileRedirector::GetLibraryVersionW());
		_MESSAGE("[SKSE] %s %s loaded", PrivateProfileRedirector::GetLibraryNameA(), PrivateProfileRedirector::GetLibraryVersionA());
		
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
RedirectorSEInterface& RedirectorSEInterface::GetInstance()
{
	static RedirectorSEInterface ms_Instance;
	return ms_Instance;
}

bool RedirectorSEInterface::RegisterScaleform(GFxMovieView* view, GFxValue* root)
{
	return true;
}

bool RedirectorSEInterface::OnQuery(PluginHandle pluginHandle, SKSEScaleformInterface* scaleforem)
{
	m_PluginHandle = pluginHandle;
	m_Scaleform = scaleforem;

	return true;
}
bool RedirectorSEInterface::OnLoad()
{
	OverrideRefreshINI();
	return true;
}

RedirectorSEInterface::ConsoleCommandInfo* RedirectorSEInterface::FindScriptCommand(const std::string_view& fullName) const
{
	#if defined _WIN64
	for (ConsoleCommandInfo* command = g_firstConsoleCommand; command->opcode < kObScript_NumConsoleCommands + kObScript_ConsoleOpBase; ++command)
	{
		if (command->longName && std::string_view(command->longName) == fullName)
		{
			return command;
		}
	}
	#endif
	return NULL;
}
void RedirectorSEInterface::OverrideRefreshINI()
{
	#if defined _WIN64
	SELOG("[SKSE] Overriding 'RefreshINI' console command to refresh INIs from disk");

	m_RefreshINICommand = FindScriptCommand("RefreshINI");
	if (m_RefreshINICommand)
	{
		m_OriginalRefreshINIHandler = m_RefreshINICommand->execute;
		auto OnCall = [](void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffsetPtr) -> bool
		{
			RedirectorSEInterface& instance = RedirectorSEInterface::GetInstance();
			Console_Print("Executing 'RefreshINI'");

			size_t reloadedCount = PrivateProfileRedirector::GetInstance().RefreshINI();
			bool ret = instance.m_OriginalRefreshINIHandler(paramInfo, scriptData, thisObj, containingObj, scriptObj, locals, result, opcodeOffsetPtr);

			Console_Print("Executing 'RefreshINI' done with result '%d', %zu files reloaded.", (int)ret, reloadedCount);
			return ret;
		};

		ObScriptCommand newCommand = *m_RefreshINICommand;
		newCommand.longName = "RefreshINI";
		newCommand.shortName = "RefreshINI";
		newCommand.helpText = "[PrivateProfileRedirector] Reloads INIs content from disk and calls original 'RefreshINI' after it";
		newCommand.needsParent = 0;
		newCommand.numParams = 0;
		newCommand.params = NULL;
		newCommand.execute = OnCall;
		newCommand.eval = NULL;
		newCommand.flags = 0;

		SafeWriteBuf(reinterpret_cast<uintptr_t>(m_RefreshINICommand), &newCommand, sizeof(newCommand));
		SELOG("[SKSE] Command 'RefreshINI' is overridden");
	}
	else
	{
		SELOG("[SKSE] Can't find 'RefreshINI' command to override");
	}
	#endif
}

RedirectorSEInterface::RedirectorSEInterface()
	:m_PluginHandle(kPluginHandle_Invalid)
{
}
RedirectorSEInterface::~RedirectorSEInterface()
{
}
