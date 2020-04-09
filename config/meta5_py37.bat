set PYTHONHOME=%USERPROFILE%\Anaconda3
set PYTHONPATH=%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;
set Path=%Path%;%PYTHONHOME%;%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;%PYTHONHOME%\Scripts;%PYTHONHOME%;
D:
cd "D:\MetaTrader 5"
start /b terminal64.exe
