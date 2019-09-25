// dllmain.cpp : Defines the entry point for the DLL application.
#include "exports.h"
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include <iostream>

#ifdef DEBUG
#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
std::ofstream debugfile("pythondlib_log.txt");
#else
#define debugfile std::cout
#endif

inline const std::string BoolToString(bool b){ 	return b ? "true" : "false"; }
inline const std::string PtrToString(LPVOID b) { return b ? "not-null" : "null"; }


namespace py = pybind11;

static py::module pycode; // python module to be loaded

// https://docs.python.org/3/c-api/init.html#non-python-created-threads
PyGILState_STATE gstate; // calling python from a thread not created by Python itself

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

int pyTrainModel(double X[], int y[], int ntraining, int xtrain_dim,
						char *model, int pystr_size){
	py::buffer_info buf;
	double* dptr;
	int* iptr;
	// create python array and copy c array data to it
	// X feature vector
	py::array_t<double> pyX = py::array_t<double>(ntraining * xtrain_dim);
	buf = pyX.request();
	dptr = (double*) buf.ptr;
	std::memcpy(dptr, X, ntraining * xtrain_dim * sizeof(double));
	pyX.resize({ ntraining, xtrain_dim });
	// y target class vector
	py::array_t<int> pyY = py::array_t<int>(ntraining);
	buf = pyY.request();
	iptr = (int*) buf.ptr;
	std::memcpy(iptr, y, ntraining * sizeof(int));

	std::string strpyModel;

	try {
		// call python code
		strpyModel = pycode.attr("pyTrainModel")(pyX, pyY).cast<py::bytes>();
		unsigned int size = strpyModel.length();
#ifdef DEBUG
		if (ntraining > 2) {
			debugfile << "X: " << X[0] << " " << X[1] << " " << X[2] << std::endl;
			debugfile << "y: " << y[0] << " " << y[1] << " " << y[2] << std::endl;
			debugfile << "strsize: " << pystr_size << std::endl;
			debugfile << "model size: " << size << std::endl;
		}
#endif
		if (strpyModel.length() > pystr_size)
			return 0;
		std::memcpy(model, strpyModel.data(), strpyModel.length() * sizeof(char));
	}
	catch (py::error_already_set const& pythonErr) {
		debugfile << "python exception: " << std::endl;
		debugfile << pythonErr.what() << std::endl;
	}
	catch (const std::exception& ex) {
		debugfile << "c++ exception: " << std::endl;
		debugfile << ex.what() << std::endl;
	}
	catch (...) {
		debugfile << "Weird no idea exception" << std::endl;
	}

	return strpyModel.length();// return size of model can be 0 on failure
}



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {

    case DLL_PROCESS_ATTACH:
			debugfile << "PROCESS_ATTACH" << std::endl;
			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
			try {
				if (!Py_IsInitialized()) // starts only once on the mt5 process
					py::initialize_interpreter();
				else // just get the GIL for the thread who called the DLL
					gstate = PyGILState_Ensure();
				pycode = py::module::import("python_code");
			}
			catch (py::error_already_set const& pythonErr){
				debugfile << "python exception: " << std::endl;
				debugfile << pythonErr.what() << std::endl;
			}
			catch (const std::exception& ex){
				debugfile << "c++ exception: " << std::endl;
				debugfile << ex.what() << std::endl;
			}
			catch (...){
				debugfile << "Weird no idea exception" << std::endl;
			}
		// attach to process
		// return FALSE to fail DLL load
		break;
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH: // will keep runing until mt5 dies
		//if (Py_IsInitialized()) {
		//	pycode.~module(); // must destroy here otherwise will try to destroy
			// after the interpreter is destroyed and booom!!!$&##��*�@!(@
		//	py::finalize_interpreter();
		//	debugfile << "finalized interpreter" << std::endl;
		//	debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
		//}
		// Rofdl did finalize the interpreter after every dll unload
		// Numpy array poses a problem since it's data structures
		// are not fully unloaded and them breaks the interpreter when imported again
		// with a subsequent call to PyInitialize
		// so better not unload if you want to use numpy
			debugfile << "PROCESS_DETACH" << std::endl;
			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
		// Solved many booms $&##��*�@!(@ on metatrader 5
		// releasing the thread GIL (note that the main thread allways has one of this)
		PyGILState_Release(gstate);
        break;
    }
    return TRUE;
}
