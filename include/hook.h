#pragma once

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <detours.h>
#include <utils.h>
#include "obfuscate.h"

#define HOOK_ATTACH(realFnPtr, detourFn)  DetourAttach(reinterpret_cast<PVOID*>(realFnPtr), reinterpret_cast<PVOID>(detourFn))
#define HOOK_DETACH(realFnPtr, detourFn)  DetourDetach(reinterpret_cast<PVOID*>(realFnPtr), reinterpret_cast<PVOID>(detourFn))


// API export ismiyle hook (ör. MessageBoxW gibi)
#define DECL_HOOK_API(ret, callcv, name, ...)            \
    extern ret (callcv * Real_##name)(__VA_ARGS__);       \
    ret callcv Mine_##name(__VA_ARGS__);


#define DECL_HOOK_ADDR(ret, callcv, name, addr, ...)      \
    using name##_t = ret (callcv*)(__VA_ARGS__);          \
    extern name##_t Real_##name;                          \
    ret callcv Mine_##name(__VA_ARGS__);

// Başlat/bitir
void StartHooks();
void StopHooks();


DECL_HOOK_API(int, WINAPI, MessageBoxW, HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)



