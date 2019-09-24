set PYTHONHOME=%USERPROFILE%\AppData\Local\Programs\Python\Python37
set PYTHONPATH=%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;
set Path=%Path%;%PYTHONHOME%;%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;%PYTHONHOME%\Scripts;%PYTHONHOME%;
cd "D:\MetaTrader 5"
start /b terminal64.exe
