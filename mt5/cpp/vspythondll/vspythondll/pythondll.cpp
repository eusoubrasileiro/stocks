// dllmain.cpp : Defines the entry point for the DLL application.
#include "exports.h"
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

namespace py = pybind11;

static py::module pycode; // python module to be loaded
static bool python_started = false; // python interpreter started or not

int Unique(double arr[], int n) {
	py::buffer_info buf;
	double* ptr;
	// create python array and copy c array data to it
	py::array_t<double> npyarray = py::array_t<double>(n);
	buf = npyarray.request();
	ptr = (double*) buf.ptr;
	std::memcpy(ptr, arr, n * sizeof(double));
	// call python code use same array to get output
	npyarray = pycode.attr("Unique")(npyarray);
	// result copy back to same array
	buf = npyarray.request();
	ptr = (double*) buf.ptr;
	std::memcpy(arr, ptr, buf.shape[0] * sizeof(double));	
	return buf.shape[0]; // return unique size
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
		
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		if (!python_started) {	
			try {			
				py::initialize_interpreter();
				//pycode = py::module::import("numpy");
				//pycode = py::module::import("sys");
				//py::print(pycode.attr("path"));
				//pycode.attr("path").attr("clear")(); // clear the current path list
				//append_path = pycode.attr("path").attr("append");
				// add all anaconda3 default sys paths to import numpy etc...
				//for (auto path = anaconda_paths.begin(); path != anaconda_paths.end(); path++) {
				//	append_path(*path);
				//}	
				//py::print(pycode.attr("path"));		
				//pycode = py::module::import("numpy");
				pycode = py::module::import("python_code");
				python_started = true;
			}
			catch (py::error_already_set const& pythonErr) { std::cout << pythonErr.what(); }
		}		
		// attach to process
		// return FALSE to fail DLL load
		break;
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH:
		if (python_started) {
			pycode.~module(); // must destroy here otherwise will try to destroy 
			// after the interpreter is destroyed and booom!!!$&##ии*и@!(@
			py::finalize_interpreter();
			python_started = false;
		}
		// detach from process
        break;
    }
    return TRUE;
}

