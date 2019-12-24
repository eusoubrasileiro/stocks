#pragma once

#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

namespace py = pybind11;

int pyTrainModel(double X[], int y[], int ntraining, int xtrain_dim,
    char* model, int pymodel_size_max);

int pyPredictwModel(double X[], int xtrain_dim,
    char* model, int pymodel_size);

void LoadPythonCode();


extern py::module pycode; // python module to be loaded

class sklearnModel
{
public:
    std::vector<char> pymodel;
    // size of python model in bytes after created
    int  pymodel_size;
    bool isready; // is ready to be used

    sklearnModel(void) {
        // dont know the size of a sklearn model
        // serialized so put something big here 5MB
        isready = false;
        pymodel_size = 0; // real size
        pymodel.resize((1024 * 1024 * 5));
    }

};