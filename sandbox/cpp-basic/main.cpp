#include <iostream>

int main() {

	int x = 42;
	int* p = &x;

	std::cout << x << std::endl;
	std::cout << &x << std::endl;
	std::cout << p << std::endl;
	std::cout << *p << std::endl;

	*p = 99;

	std::cout << x << std::endl;

	return 0;
}