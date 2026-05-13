#include "hook.h"


const char* login_ip_addr = AY_OBFUSCATE("127.0.0.1"); // It hides the IP address inside the DLL.
auto login_ip_addr_format = "%s";

DWORD write_login_ip_addr_retn = 0x409654;
_declspec(naked) void write_login_ip_addr()
{
	_asm {
		push login_ip_addr
		push login_ip_addr_format
		push 0x7FE290 // ip string formatted
		call sprintf
		add esp, 12
		jmp write_login_ip_addr_retn
	}
}

_declspec(naked) void select_server_packet()
{
	_asm {
		mov eax, 0x0A301
	}
}

_declspec(naked) void game_port()
{
	_asm {
		push 0x785A
	}
}

_declspec(naked) void login_port()
{
	_asm {
		push 0x7850
	}
}

void NETWORK() {
	utils::Hook((LPVOID)0x409552, write_login_ip_addr, 5);
	//utils::MemWrite((LPVOID)0x60C50F, select_server_packet, 5);
	utils::MemWrite((LPVOID)0x517F09, game_port, 5);
	utils::MemWrite((LPVOID)0x43EFD0, game_port, 5);
	utils::MemWrite((LPVOID)0x481A70, game_port, 5);
	utils::MemWrite((LPVOID)0x40B172, login_port, 5);
	utils::MemWrite((LPVOID)0x493A6E, login_port, 5);
	utils::MemWrite((LPVOID)0x4DD1E8, login_port, 5);
}

