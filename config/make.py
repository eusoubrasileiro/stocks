#!/usr/bin/env python
import subprocess
import os, sys, glob
import shutil
import platform
from pathlib import Path
from argparse import ArgumentParser
import pandas as pd
from xml.sax import ContentHandler, parse
import configparser # just awesome for manipulating config *.ini files

parser = ArgumentParser()
parser.add_argument("-meta5", dest="meta5", default=False, action="store_true",
                    help="link all *.mql and *.mqh files to MetaTrader 5 Advisors Path")
parser.add_argument("-cppbuild", dest="cppbuild", default=False, action="store_true",
                    help="build cpp library as a dll for mt5 using mingw32-w64 or cl on windows")
parser.add_argument("-cppdebug", dest="cppdebug", default=False, action="store_true",
                    help="build cpp's library with DEBUG preprocessor slows it down")
parser.add_argument("-clean", dest="clean", default=False, action="store_true",
                    help="clean all symlinks")
parser.add_argument("-newdata", dest="newdata", default=False, action="store_true",
                    help="copy new Metatrader 5 binary files to data folder")
parser.add_argument("-cpdll", dest="cpdll", default=False, action="store_true",
                    help="copy built dll and dependencies to all Metatrader 5 tester agent library folders")
parser.add_argument("-optim", dest="optim", default=False, action="store_true",
                    help="run optimization for specified expert and all symbols passed")
args = parser.parse_args()


userhome = str(Path.home()) # get userhome folder
# repository and libraries paths
repopath = os.path.join(userhome, r"Projects\stocks")
# c++ library
armadilloroot  = os.path.join(userhome,r"Projects\armadillo-code-9.600.x")
# aewsome c++ library (better use this one? same of vstudio?)
pybindroot =os.path.join(userhome, r"Projects\pybind11-master")
# c library talib x64
talibroot  = os.path.join(userhome,r"Projects\ta-lib\c")
 # represents the local MetaTrader 5 installation
usermt5hash = '8B052D0699A0083067EBF3A36123603B'
# local user paths for MetaTrader 5 installation
usermt5path = os.path.join(userhome,r"AppData\Roaming\MetaQuotes")
# better use this one? same of vstudio?
# python foundation source code - installation needed to buil python dll
pythonpath = os.path.join(userhome, r"AppData\Local\Programs\Python\Python37")
libtorchpath = os.path.join(userhome, r"Projects\libtorch")

