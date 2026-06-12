#include "hook.h"



void Itemmall();
void NETWORK();
void Buff_Icons();
void Slots();
void Load();
void Resolutions();



int (WINAPI* Real_MessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT) = ::MessageBoxW;

int WINAPI Mine_MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    // Buraya kendi mantığını ekleyebilirsin.
    // Örnek: Başlığa [Hooked] eki
    return Real_MessageBoxW(hWnd, lpText, L"[Hooked] " L"DetoursHookProject", uType);
}



void StartHooks()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // WinAPI örnek hook
    HOOK_ATTACH(&Real_MessageBoxW, Mine_MessageBoxW);



    DetourTransactionCommit();
    // Oyun içi inline patchler
    Itemmall();
    //NETWORK();
    Buff_Icons();
    Slots();
    Load();
    Resolutions();
}

void StopHooks()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // WinAPI örnek hook
    HOOK_DETACH(&Real_MessageBoxW, Mine_MessageBoxW);



    DetourTransactionCommit();
}
