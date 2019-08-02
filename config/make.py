#!/usr/bin/env python
import subprocess
import os, sys, glob
import shutil
import platform
from argparse import ArgumentParser

mt5advpath = r"/home/andre/.wine/drive_c/Program\ Files/Rico\ MetaTrader\ 5/MQL5/Experts/Advisors/"
mt5cmpath = r"/home/andre/.wine/drive_c/users/andre/Application\ Data/MetaQuotes/Terminal/Common/Files/"

parser = ArgumentParser()
parser.add_argument("-meta5", dest="meta5", default=False, action="store_true",
                    help="link all *.mql and *.mqh files to MetaTrader 5 Advisors Path")
parser.add_argument("-cppbuild", dest="cppbuild", default=False, action="store_true",
                    help="build cpp library as a dll for mt5 using mingw32-w64 or cl on windows")
parser.add_argument("-clean", dest="clean", default=False, action="store_true",
                    help="clean all symlinks")
parser.add_argument("-newdata", dest="newdata", default=False, action="store_true",
                    help="copy new Metatrader 5 binary files to data folder")
parser.add_argument("-cpdll", dest="cpdll", default=False, action="store_true",
                    help="copy built dll and dependencies to all Metatrader 5 tester agent library folders")

args = parser.parse_args()

if os.name == 'nt':
    # getting which machine we are
    if platform.uname()[5] == 'Intel64 Family 6 Model 158 Stepping 9, GenuineIntel': # home
        # repository and libraries paths
        repopath = r"C:\Users\andre\Projects\stocks"
        armadilloroot  = r"C:\Users\andre\Projects\armadillo-code-9.600.x" # c++ library
        # local user paths for MetaTrader 5 installation
        usermt5hash = "8B052D0699A0083067EBF3A36123603B" # represents the local MetaTrader 5 installation
        usermt5path = r"C:\Users\andre\AppData\Roaming\MetaQuotes"
        # command to create build env using vsbuildtools default installation paths
        vsbuildenvcmd = r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat amd64"
    else: # work
        # repository and libraries paths
        repopath = r'D:\Users\andre.ferreira\Projects\stocks'
        armadilloroot  = r"D:\Users\andre.ferreira\Projects\armadillo-code-9.600.x" # c++ library
        # local user paths for MetaTrader 5 installation
        usermt5hash = '8B052D0699A0083067EBF3A36123603B' # represents the local MetaTrader 5 installation
        usermt5path = r"D:\Users\andre.ferreira\AppData\Roaming\MetaQuotes"
        ### Using vs build tools
        vsbuildenvcmd = os.path.join(repopath, r'config\vsbuildtools.bat')
    if args.cppbuild:
        armadilloincludes = os.path.join(armadilloroot, "include")
        armadillolibblas64 = os.path.join(armadilloroot, r"examples\lib_win64\blas_win64_MT.lib")
        cpppath = os.path.join(repopath, r"mt5\cpp")
        ## cl /LD /EHsc /Gz /Fe"cpparm" /std:c++17 armcpp.cpp /I
        ## /DBUILDING_DLL /LINK D:\Users\andre.ferreira\Projects\armadillo-code-9.600.x\examples\lib_win64\blas_win64_MT.lib
        ## /Fe"cpparm" output name dll (could also be a path)
        ## /LD make a dll
        ## /Gz make __stdcall convention for all functions in code
        ## /EHsc enable execption handling
        ## /O2 optimize code for speed
        ## /D make a predefinition of symbol BUILDING_DLL
        ## Windows & is the equivalent of ; on linux
        compile = ("cd "+ cpppath + " & " +
                    vsbuildenvcmd +" & "+
                r"cl.exe /LD /EHsc /Gz /Fecpparm /std:c++latest /DBUILDING_DLL /O2 armcpp.cpp"+
                " -I "+ armadilloincludes +" /link " + armadillolibblas64)
        print(compile, file=sys.stderr)
        subprocess.call(compile, shell=True)
    if args.meta5: # create a softlink between repo folder and metatrader5 users advisors path
        # cannot create hardlink but a softlink (junction) works
        # can only create a junction without being admin
        repopath_mt5 = os.path.join(repopath, 'mt5')
        # D:\Users\andre.ferreira\AppData\Roaming\MetaQuotes\Terminal\8B052D0699A0083067EBF3A36123603B\MQL5\Experts\Advisors\mt5"
        usermt5path_advisorsmt5 = os.path.join(usermt5path, "Terminal", usermt5hash, "MQL5\Experts\Advisors\mt5")
        symlink = r'mklink /j ' + "\"" + usermt5path_advisorsmt5 + "\""+ " " +  repopath_mt5
        print(symlink, file=sys.stderr)
        subprocess.call(symlink, shell=True)
    if args.cpdll:
        repopath_dlls = os.path.join(repopath, 'mt5\cpp\*.dll')
        dllpaths = glob.glob(repopath_dlls, recursive=True) # all dlls
        #print(dllpaths, file=sys.stderr)
        usermt5path_testeragents = os.path.join(usermt5path, 'Tester', usermt5hash)
        #print(usermt5path_testeragents,  file=sys.stderr)
        # search and get all local tester agents paths
        testeragents= glob.glob(os.path.join(usermt5path_testeragents, 'Agent-127.0.0.1*'))
        for testeragent in testeragents: # for all tester agents copy all dlls
            testeragentpath = os.path.join(usermt5path_testeragents, testeragent, r'MQL5\Libraries')
            for dll in dllpaths: # every dll
                shutil.copy(dll, testeragentpath)
        # D:\Users\andre.ferreira\AppData\Roaming\MetaQuotes\Tester\8B052D0699A0083067EBF3A36123603B
        # ...\Agent-127.0.0.1-3000\MQL5\Experts\Advisors

else: ### Ubuntu
    if args.clean:
        subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
                            find . -type l -exec unlink \{\} \;""", shell=True)

    if args.meta5:
        subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
                        find . -type l -exec unlink \{\} \;""", shell=True)
        # make symlinks from stocks folder to Metatrader folder
        subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
                    ln -s /home/andre/Projects/stocks/mt5/*.mq* .""", shell=True)

    if args.cppbuild:
        ### Ubuntu  everything starts with sudo apt-get install gcc-mingw-w64-x86-64
        ### not using blas, lapack etc just vector utils - building a x64 dll for metatrader 5 c++ call
        ### on wine64
        ### another note is must use -static otherwise dll will depend on *dlls on linux from mingw
        ### with static everything will be inside of the dll altough it will be 2MB
        ### but since I started using conv - convolution I started depend upon lapack/blas
        ### so I have to copy those *.dll from lib_win64 folder everywhere I place this library
        subprocess.call("""cd '/home/andre/Projects/stocks/mt5/cpp';
                        x86_64-w64-mingw32-g++-posix -c armcpp.cpp -o main.o -O3 -std=gnu++11 -DBUILDING_DLL=1 -I'/home/andre/Downloads/armadillo-9.500.2/include';
                        x86_64-w64-mingw32-g++-posix -shared main.o -o '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors/cpparm.dll' -static -L'/home/andre/Downloads/armadillo-9.500.2/examples/lib_win64' -llapack_win64_MT -lblas_win64_MT -Wl,--output-def,libcpparm.def,--out-implib,libcpparm.a,--add-stdcall-alias""", shell=True)

    if args.newdata:
        subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Files';
                        cp *.mt5bin /home/andre/Projects/stocks/data """, shell=True)


# daemon:
# 	python -m algos.mt5daemons.buybandsd.py
