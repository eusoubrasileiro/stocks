::
:: Cl env set-up
:: Copied from cm dev prompt setted variables VS build tools 2015 C++ from installed machine making it 'portable' useful
::
:: The following folders were needed
:: C:\ProgramFiles (x86)\Microsoft SDKs
:: C:\ProgramFiles (x86)\Microsoft Visual Studio
:: C:\ProgramFiles (x86)\Microsoft Visual Studio 14.0
:: C:\ProgramFiles (x86)\Windows Kits
::
:: Actual destination folder is C:\VSBuildTools\
::
set INCLUDE=C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\INCLUDE;C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\ATLMFC\INCLUDE;C:\VSBuildTools\Windows Kits\10\include\10.0.18362.0\ucrt;C:\VSBuildTools\Windows Kits\10\include\10.0.18362.0\shared;C:\VSBuildTools\Windows Kits\10\include\10.0.18362.0\um;C:\VSBuildTools\Windows Kits\10\include\10.0.18362.0\winrt;
set LIB=C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\LIB\amd64;C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\ATLMFC\LIB\amd64;C:\VSBuildTools\Windows Kits\10\lib\10.0.18362.0\ucrt\x64;C:\VSBuildTools\Windows Kits\10\lib\10.0.18362.0\um\x64;
set LIBPATH=C:\WINDOWS\Microsoft.NET\Framework64\v4.0.30319;C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\LIB\amd64;C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\ATLMFC\LIB\amd64;C:\VSBuildTools\Windows Kits\10\UnionMetadata;C:\VSBuildTools\Windows Kits\10\References;\Microsoft.VCLibs\14.0\References\CommonConfiguration\neutral;
set OS=Windows_NT
set Path=C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\BIN\amd64;C:\WINDOWS\Microsoft.NET\Framework64\v4.0.30319;C:\VSBuildTools\Microsoft Visual Studio 14.0\Common7\IDE;C:\VSBuildTools\Microsoft Visual Studio 14.0\Common7\Tools;C:\VSBuildTools\HTML Help Workshop;C:\VSBuildTools\Windows Kits\10\bin\x64;C:\VSBuildTools\Windows Kits\10\bin\x86;C:\VSBuildTools\Intel\iCLS Client\;C:\Program Files\Intel\iCLS Client\;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\VSBuildTools\Intel\Intel(R) Management Engine Components\DAL;C:\Program Files\Intel\Intel(R) Management Engine Components\DAL;C:\VSBuildTools\Intel\Intel(R) Management Engine Components\IPT; Corporation\PhysX\Common;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;
set PATHEXT=.COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC
set Platform=X64
set PROCESSOR_ARCHITECTURE=AMD64
set PROCESSOR_IDENTIFIER=Intel64 Family 6 Model 158 Stepping 9, GenuineIntel
set PROCESSOR_LEVEL=6
set PROCESSOR_REVISION=9e09
set ProgramData=C:\ProgramData
set ProgramFiles=C:\Program Files
set ProgramFiles(x86)=C:\Program Files (x86)
set ProgramW6432=C:\Program Files
set PROMPT=$P$G
set SESSIONNAME=Console
set SystemDrive=C:
set SystemRoot=C:\WINDOWS
set TEMP=D:\Users\andre.ferreira\AppData\Local\Temp
set TMP=D:\Users\andre.ferreira\AppData\Local\Temp
set UCRTVersion=10.0.18362.0
set UniversalCRTSdkDir=C:\VSBuildToolsWindows Kits\10\
set USERDOMAIN=pma30092
set USERDOMAIN_ROAMINGPROFILE=pma30092.dnpm.net
set USERNAME=andre.ferreira
set USERPROFILE=D:\Users\andre.ferreira\
set VCINSTALLDIR=C:\VSBuildTools\Microsoft Visual Studio 14.0\VC\
set VisualStudioVersion=14.0
set VS140COMNTOOLS=C:\VSBuildTools\Microsoft Visual Studio 14.0\Common7\Tools\
set VSINSTALLDIR=C:\VSBuildTools\Microsoft Visual Studio 14.0\
set windir=C:\WINDOWS
set WindowsLibPath=C:\VSBuildTools\Windows Kits\10\UnionMetadata;C:\VSBuildTools\Windows Kits\10\References
set WindowsSdkDir=C:\VSBuildTools\Windows Kits\10\
set WindowsSDKLibVersion=10.0.18362.0\
set WindowsSDKVersion=10.0.18362.0\
