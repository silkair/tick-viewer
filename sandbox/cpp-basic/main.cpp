#include <iostream>

class Tick {
public:
	Tick() {
		std::cout << "Tick create\n";
	}

	~Tick() {
		std::cout << "Tick delete\n";
	}

};

int main() {

    std::cout << "--- before ---\n";

    {
        Tick* p = new Tick();
		delete p;

    }
    std::cout << "--- after ---\n";


    return 0;
}

//int main() {
//
//	int x = 42;
//	int* p = &x;
//
//	std::cout << x << std::endl;
//	std::cout << &x << std::endl;
//	std::cout << p << std::endl;
//	std::cout << *p << std::endl;
//
//	*p = 99;
//
//	std::cout << x << std::endl;
//
//
//
//	int* q = new int(7);
//	std::cout << *q << std::endl;
//
//	*q = 123;
//	std::cout << *q << std::endl;
//
//	delete q;
//	q = nullptr;
//
//	return 0;
//}

