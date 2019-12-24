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