// dllmain.cpp : Defines the entry point for the DLL application.
#include "exports.h"
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

#include <iostream>

#ifdef DEBUG
#include <fstream> // debugging dll load by metatrader 5

std::ofstream debugfile("vspythondll_log.txt");

inline const std::string BoolToString(bool b)
{
	return b ? "true" : "false";
}

inline const std::string PtrToString(LPVOID b)
{
	return b ? "not-null" : "null";
}

#endif

namespace py = pybind11;

static py::module pycode; // python module to be loaded
//std::mutex dll_mutex; // guarantee thread only acess to internal dll variables

int Unique(double arr[], int n) {
	//std::lock_guard<std::mutex> guard(dll_mutex); // unlocks the mutex when destroctor is called
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
	//std::lock_guard<std::mutex> guard(dll_mutex); // unlocks the mutex when destroctor is called
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
#ifdef DEBUG
			debugfile << "PROCESS_ATTACH" << std::endl;
			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
			debugfile << "lpReserved: " + PtrToString(lpReserved) << std::endl;
#endif // DEBUG
			try {
				if (!Py_IsInitialized()) // starts only once
					py::initialize_interpreter();
				pycode = py::module::import("python_code");
			}
			catch (py::error_already_set const& pythonErr){
#ifdef DEBUG
				debugfile << "python exception: " << std::endl;
				debugfile << pythonErr.what() << std::endl;
#endif // DEBUG
			}
			catch (const std::exception& ex) {
#ifdef DEBUG
				debugfile << "c++ exception: " << std::endl;
				debugfile << ex.what() << std::endl;
#endif // DEBUG
			}
			catch (...)
			{
#ifdef DEBUG
				debugfile << "Weird no idea exception" << std::endl;
#endif // DEBUG
			}
		// attach to process
		// return FALSE to fail DLL load
		break;
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH: // will keep runing until mt5 dies
#ifdef DEBUG
			debugfile << "PROCESS_DETACH" << std::endl;
			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
			debugfile << "lpReserved: " + PtrToString(lpReserved) << std::endl;
///		if (Py_IsInitialized()) {
///			pycode.~module(); // must destroy here otherwise will try to destroy
///			// after the interpreter is destroyed and booom!!!$&##��*�@!(@
///			py::finalize_interpreter();
///			debugfile << "finalized interpreter" << std::endl;
///			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
///		}
		// detach from process
#endif
        break;
    }
    return TRUE;
}
