#include <hook.h>
#include <string>
#include <cstdlib>
#include <climits>


// === Global durum ===
static volatile int LocationX = 0;
static volatile int LocationY = 0;
static volatile int OldX = 0;
static volatile int OldY = 0;

// Opsiyonel debug
static volatile int MouseX = 0;
static volatile int MouseY = 0;

static const char* INI_PATH = ".\\CONFIG.ini";
static const char* SEC = "BUFF";

static __forceinline int ReadMouseX() { return *reinterpret_cast<volatile int*>(0x7C3C0C); }
static __forceinline int ReadMouseY() { return *reinterpret_cast<volatile int*>(0x7C3C10); }

// === FIX: Debuff render sırasında small ikonları zorlamak için flag ===
static volatile BYTE g_ForceSmallIcons = 0;

// CONFIG.ini > [BUFF]
// SMALL_ICONS=1          -> 32x32 yerine 17x17 ikon yolu
// TRANSPARENT_ICONS=1    -> arkadaşın hook'unun düzeltilmiş hali: 0x80FFFFFF alpha
static volatile BYTE small_buff_toggle = 1;
static volatile BYTE g_TransparentIcons = 1;
static volatile BYTE g_BuffRenderAlphaHookActive = 0;

// === INI ===
static void LoadFromIni()
{
    // Projedeki mevcut stil: .\CONFIG.ini / [BUFF] / uppercase key.
    // Eski Icons.cpp uyumluluğu için lowercase key'leri de fallback olarak okuyoruz.
    LocationX = GetPrivateProfileIntA(SEC, "LOCATION_X", INT_MIN, INI_PATH);
    if (LocationX == INT_MIN)
        LocationX = GetPrivateProfileIntA(SEC, "location_x", 0, INI_PATH);

    LocationY = GetPrivateProfileIntA(SEC, "LOCATION_Y", INT_MIN, INI_PATH);
    if (LocationY == INT_MIN)
        LocationY = GetPrivateProfileIntA(SEC, "location_y", 0, INI_PATH);

    small_buff_toggle = static_cast<BYTE>(GetPrivateProfileIntA(SEC, "SMALL_ICONS", 1, INI_PATH) ? 1 : 0);
    g_TransparentIcons = static_cast<BYTE>(GetPrivateProfileIntA(SEC, "TRANSPARENT_ICONS", 1, INI_PATH) ? 1 : 0);

    OldX = LocationX; OldY = LocationY;
}

static void SaveIfChanged()
{
    if (LocationX != OldX || LocationY != OldY)
    {
        char sx[16], sy[16];
        _itoa_s(LocationX, sx, 10);
        _itoa_s(LocationY, sy, 10);
        WritePrivateProfileStringA(SEC, "LOCATION_X", sx, INI_PATH);
        WritePrivateProfileStringA(SEC, "LOCATION_Y", sy, INI_PATH);
        OldX = LocationX; OldY = LocationY;
    }
}

BYTE Adjust;
BYTE Adjust_1;
std::string debuff[255];

DWORD Pos1 = 0;
DWORD Pos2 = 0;
DWORD Pos3 = 0;
DWORD Pos4 = 0;

int getValuePos() {
    char buffer[255] = {};
    char buffer2[255] = {};
    GetPrivateProfileStringA(SEC, "LOCATION_X", "", buffer, sizeof(buffer), INI_PATH);
    GetPrivateProfileStringA(SEC, "LOCATION_Y", "", buffer2, sizeof(buffer2), INI_PATH);
    if (buffer[0] == '\0')
        GetPrivateProfileStringA(SEC, "location_x", "", buffer, sizeof(buffer), INI_PATH);
    if (buffer2[0] == '\0')
        GetPrivateProfileStringA(SEC, "location_y", "", buffer2, sizeof(buffer2), INI_PATH);
    Pos1 = atoi(buffer);
    Pos2 = atoi(buffer2);
    return 0;
}

// stdcall thunk (getValuePos'u değiştirmeden thread'e uygun hale getirir)
static DWORD WINAPI getValuePosThread(LPVOID) { getValuePos(); return 0; }

// === MousePos kancası (aynen) ===
DWORD Mouse_Pos_Return = 0x00488832;
__declspec(naked) void MousePos() {
    __asm {
        cmp byte ptr ds : [Adjust] , 00
        je originalcode
        cmp byte ptr ds : [Adjust_1] , 01
        je originalcode

        push eax
        mov eax, dword ptr ds : [0x005646CF]
        mov ecx, [eax]
        mov[Pos1], ecx
        mov eax, dword ptr ds : [0x005646A5]
        mov ecx, [eax]
        mov[Pos2], ecx
        pop eax

        originalcode :
        fild dword ptr ds : [0x7C3C0C]
            jmp Mouse_Pos_Return
    }
}

