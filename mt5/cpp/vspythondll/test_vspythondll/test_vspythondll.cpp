#include <iostream>
#include "exports.h"

void test_Unique() {
	double ex[] = { 1, 1, 2, 3, 4, 5., 5 };

	int size = Unique(ex, 7);

	if (size == 5 && ex[0] == 1 && ex[1] == 2 && ex[2] == 3 && ex[4] == 5)
		std::cout << "Passed - Test Unique";
	else
		std::cout << "Failed   - Test Unique";
}

int main() {
	test_Unique();
	return 0;
}
