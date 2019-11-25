// python interpreter embedded in this dll
#include "dll.h"
#include "embpython.h"
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

namespace py = pybind11;
static py::module pycode; // python module to be loaded

#ifdef EMBPYDEBUG
#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
std::ofstream debugfile("embpython_log.txt");
#else
#define debugfile std::cout
#endif

inline const std::string BoolToString(bool b){ 	return b ? "true" : "false"; }
inline const std::string PtrToString(LPVOID b) { return b ? "not-null" : "null"; }

int pyTrainModel(double X[], int y[], int ntraining, int xtrain_dim,
						char *model, int pymodel_size_max){
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
			debugfile << "X: " << X[ntraining-3] << " " << X[ntraining-2] << " " << X[ntraining-1] << std::endl;
			debugfile << "y: " << y[ntraining-3] << " " << y[ntraining-2] << " " << y[ntraining-1] << std::endl;
			debugfile << "pymodel_size max: " << pymodel_size_max << std::endl;
			debugfile << "pymodel_size: " << size << std::endl;
		}
#endif
		if (strpyModel.length() > pymodel_size_max) // not enough space to store the model
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

int pyPredictwModel(double X[], int xtrain_dim,	char *model, int pymodel_size)
{
	py::buffer_info buf;
	double* dptr;
	// create python array and copy c array data to it
	// X feature vector
	py::array_t<double> pyX = py::array_t<double>(xtrain_dim);
	buf = pyX.request();
	dptr = (double*) buf.ptr;
	std::memcpy(dptr, X, xtrain_dim * sizeof(double));
	pyX.resize({ 1, xtrain_dim });

	py::bytes strpyModel(model, pymodel_size);
	int y_pred = -100;

	try {
		// call python code
		y_pred = pycode.attr("pyPredictwModel")(pyX, strpyModel).cast<int>();
#ifdef DEBUG
		if (xtrain_dim > 2) {
			debugfile << "X: " << X[xtrain_dim-3] << " " << X[xtrain_dim-2] << " " << X[xtrain_dim-1] << std::endl;
			debugfile << "y_pred: " << y_pred << std::endl;
		}
#endif
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
	return y_pred;
}


// https://docs.python.org/3/c-api/init.html#non-python-created-threads
PyGILState_STATE gstate; // calling python from a thread not created by Python itself

bool calledbyPython=false; // if called by Python dll behaves differently

BOOL __stdcall DllMain( HMODULE hModule,
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
    case DLL_PROCESS_DETACH: // will keep runing until mt5 dies or python dies
			debugfile << "PROCESS_DETACH" << std::endl;
			debugfile << "python_started: " + BoolToString(Py_IsInitialized()) << std::endl;
		// Solved many booms $&##��*�@!(@ on metatrader 5
		// releasing the thread GIL (note that the main thread allways has one of this)
		PyGILState_Release(gstate);
        // only use when called as a python module
        if (Py_IsInitialized() && calledbyPython) { 
	        pycode.~module(); // must destroy here otherwise will try to destroy
            //after the interpreter is destroyed and booom!!!$&##��*�@!(@
	        py::finalize_interpreter();
	        debugfile << "finalized interpreter" << std::endl;
         }
        break;
    }
    return TRUE;
}
