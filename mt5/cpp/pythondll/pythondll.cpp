#include "exports.h"
#include <iostream>
#include "Python.h"
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

using namespace py = pybind11;
using namespace std;

static py::module pycode;

int DLL_EXPORT Unique(double arr[], int n){
  py::array_t<double> array;
  array.assign(arr, arr+n);
  pycode.attr("Unique")(arr);
}

extern "C" DLL_EXPORT int APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    static py::scoped_interpreter guard{}; // will call the destructor once needed
    pycode = py::module::import("python_code.py");
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}

 