// === debuff blokları (aynen) ===
std::string pos_x_stack[128];
std::string buff_stack[128];
std::string isPos[128];
std::string stack[128];
DWORD checkDebuffSkill = 0x0046CDA0;
DWORD isBuffRender = 0x004B57B0;
DWORD isLoadTime = 0x004B6BD0;
DWORD loadWhite = 0x004B6180;
DWORD loadDebuffIcon = 0x0057B860;
DWORD isBuffReturn = 0x004D740A;

__declspec(naked) void separateDebuffMain() {
    __asm {
        pushad
        xor esi, esi
        mov eax, dword ptr ds : [0x007AB0D8]
        cdq
        sub eax, edx
        mov edi, eax
        mov eax, dword ptr ds : [0x022AA6F8]
        mov ecx, [eax]
        sar edi, 1
        cmp byte ptr ds : [Adjust] , 00
        je isDebuff01

        movzx edi, word ptr ds : [Pos1]
        add edi, -0xA
        movzx ebx, word ptr ds : [Pos2]
        add ebx, -0xA
        jmp return_mouse_pos_1

        isDebuff01 :
        cmp dword ptr ds : [Pos1] , 0
            je continue_pos_edi
            cmp dword ptr ds : [Pos2] , 0
            je continue_pos_edi
            movzx edi, word ptr ds : [Pos1]
            add edi, -0xA
            movzx ebx, word ptr ds : [Pos2]
            add ebx, -0xA
            jmp return_mouse_pos_1

            continue_pos_edi :
        sub edi, 0x00000082

            return_mouse_pos_1 :
            mov ebx, 0x0000008A
            mov dword ptr ds : [stack] , esi
            mov edx, dword ptr ds : buff_stack
            sub ebx, edx
            add edx, ebx
            cmp ebx, edx
            je continue_pos
            mov ebx, dword ptr ds : buff_stack
            add ebx, 0x55

            continue_pos :
            mov dword ptr ds : [isPos] , ebx
            xor esi, esi
            mov[esp + 0x3C], esi
            mov dword ptr ds : [0x022AA700] , ecx
            cmp ecx, eax
            jne continue_stack
            popad
            jmp originalcode

            get_pos_y_stack :
        mov ebx, dword ptr ds : [isPos]

            continue_stack :
            mov esi, dword ptr ds : [ecx + 0x10]
            test esi, esi
            je check_buff_stack
            cmp dword ptr ds : [esi + 0x20] , 01
            je check_buff_stack
            cmp word ptr ds : [esi + 0x06] , 100
            je continue_to_stack
            cmp word ptr ds : [esi + 0x06] , 100
            jg is_buff_stack
            mov al, [esi + 0x06]
            movzx ecx, word ptr ds : [esi + 0x04]
            push eax
            push ecx
            mov ecx, 0x0091AD64
            call checkDebuffSkill
            mov ebp, eax
            test ebp, ebp
            je check_buff_stack_ebp
            cmp byte ptr ds : [ebp + 0x1F] , 03
            je continue_to_stack
            jmp check_buff_stack_ebp

            continue_to_stack :
        movzx eax, byte ptr ds : [esi + 0x06]
            movzx ecx, word ptr ds : [esi + 0x04]
            push 01
            push eax
            push ecx
            push ebx
            push edi
            push - 0x01
            mov ecx, ebp

            // === FIX: Debuff ayrık render akışında small'ı zorla ===
            mov byte ptr ds : [g_ForceSmallIcons] , 1
            call isBuffRender

            mov edx, dword ptr ds : [esi + 0x10]
            mov eax, dword ptr ds : [esi + 0x08]
            push edx
            push eax
            push ebx
            push edi
            push 0xFFFFFF00
            mov ecx, ebp
            call isLoadTime
            mov byte ptr ds : [g_ForceSmallIcons] , 0
            // === FIX end ===

            mov al, [esi + 0x06]
            xor ebx, ebx
            cmp al, 0x64
            jne isCheckDebuffSkill
            mov ebx, 0x00000001

            condition_menssage:
        mov eax, 0x10624DD3
            mul dword ptr ds : [esi + 0x10]
            shr edx, 0x06

            condition_menssage_2 :
            cmp dword ptr ds : [0x007C3C0C] , edi
            jl isCondition
            lea ecx, [edi + 0x20]
            cmp dword ptr ds : [0x007C3C0C] , ecx
            jg isCondition
            mov eax, dword ptr ds : [isPos]
            cmp dword ptr ds : [0x007C3C10] , eax
            jl isCondition
            cmp byte ptr ds : [small_buff_toggle] , 01
            jne isSizeNormal
            add eax, 0x11
            jmp isSizeContinue

            isSizeNormal :
        add eax, 0x20

            isSizeContinue :
            cmp dword ptr ds : [0x007C3C10] , eax
            jg isCondition
            mov dword ptr ds : [ebp + 0x00] , 00000001
            mov ax, [esi + 0x04]
            mov[ebp + 0x14], ax
            mov cl, [esi + 0x06]
            mov[ebp + 0x16], cl
            mov dword ptr ds : [esp + 0x3C] , edx

            isCondition :
        test ebx, ebx
            je set_stack
            mov edx, dword ptr ds : [isPos]
            mov eax, dword ptr ds : [0x022AB854]
            cmp byte ptr ds : [small_buff_toggle] , 01
            jne isNormalSize
            push 0x11
            push 0x11
            jmp continue_size

            isNormalSize :
        push 0x20
            push 0x20

            continue_size :
            push edx
            push edi
            push eax
            call loadWhite
            add esp, 0x14
            mov eax, dword ptr ds : [isPos]
            sub eax, 0x0C
            push eax
            mov ecx, edi
            cmp byte ptr ds : [small_buff_toggle] , 01
            jne isDebuffSizeNormal
            add ecx, 9
            jmp continueDebuffSize

            isDebuffSizeNormal :
        add ecx, 0x13

            continueDebuffSize :
            push ecx
            lea ecx, dword ptr ds : debuff
            call loadDebuffIcon

            set_stack :
        mov eax, dword ptr ds : [stack]
            inc eax
            cmp byte ptr ds : [small_buff_toggle] , 01
            jne originalStack
            add edi, 0x25
            cmp eax, 0x8
            jmp continueStack_

            originalStack :
        add edi, 0x28
            cmp eax, 0x06

            continueStack_ :
            mov dword ptr ds : [stack] , eax
            jng is_buff_stack
            mov eax, dword ptr ds : [0x007AB0D8]
            cdq
            sub eax, edx
            mov edi, eax
            sar edi, 1
            cmp byte ptr ds : [Adjust] , 00
            je originalDebuff
            movzx edi, word ptr ds : [Pos1]
            add edi, -0xA
            jmp Mov_Pos_3_Return

            originalDebuff :
        cmp dword ptr ds : [Pos1] , 0
            je continue_pos_edi_2
            cmp dword ptr ds : [Pos2] , 0
            je continue_pos_edi_2
            movzx edi, word ptr ds : [Pos1]
            add edi, -0xA
            jmp Mov_Pos_3_Return

            continue_pos_edi_2 :
        sub edi, 0x00000082

            Mov_Pos_3_Return :
            add dword ptr ds : [isPos] , 0x28
            mov dword ptr ds : [stack] , 0x00000000

            is_buff_stack :
            mov ecx, dword ptr ds : [0x022AA700]

            check_buff_stack :
            cmp byte ptr ds : [ecx + 0x15] , 0x00
            jne label_end
            mov eax, [ecx + 0x08]
            cmp byte ptr ds : [eax + 0x15] , 0x00
            jne label_1
            mov ecx, eax
            mov eax, [ecx]
            cmp byte ptr ds : [eax + 0x15] , 00
            jne label_2
            nop

            back_ :
        mov ecx, eax
            mov eax, [ecx]
            cmp byte ptr ds : [eax + 0x15] , 00
            je back_
            jmp label_2

            label_1 :
        mov eax, [ecx + 0x04]
            cmp byte ptr ds : [eax + 0x15] , 00
            jne label_3

            label_4 :
        cmp ecx, [eax + 0x08]
            jne label_3
            mov ecx, eax
            mov dword ptr ds : [0x022AA700] , ecx
            mov eax, [eax + 0x04]
            cmp byte ptr ds : [eax + 0x15] , 00
            je label_4

            label_3 :
        mov ecx, eax

            label_2 :
        mov dword ptr ds : [0x022AA700] , ecx

            label_end :
        cmp ecx, dword ptr ds : [0x022AA6F8]
            jne get_pos_y_stack
            popad

            originalcode :
        mov dword ptr ds : [0x022AA700] , ecx
            jmp isBuffReturn

            check_buff_stack_ebp :
        mov ebp, dword ptr ds : [esp + 0x34]
            jmp is_buff_stack

            popad
            jmp originalcode

            continue_message :
        mov ebx, 0x01
            cmp byte ptr ds : [ebp + 0x93] , 01
            mov ebp, dword ptr ds : [esp + 0x34]
            jne condition_menssage
            mov edx, dword ptr ds : [esi + 0x10]
            jmp condition_menssage_2

            isCheckDebuffSkill :
        movzx ecx, word ptr ds : [esi + 0x04]
            push eax
            push ecx
            mov ecx, 0x0091AD64
            call checkDebuffSkill
            mov ebp, eax
            test ebp, ebp
            je check_buff_stack_ebp
            cmp byte ptr ds : [ebp + 0x1F] , 0x03
            je continue_message
            jmp check_buff_stack
    }
}

