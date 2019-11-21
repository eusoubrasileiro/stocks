#pragma once
int pyTrainModel(double X[], int y[], int ntraining, int xtrain_dim,
    char* model, int pymodel_size_max);
int pyPredictwModel(double X[], int xtrain_dim,
    char* model, int pymodel_size);
