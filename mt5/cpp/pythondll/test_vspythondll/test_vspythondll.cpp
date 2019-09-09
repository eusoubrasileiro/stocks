#include <iostream>
#include "exports.h"

typedef int(__stdcall* myFunc)(double*, int);

void test_Unique(myFunc Unique) {
	double ex[] = { 1, 1, 2, 3, 4, 5., 5 };

	int size = Unique(ex, 7);

	if (size == 5 && ex[0] == 1 && ex[1] == 2 && ex[2] == 3 && ex[4] == 5)
		std::cout << "Passed - Test Unique" << std::endl;
	else
		std::cout << "Failed - Test Unique" << std::endl;
}

// Testing with LoadLibrary is more realistic for Mt5 

int LoadnCall() {
	HINSTANCE hinstDll;
	myFunc ProcAdd;

	// Get a handle to the DLL module.
	hinstDll = LoadLibrary(TEXT("pythondlib.dll"));
	//HMODULE pythondll = LoadLibrary(TEXT("vspythondll.dll"));
	// If the handle is valid, try to get the function address.
	if (hinstDll != NULL)
	{
		ProcAdd = (myFunc) GetProcAddress(hinstDll, "Unique");
		// If the function address is valid, call the function.
		if (NULL != ProcAdd)
		{
			test_Unique(ProcAdd);
		}
		// Free the DLL module.
		FreeLibrary(hinstDll);
	}

	// If unable to call the DLL function, use an alternative.
	//	if (!fRunTimeLinkSuccess)
	//		std::cout << "Message printed from executable\n";

	return 0;
}

int main(void)
{
	LoadnCall();
	// again
	LoadnCall();
	// again
	LoadnCall();
	return 0;
}






