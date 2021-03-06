#pragma once
#include "stdafx.h"

#define SEAPI(retType) extern "C" __declspec(dllexport) retType __cdecl

struct PluginInfo;
struct SKSEInterface;
struct SKSEScaleformInterface;
class TESObjectREFR;
class GFxMovieView;
class GFxValue;

#if defined _WIN64
struct ObScriptCommand;
#else
struct CommandInfo;
#endif

SEAPI(bool) SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info);
SEAPI(bool) SKSEPlugin_Load(const SKSEInterface* skse);

//////////////////////////////////////////////////////////////////////////
class RedirectorSEInterface
{
	using PluginHandle = uint32_t;
	using ConsoleCommandHandler = bool(*)(void*, void*, TESObjectREFR*, void*, void*, void*, double*, void*);
	#if defined _WIN64
	using ConsoleCommandInfo = ObScriptCommand;
	#else
	using ConsoleCommandInfo = CommandInfo;
	#endif

	friend bool SKSEPlugin_Query(const SKSEInterface*, PluginInfo*);
	friend bool SKSEPlugin_Load(const SKSEInterface*);

	public:
		static RedirectorSEInterface& GetInstance();
		
		static bool RegisterScaleform(GFxMovieView* view, GFxValue* root);
		template<class T> static void RegisterScaleformFunction(GFxMovieView* view, GFxValue* root, const char* name)
		{
			#if defined _WIN64
			::RegisterScaleformFunction<T>(root, view, name);
			#else
			::RegisterFunction<T>(root, view, name);
			#endif
		}
		template<class T> static void RegisterScaleformFunction(GFxMovieView* view, GFxValue* root, const std::string& name)
		{
			#if defined _WIN64
			::RegisterScaleformFunction<T>(root, view, name.c_str());
			#else
			::RegisterFunction<T>(root, view, name);
			#endif
		}

	public:
		PluginHandle m_PluginHandle;
		SKSEScaleformInterface* m_Scaleform = NULL;

		ConsoleCommandInfo* m_RefreshINICommand = NULL;
		ConsoleCommandHandler m_OriginalRefreshINIHandler = NULL;

	private:
		bool OnQuery(PluginHandle pluginHandle, SKSEScaleformInterface* scaleforem);
		bool OnLoad();

		ConsoleCommandInfo* FindScriptCommand(const std::string_view& fullName) const;
		void OverrideRefreshINI();

	private:
		RedirectorSEInterface();
		~RedirectorSEInterface();

	public:
		PluginHandle GetPluginHandle() const
		{
			return m_PluginHandle;
		}
		
		bool HasScaleform() const
		{
			return m_Scaleform != NULL;
		}
		SKSEScaleformInterface* GetScaleform() const
		{
			return m_Scaleform;
		}
};
