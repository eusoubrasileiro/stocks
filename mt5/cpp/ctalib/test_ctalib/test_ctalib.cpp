#include "exports.h"
#include <iostream>
#include <windows.h>

typedef int(__stdcall* funcTA_MA)(int, int, const double[], int, int, double[]);

void test_taMA(funcTA_MA taMA) {
	double in[] = { 1, 1, 2, 3, 4, 5., 5 };
	int window = 3;
	double out[10];	

	int size = taMA(0, 7, in, 3, 0, out);	// 0 simple moving average 1 EMA

	if (size == 5 && out[0] == 4./3 && out[size-1] == 14./3)
		std::cout << "Passed - Test TA_Ma" << std::endl;
	else
		std::cout << "Failed - Test TA_Ma" << std::endl;

	size = taMA(0, 7, in, 2, 0, out);

	if (size == 6 && out[0] == 1 && out[size - 1] == 5)
		std::cout << "Passed - Test TA_Ma" << std::endl;
	else
		std::cout << "Failed - Test TA_Ma" << std::endl;

}

// Testing with LoadLibrary is more realistic for Mt5 

int main(void)
{
	HINSTANCE hinstDll;
	FARPROC ProcAdd = NULL;
	FARPROC ProcAdd_1 = NULL;

	for (int i = 0; i < 3; i++) { // call 3 times the same function loading and unloading the dll
		// Get a handle to the DLL module.
		hinstDll = LoadLibrary(TEXT("ctalib.dll"));
		// If the handle is valid, try to get the function address.
		if (hinstDll != NULL) {
			ProcAdd = GetProcAddress(hinstDll, "taMA");
			if (ProcAdd != NULL) {
				test_taMA((funcTA_MA) ProcAdd);
			}
			FreeLibrary(hinstDll); // Free the DLL module.
		}		
	}

	return 0;
}