DWORD originalConditionAddr = 0x004D76A6;
DWORD isHideDebuffReturn = 0x004D7439;
DWORD ignoreDebuff = 0x004D76A0;

__declspec(naked) void isHideDebuff() {
    __asm {
        mov dword ptr ds : [buff_stack] , ebx
        pushad
        mov al, [esi + 0x06]
        cmp al, 0x64
        je pop_and_jmp
        movzx ecx, word ptr ds : [esi + 0x04]
        push eax
        push ecx
        mov ecx, 0x0091AD64
        call checkDebuffSkill
        mov ebp, eax
        test ebp, ebp
        je pop_original
        cmp byte ptr ds : [ebp + 0x1F] , 03
        je pop_and_jmp
        popad

        originalcode :
        cmp word ptr ds : [esi + 04] , 398
            je isHideDebuffReturn_
            cmp dword ptr ds : [esi + 0x20] , 01
            je originalCondition
            jmp isHideDebuffReturn

            isHideDebuffReturn_ :
        jmp isHideDebuffReturn

            pop_and_jmp :
        popad
            jmp ignoreDebuff

            pop_original :
        popad
            jmp originalcode

            originalCondition :
        jmp originalConditionAddr
    }
}

// === Hotkey thread: SHIFT + Sağ Tık anında X,Y yakala ve yaz ===
static DWORD WINAPI GetLocationThread(LPVOID)
{
    SHORT prevR = 0;
    for (;;)
    {
        SHORT s = GetAsyncKeyState(VK_SHIFT);
        SHORT r = GetAsyncKeyState(VK_RBUTTON);

        bool shiftDown = (s & 0x8000) != 0;
        bool rDown = (r & 0x8000) != 0;
        bool rWasDown = (prevR & 0x8000) != 0;

        if (shiftDown && rDown && !rWasDown)
        {
            int mx = ReadMouseX();
            int my = ReadMouseY();

            MouseX = mx; MouseY = my; // sadece debug
            LocationX = mx; LocationY = my;

            // HEMEN yansısın diye Pos1/Pos2'yi de güncelle
            Pos1 = mx; Pos2 = my;

            SaveIfChanged();
            Sleep(120); // tek tıkta tekrarlı tetiklemeyi engelle
        }

        prevR = r;
        Sleep(10);
    }
    // return 0;
}

