#include "dll.h"
#include "callpython.h"

py::module pycode; // python module to be loaded


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
		size_t size = strpyModel.length();
#ifdef META5DEBUG
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
#ifdef META5DEBUG
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

// load python_code.py
void LoadPythonCode(){
  pycode = py::module::import("python_code");
}
