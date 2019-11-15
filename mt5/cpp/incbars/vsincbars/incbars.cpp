// dllmain.cpp : Defines the entry point for the DLL application.
#include "exports.h"
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "buffers.h"
#include "time.h"
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



BOOL __stdcall DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {

    case DLL_PROCESS_ATTACH:
		// attach to process
		// return FALSE to fail DLL load
		break;
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH: 

        break;
    }
    return TRUE;
}