// --- güvenli dönüş makrosu (EAX'a dokunmaz)
#define RETJMP(addr) __asm { push addr } __asm { ret }

static const DWORD RET_4D73F3 = 0x004D73F3; // Mov_Pos_2_Return
static const DWORD RET_4D73F8 = 0x004D73F8; // Mov_Pos_2_Jump
static const DWORD RET_4D7509 = 0x004D7509; // Mov_Pos_1_Return

// 0x4D73ED: MouseHook_4D73ED + Mov_Pos_2 (EDI/EBX)
__declspec(naked) void Hook_4D73ED_Combined()
{
    __asm {
        // opsiyonel debug
        pushfd
        pushad
        mov eax, dword ptr[0x7C3C0C]
        mov[MouseX], eax
        mov eax, dword ptr[0x7C3C10]
        mov[MouseY], eax
        popad
        popfd

        // 1) Adjust != 0 → Pos1/Pos2
        cmp byte ptr[Adjust], 0
        jne use_pos

        // 2) ÖNCE LocationX/Y (varsa)
        cmp dword ptr[LocationX], 0
        jne use_loc
        cmp dword ptr[LocationY], 0
        jne use_loc

        // 3) Sonra Pos1/Pos2 (ikisi de setse)
        cmp dword ptr[Pos1], 0
        je  fallback
        cmp dword ptr[Pos2], 0
        je  fallback

        use_pos :
        movzx edi, word ptr[Pos1]
            add   edi, -0x0A
            movzx ebx, word ptr[Pos2]
            add   ebx, -0x0A
            RETJMP(RET_4D73F8)

            use_loc :
            mov   edi, [LocationX]
            add   edi, -0x0A
            mov   ebx, [LocationY]
            add   ebx, -0x0A
            RETJMP(RET_4D73F8)

            fallback :
            sub edi, 0x00000082
            RETJMP(RET_4D73F3)
    }
}

