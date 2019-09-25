set PYTHONHOME=%USERPROFILE%\AppData\Local\Programs\Python\Python37
set PYTHONPATH=%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;
set Path=%Path%;%PYTHONHOME%;%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;%PYTHONHOME%\Scripts;%PYTHONHOME%;
cd "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE"
cd "D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE"
start /b devenv.exe
