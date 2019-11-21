//
//
// This is:
// 1. __stdcall windows dll for generic porpouses
// 2. a __cdecl x64 Python module can be imported as `import incbars.dll`
// 
// But here only the exports for mql5 or even c++ call
//
//

#ifndef DLLEXPORTS_H
#define DLLEXPORTS_H

#define NOMINMAX

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <iostream>

BOOL __stdcall DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved);

/*  To use this exported function of dll, include this header
 *  in your project.
 */



#endif //DLLEXPORTS_H