// 0x4D7503: NewLocation_4D7503 + Mov_Pos_1 (yalnız EDI)
__declspec(naked) void Hook_4D7503_Combined()
{
    __asm {
        // 1) Adjust != 0 → Pos1
        cmp byte ptr[Adjust], 0
        jne use_pos

        // 2) ÖNCE LocationX (varsa)
        cmp dword ptr[LocationX], 0
        jne use_loc

        // 3) Sonra Pos1 (setse)
        cmp dword ptr[Pos1], 0
        jne use_pos

        // 4) Fallback
        sub edi, 0x00000082
        RETJMP(RET_4D7509)

        use_pos:
        movzx edi, word ptr[Pos1]
            add   edi, -0x0A
            RETJMP(RET_4D7509)

            use_loc :
            mov   edi, [LocationX]
            add   edi, -0x0A
            RETJMP(RET_4D7509)
    }
}

// === Small Icon parçaları (aynen) ===
UINT8 jump[1] = { 0xEB };


DWORD cSendSmall = 0x57B990;
DWORD cStopFuncAddr = 0x4B58F2;
DWORD small_buff_toggleReturn = 0x4B58E5;
DWORD small_buff_toggleReturn2 = 0x4B58B7;

__declspec(naked) void small_buff_toggleIcon()
{
    __asm {
        cmp byte ptr[small_buff_toggle], 1
        jne originalcode

        // === FIX: Debuff ayrı render sırasında small'ı zorla ===
        cmp byte ptr ds : [g_ForceSmallIcons] , 1
        je  do_small

        // Arkadaşın transparency hook'u 0x4D7447'de orijinal call'ı sardığı için
        // eski return-address filtresi tek başına yetmiyor. Bu flag aktifken
        // yine small render branch'ine girmeliyiz; aksi halde alpha çalışır ama icon 32x32 kalır.
        cmp byte ptr ds : [g_BuffRenderAlphaHookActive] , 1
        je  do_small

        // Eski filtre (normal buff flow için)
        push eax
        mov eax, 0x4D7450
        cmp[esp + 0x24], eax
        pop eax
        jne originalcode

        do_small :
        push 0x11
            push 0x11
            push esi
            push ebp
            push ecx
            lea ecx, [edi + 0x90CE10]
            call cSendSmall
            jmp cStopFuncAddr

            originalcode :
        push 0x20
            push 0x20
            push esi
            jmp small_buff_toggleReturn2
    }
}

DWORD cStackReturn = 0x4D74ED;
__declspec(naked) void cStack()
{
    __asm {
        cmp byte ptr ds : [small_buff_toggle] , 0x1
        jne originalcode

        add edi, 0x15
        cmp eax, 0x0A
        jmp cStackReturn

        originalcode :
        add edi, 0x23
            cmp eax, 0x0A
            jmp cStackReturn
    }
}

DWORD cWhiteReturn = 0x4D74D8;
__declspec(naked) void cWhiteSmall() {
    __asm {
        cmp byte ptr ds : [small_buff_toggle] , 0x1
        jne originalcode

        push 0x11
        push 0x11
        push edx
        jmp cWhiteReturn

        originalcode :
        push 0x20
            push 0x20
            push edx
            jmp cWhiteReturn
    }
}

double small_ico = 17;
double big_ico = 32;

