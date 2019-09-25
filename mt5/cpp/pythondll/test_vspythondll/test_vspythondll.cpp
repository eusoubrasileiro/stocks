#include <iostream>
#include <windows.h>

typedef int(__stdcall* funcUnique)(double*, int);
typedef int(__stdcall* funcpyTrainModel)(double*, int*, int, int, char*, int);
typedef int(__stdcall* funcpyPredictwModel)(double*, int, char*, int);

void test_Unique(funcUnique Unique) {
	double ex[] = { 1, 1, 2, 3, 4, 5., 5 };

	int size = Unique(ex, 7);

	if (size == 5 && ex[0] == 1 && ex[1] == 2 && ex[2] == 3 && ex[4] == 5)
		std::cout << "Passed - Test Unique" << std::endl;
	else
		std::cout << "Failed - Test Unique" << std::endl;
}


void test_pyTrainModel(funcpyTrainModel pyTrainModel){
	// xor example
	double X[] = { 0, 0, 1, 1, 0, 1, 1, 0};  // second dim = 2 
	int y[] = { 0, 0, 1, 1 }; // first dim = 4
	int xdim = 2;
	int ntrain = 4; 
	char *strmodel = (char*) malloc(1024*500); // 500 Kb
	int modelsize = pyTrainModel(X, y, 4, 2, strmodel, 1024 * 1500);
	if(modelsize > 0)
		std::cout << "Passed - Test pyTrainModel" << std::endl;
}

void test_pyTrainAndPredict(funcpyTrainModel pyTrainModel, funcpyPredictwModel pyPredictwModel) {
	// xor example
	double X[] = { 0, 0, 1, 1, 0, 1, 1, 0 };  // second dim = 2 
	int y[] = { 0, 0, 1, 1 }; // first dim = 4
	int ypred = -5;
	int xdim = 2;
	int ntrain = 4;
	char* strmodel = (char*) malloc(1024 * 500); // 500 Kb
	int modelsize = pyTrainModel(X, y, 4, 2, strmodel, 1024 * 1500);
	if (modelsize > 0)
		std::cout << "Passed - Test pyTrainModel" << std::endl;
	for (int i = 0; i < 3; i++) {		// what is the prediction for [0, 1] = should be 1
		ypred = pyPredictwModel(&X[4], 2, strmodel, modelsize);
		// std::cout << "y_pred: "<< ypred << std::endl;
		if(ypred == 1)
			std::cout << "Passed - Test pyPredictwModel" << std::endl;
	}
}

// Testing with LoadLibrary is more realistic for Mt5 

int main(void)
{
	HINSTANCE hinstDll;
	FARPROC ProcAdd = NULL;
	FARPROC ProcAdd_1 = NULL;

	for (int i = 0; i < 3; i++) { // call 3 times the same function loading and unloading the dll
		// Get a handle to the DLL module.
		hinstDll = LoadLibrary(TEXT("pythondlib.dll"));
		// If the handle is valid, try to get the function address.
		if (hinstDll != NULL) {
			ProcAdd = GetProcAddress(hinstDll, "Unique");
			if (ProcAdd != NULL) {
				test_Unique((funcUnique) ProcAdd);
			}
			FreeLibrary(hinstDll); // Free the DLL module.
		}		
	}

	for (int i = 0; i < 3; i++) { // call 3 times the same function loading and unloading the dll
	// Get a handle to the DLL module.
		hinstDll = LoadLibrary(TEXT("pythondlib.dll"));
		// If the handle is valid, try to get the function address.
		if (hinstDll != NULL) {
			ProcAdd = GetProcAddress(hinstDll, "pyTrainModel");
			ProcAdd_1 = GetProcAddress(hinstDll, "pyPredictwModel");
			if (ProcAdd != NULL) {
				test_pyTrainAndPredict((funcpyTrainModel) ProcAdd, (funcpyPredictwModel) ProcAdd_1);
			}
			FreeLibrary(hinstDll); // Free the DLL module.
		}
	}


	return 0;
}






