### PYBIND11 AND VISUAL STUDIO DEBUG C++/Python

# saved thousand of years of wasted effort!!!!
# for system user and visual studio see Python
[Environment]::SetEnvironmentVariable("PYTHONHOME", "D:\Users\andre.ferreira\AppData\Local\Programs\Python\Python37", "User")

# building why? pybdin11
cmake CMakeLists.txt -G"Visual Studio 16 2019" -A x64

What is needed to work and debug with pybind11 and VStudio
Open a cmd and type this bellow to open VStudio
pybind11 includes must be from anaconde\include where it were installed with "conda install pybind11"
and libs and everything else from python37 installation path bellow
packages must be installed with pip (same cmd sequence)
must use Release to debug and config to build Release with debug info.

set PYTHONHOME=D:\Users\andre.ferreira\AppData\Local\Programs\Python\Python37
set PYTHONPATH=%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;
set Path=%Path%;%PYTHONHOME%;%PYTHONHOME%\DLLs;%PYTHONHOME%\Lib;%PYTHONHOME%\Lib\site-packages;%PYTHONHOME%\Scripts;%PYTHONHOME%;
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe"


### TA_Lib

From here:.
https://mrjbq7.github.io/ta-lib/install.html

Windows:

Use this binary package here:
https://www.lfd.uci.edu/~gohlke/pythonlibs/#ta-lib

search for 'TA-Lib'
Place it in C:\ such as C:\talib is the folder.
use something like this to install the whl package above using the library:
`C:\ta-lib\c>pip install D:\Users\alferreira\Downloads\TA_Lib-0.4.17-cp37-cp37m-win_amd64.whl`

Linux:

Download de source code and:.

$ untar and cd
$ ./configure --prefix=/usr/local/lib/talib
$ make
$ sudo make install

Install somewhere, for example:. /usr/local/lib/talib
Install pip on anaconda and then make pip find the library when installing and compiling the python module.
pip install --global-option=build_ext --global-option="-L/usr/local/lib/talib/lib" --global-option="-I/usr/local/lib/talib/include/" TA-Lib
after copy the libraries to the anaconda libraries folder

cd /usr/local/lib/talib/lib
cp * /home/alferreira/anaconda3/lib

Everything is ready.