DWORD cAjustTimeReturn = 0x4B6C38;
__declspec(naked) void cAjustTime()
{
    __asm {
        cmp byte ptr ds : [small_buff_toggle] , 0x1
        jne originalcode
        fmul qword ptr ss : [small_ico]
        jmp cAjustTimeReturn

        originalcode :
        fmul qword ptr ss : [big_ico]
            jmp cAjustTimeReturn
    }
}
DWORD cAjustTime2Return = 0x4B6BF3;
__declspec(naked) void cAjustTime2()
{
    __asm {
        cmp byte ptr ds : [small_buff_toggle] , 0x1
        jne originalcode
        push 0x03
        push 0x11
        add esi, 0x11
        jmp cAjustTime2Return

        originalcode :
        push 0x03
            push 0x20
            add esi, 0x20
            jmp cAjustTime2Return
    }
}
DWORD cAjustMenssageReturn = 0x4D74AB;
__declspec(naked) void cAjustMenssage()
{
    __asm {
        cmp byte ptr ds : [small_buff_toggle] , 0x1
        jne originalcode
        add eax, 0x11
        cmp dword ptr ds : [0x7C3C10] , eax
        jmp cAjustMenssageReturn

        originalcode :
        add eax, 0x20
            cmp dword ptr ds : [0x7C3C10] , eax
            jmp cAjustMenssageReturn
    }
}

// === Periyodik INI flush (opsiyonel) ===
static DWORD WINAPI PeriodicSaveThread(LPVOID)
{
    for (;;)
    {
        SaveIfChanged();
        Sleep(1000);
    }
    // return 0;
}


// === Buff transparency / alpha hook ===
// Arkadaşın gönderdiği kodun düzeltilmiş hali.
// Önemli düzeltmeler:
// 1) Return adresi 0x004D7447 + 9 = 0x004D7450 olmalı.
// 2) Bu hook sadece alpha/color değiştirir; small-icon ölçülerinin çalışması için
//    g_BuffRenderAlphaHookActive flag'i small_buff_toggleIcon() içinde dikkate alınıyor.
DWORD cBuffRenderAlphaCall = 0x004B57B0;
DWORD cBuffRenderAlphaReturn = 0x004D7450;

__declspec(naked) void buff_transparency_hook()
{
    __asm {
        mov byte ptr ds : [g_BuffRenderAlphaHookActive] , 1

        cmp byte ptr ds : [g_TransparentIcons] , 1
        jne original_alpha

        // enabled behavior: 0x80 alpha + white RGB
        push 0x80FFFFFF
        mov ecx, ebp
        call cBuffRenderAlphaCall
        mov byte ptr ds : [g_BuffRenderAlphaHookActive] , 0
        jmp cBuffRenderAlphaReturn

        original_alpha :
        // original game behavior: full opaque white
        push -1
        mov ecx, ebp
        call cBuffRenderAlphaCall
        mov byte ptr ds : [g_BuffRenderAlphaHookActive] , 0
        jmp cBuffRenderAlphaReturn
    }
}


void Buff_Icons()
{
    LoadFromIni();

 
    utils::Hook((LPVOID)0x4D73ED, Hook_4D73ED_Combined, 11);
    utils::Hook((LPVOID)0x4D7503, Hook_4D7503_Combined, 6);

    // Thread'ler
    HANDLE t1 = CreateThread(nullptr, 0, GetLocationThread, nullptr, 0, nullptr);
    if (t1) CloseHandle(t1);
    HANDLE t2 = CreateThread(nullptr, 0, PeriodicSaveThread, nullptr, 0, nullptr);
    if (t2) CloseHandle(t2);

    // Skill level
    utils::MemWrite((LPVOID)0x4B58C7, jump, 1);

    // Buff transparency / alpha (düzeltilmiş arkadaş kodu)
    utils::Hook((LPVOID)0x004D7447, buff_transparency_hook, 9);

    // SMALL ICONS
    utils::Hook((LPVOID)0x4B58B2, small_buff_toggleIcon, 5);
    utils::Hook((LPVOID)0x4D74E7, cStack, 6);
    utils::Hook((LPVOID)0x4D74D3, cWhiteSmall, 5);
    utils::Hook((LPVOID)0x4B6C32, cAjustTime, 6);
    utils::Hook((LPVOID)0x4B6BEC, cAjustTime2, 7);
    utils::Hook((LPVOID)0x4D74A2, cAjustMenssage, 9);

    // Debuff
    utils::Hook((LPVOID)0x4D7404, separateDebuffMain, 6);
    utils::Hook((LPVOID)0x4D742F, isHideDebuff, 10);

    // MousePos
    utils::Hook((LPVOID)0x48882C, MousePos, 6);

    // INI'den Pos'ları bir kerelik oku
    CreateThread(nullptr, 0, getValuePosThread, nullptr, 0, nullptr);
}
