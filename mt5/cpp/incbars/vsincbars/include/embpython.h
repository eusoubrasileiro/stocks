#pragma once
#ifdef BUILDING_DLL
#define EXPORT 
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define IMPORT
#define DLL_EXPORT extern "C" __declspec(dllimport)
#endif

DLL_EXPORT int pyTrainModel(double X[], int y[], int ntraining, int xtrain_dim,
    char* model, int pymodel_size_max);
DLL_EXPORT int pyPredictwModel(double X[], int xtrain_dim,
    char* model, int pymodel_size);

