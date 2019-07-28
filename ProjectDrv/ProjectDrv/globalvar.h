#pragma once

typedef unsigned long ULONG;
typedef int(__stdcall *ZWOPENPROCESSTOKENEX)(ULONG, ULONG, ULONG, ULONG);

ULONG g_BuildNumber = 0;
ZWOPENPROCESSTOKENEX g_ZwOpenProcessTokenEx = NULL;
