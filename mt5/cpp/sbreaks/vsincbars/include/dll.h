//
//
// This is:
// 1. __stdcall windows dll for generic porpouses
// 2. a __cdecl x64 Python module can be imported as `import incbars.dll`
// 
// But here only the exports for mql5 or even c++ call
//
//
#pragma once

#ifndef DLLEXPORTS_H
#define DLLEXPORTS_H

#define NOMINMAX

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <iostream>

#include "eincbands.h"
#include "callpython.h"
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

namespace py = pybind11;

inline const std::string BoolToString(bool b) { return b ? "true" : "false"; }
inline const std::string PtrToString(LPVOID b) { return b ? "not-null" : "null"; }

BOOL DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved);

#ifdef META5DEBUG
#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
extern std::ofstream debugfile;
#else
#define debugfile std::cout
#endif


#endif //DLLEXPORTS_H