if os.name == 'nt':
    # getting which machine we are
    if platform.uname()[5] == 'Intel64 Family 6 Model 158 Stepping 9, GenuineIntel': # HOME
        mt5path = r"D:\MetaTrader 5"
        # command to create build env using vsbuildtools default installation paths
        vsbuildenvcmd = "\"C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\vcvarsall.bat\" " + ' amd64'
    else: # WORK
        mt5path = r"D:\MetaTrader 5"
        ### Using vs build tools
        vsbuildenvcmd = os.path.join(repopath, r'config\vsbuildtools.bat')
    if args.cppbuild:
        ######################################
        # ARMADILLO DLL
        ######################################
        arminc = os.path.join(armadilloroot, "include")
        armlibs = os.path.join(armadilloroot, r"examples\lib_win64\blas_win64_MT.lib")
        cpppath = os.path.join(repopath, r"mt5\cpp\arm")
        ## cl /LD /EHsc /Gz /Fe"cpparm" /std:c++17 armcpp.cpp /I
        ## /DBUILDING_DLL /LINK D:\Users\andre.ferreira\Projects\armadillo-code-9.600.x\examples\lib_win64\blas_win64_MT.lib
        ## /Fe"cpparm" output name dll (could also be a path)
        ## /LD make a dll
        ## /Gz make __stdcall convention for all functions in code
        ## /EHsc enable execption handling
        ## /O2 optimize code for speed
        ## /D make a predefinition of symbol BUILDING_DLL
        ## Windows & is the equivalent of ; on linux
        compile = ("cd "+ cpppath + " & " + " @echo off " + " & " +  # dont echo vsbuildenvcmd
                    vsbuildenvcmd +" && "+ " @echo on " + " & " +
                r"cl.exe /LD /EHsc /Gz /Fecpparm /DBUILDING_DLL /O2 armcpp.cpp"+
                " -I "+ arminc + " " + armlibs)
        print(compile, file=sys.stderr)
        subprocess.call(compile, shell=True)
        ######################################
        # CTALIB DLL
        ######################################
        talibinc = os.path.join(talibroot, "include")
        taliblibs = "ta_libc_cdr_x64.lib"
        cpppath = os.path.join(repopath, r"mt5\cpp\ctalib")
        ## /MD saved my life
        ## msvcrt.lib: import library for the release DLL version of the CRT
        compile = ("cd "+ cpppath + " & " + " @echo off " + " & " + # dont echo vsbuildenvcmd
                    vsbuildenvcmd +" && "+ " @echo on " + " & " +
            r"cl.exe /MD /EHsc /Gz /DBUILDING_DLL /O2 ctalib.cpp" +
             " /I "+ talibinc +
            # linker options bellow
            " /link /OUT:ctalib.dll " + taliblibs +
            " /IMPLIB:ctalib.lib " +
            " /DLL /MACHINE:X64 ")
        print(compile, file=sys.stderr)
        subprocess.call(compile, shell=True)
        # ######################################
        # # PYTHON DLL
        # ######################################
        cppdebug = ""
        if args.cppdebug:
            cppdebug = " /DDEBUG "
        pythonincludes = os.path.join(pythonpath, "include")
        pythonlibs = os.path.join(pythonpath, r"libs\python37.lib") # never use _d debug
        pybindincludes = os.path.join(pybindroot, "include")
        cpppath = os.path.join(repopath, r"mt5\cpp\pythondll\vspythondll")
        compile = ("cd "+ cpppath + " & " + " @echo off " + " & " + # dont echo vsbuildenvcmd
                    vsbuildenvcmd +" && "+ " @echo on " + " & " +
                r"cl.exe /LD /EHsc /Gz /Fepythondlib /DBUILDING_DLL "+ cppdebug + " /O2  pythondll.cpp"+
                " /I "+ pythonincludes + " /I " + pybindincludes + " " + pythonlibs)
        print(compile, file=sys.stderr)
        subprocess.call(compile, shell=True)
        # ######################################
        # # PYTORCHCPP DLL
        # ######################################
        cppdebug = ""
        if args.cppdebug:
            cppdebug = " /DDEBUG "
        libtorchincludes1 = os.path.join(libtorchpath, r"include\torch\csrc\api\include")
        libtorchincludes2 = os.path.join(libtorchpath, r"include")
        libtorchlibs = ' '.join([ os.path.join(libtorchpath, r"lib\\"+lib)
                                for lib in ['c10.lib', 'torch.lib'] ])
        cpppath = os.path.join(repopath, r"mt5\cpp\pytorchcpp\pytorch")
        compile = ("cd "+ cpppath + " & " + " @echo off " + " & " + # dont echo vsbuildenvcmd
                    vsbuildenvcmd +" && "+ " @echo on " + " & " +
                r"cl.exe /LD /EHsc /Ot /Gz /Fepytorchcpp /DBUILDING_DLL "+ cppdebug + " /O2  pytorch.cpp"+
                " /I "+ libtorchincludes1 + " /I " + libtorchincludes2 + " " + libtorchlibs)
        print(compile, file=sys.stderr)
        subprocess.call(compile, shell=True)

    if args.meta5: # create a softlink between repo folder and metatrader5 users advisors path
        # cannot create hardlink but a softlink (junction) works
        # can only create a junction without being admin
        repopath_mt5 = os.path.join(repopath, 'mt5')
        # D:\Users\andre.ferreira\AppData\Roaming\MetaQuotes\Terminal\8B052D0699A0083067EBF3A36123603B\MQL5\Experts\mt5"
        usermt5path_advisorsmt5 = os.path.join(usermt5path, "Terminal", usermt5hash, r"MQL5\Experts\mt5")
        symlink = r'mklink /j ' + "\"" + usermt5path_advisorsmt5 + "\""+ " " +  repopath_mt5
        print(symlink, file=sys.stderr)
        subprocess.call(symlink, shell=True)

    if args.cpdll:
        repopath_dlls = os.path.join(repopath, r'mt5\cpp')
        dllpaths = list(Path(repopath_dlls).glob(r'**\*.dll')) # all dll's # (glob recursive not working)
        dllpaths += list(Path(repopath_dlls).glob(r'**\*.pyd'))
        # include pytorchcpp dll's
        dllpaths += list(Path(os.path.join(libtorchpath, 'lib')).glob(r'**\*.dll'))

        # for repeated files get the created most recently
        times = [ os.path.getctime(dll) for dll in dllpaths ]
        dllnames = [os.path.basename(dll) for dll in dllpaths ]
        df = pd.DataFrame(zip(dllnames, times, dllpaths), columns=['names', 'time', 'path'])
        df = df[df['time'] == df.groupby(['names'])['time'].transform(min)] # dont understand
        # transform(min) for me should be max - but works
        dllpaths = df.path.values
        print(dllpaths, file=sys.stderr)
        # every dll to mt5path root D:\Metatrader 5\ path MUCH better
        for dll in dllpaths: #  than multiple cpies to every agent tester path
            shutil.copy(dll, mt5path)
        # must also copy python_code.py
        # must run mt5 with python37 envs variables set on "cmd"
        pythoncode_path = os.path.join(repopath, r"mt5\cpp\pythondll\vspythondll\python_code.py")
        shutil.copy(pythoncode_path, mt5path)

    if args.optim:
        # run optimization on default symbols stocks passed as params (to implement)
        symbols = ["BBDC4", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4", "WEGE3", "UGPA3"]
        #mt5terminalexe = os.path.join(mt5path, 'terminal64.exe')
        expert = 'NaiveGap'
        # copy params set files all to
        paramset_files = list(Path(os.path.join(repopath, r'mt5\params')).glob('**\*.set')) # all *.set files
        for symbol in symbols:
            #symbol = r'WIN@'
            # The Expert Advisor parameters MUST BE at usermt5path\Terminal\8B052D0699A0083067EBF3A36123603B\MQL5\Profiles\Tester
            usermt5path_testerparams = os.path.join(usermt5path, "Terminal", usermt5hash, r"MQL5\Profiles\Tester")
            for paramset_file in paramset_files: # every *.set file
                shutil.copy(paramset_file, usermt5path_testerparams)
            defaultOptconfigpath = os.path.join(repopath, r'mt5\config\optconfigDefault.ini')
            # write a new config file just for executing this symbol
            config = configparser.ConfigParser()
            config.read(defaultOptconfigpath)
            config['Tester']['ExpertParameters'] = 'opt'+expert+'.set' # optmin param set file
            config['Tester']['Symbol'] = symbol
            config['Tester']['Optimization'] = '2' # 1- complete slow  / 2- genetic
            config['Tester']['shutdownterminal'] = '1' # shutdown after (1) or not (0) execution
            config['Tester']['Report'] ='opt'+expert+'_'+symbol+'_report.xml' # report file
            # save and execute the created config file
            execOptconfigpath = os.path.join(repopath, r'mt5\config\optconfig' + symbol + '.ini')
            with open(execOptconfigpath, 'w') as file:
                config.write(file)
            runoptimsymbol = "cd "+ mt5path + " & " + r" D: " + " & " +"terminal64.exe /config:"+execOptconfigpath
            print(runoptimsymbol, file=sys.stderr)
            subprocess.call(runoptimsymbol, shell=True)
            # copy reports back to repo folder
            # reports file path, 2 reports one for forward and another commom
            usermt5_optreportpath = os.path.join(usermt5path, "Terminal", usermt5hash)
            usermt5_optreports = list(Path(usermt5_optreportpath).glob('*.xml')) # all *.xml files
            repopath_optreport = os.path.join(repopath, r'mt5\optimization')
            for usermt5_optreport in usermt5_optreports:
                shutil.copy(usermt5_optreport, repopath_optreport)

# dont see when will use this again since Wine/MetaTrader is not good for MT5
# else: ### Ubuntu
#     if args.clean:
#         subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
#                             find . -type l -exec unlink \{\} \;""", shell=True)
#
#     if args.meta5:
#         subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
#                         find . -type l -exec unlink \{\} \;""", shell=True)
#         # make symlinks from stocks folder to Metatrader folder
#         subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors';
#                     ln -s /home/andre/Projects/stocks/mt5/*.mq* .""", shell=True)
#
#     if args.cppbuild:
#         ### Ubuntu  everything starts with sudo apt-get install gcc-mingw-w64-x86-64
#         ### not using blas, lapack etc just vector utils - building a x64 dll for metatrader 5 c++ call
#         ### on wine64
#         ### another note is must use -static otherwise dll will depend on *dlls on linux from mingw
#         ### with static everything will be inside of the dll altough it will be 2MB
#         ### but since I started using conv - convolution I started depend upon lapack/blas
#         ### so I have to copy those *.dll from lib_win64 folder everywhere I place this library
#         subprocess.call("""cd '/home/andre/Projects/stocks/mt5/cpp';
#                         x86_64-w64-mingw32-g++-posix -c armcpp.cpp -o main.o -O3 -std=gnu++11 -DBUILDING_DLL=1 -I'/home/andre/Downloads/armadillo-9.500.2/include';
#                         x86_64-w64-mingw32-g++-posix -shared main.o -o '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Experts/Advisors/cpparm.dll' -static -L'/home/andre/Downloads/armadillo-9.500.2/examples/lib_win64' -llapack_win64_MT -lblas_win64_MT -Wl,--output-def,libcpparm.def,--out-implib,libcpparm.a,--add-stdcall-alias""", shell=True)
#
#     if args.newdata:
#         subprocess.call("""cd '/home/andre/.wine/drive_c/Program Files/MetaTrader 5/MQL5/Files';
#                         cp *.mt5bin /home/andre/Projects/stocks/data """, shell=True)
