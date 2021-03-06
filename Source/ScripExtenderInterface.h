#pragma once
#include "stdafx.h"

#define SEAPI(retType) extern "C" __declspec(dllexport) retType __cdecl

struct SKSEInterface;
struct PluginInfo;
SEAPI(bool) SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info);
SEAPI(bool) SKSEPlugin_Load(const SKSEInterface* skse);
