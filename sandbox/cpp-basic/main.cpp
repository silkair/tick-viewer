#include <iostream>
#include <string>

class Tick {
public:
    Tick(std::string time, int price, int qty) {
        time_ = time;
        price_ = price;
        qty_ = qty;        
        std::cout << "Tick create " <<"\n";
    }

    ~Tick() {
        std::cout << "Tick delete\n";
    }

    Tick(const Tick& other) {
        time_ = other.time_;
        price_ = other.price_;
        qty_ = other.qty_;

        std::cout << "Tick copy\n";
    }

    void print() const {
        std::cout << time_ << " | " << price_ << " | " << qty_ << std::endl;
    }

    void setPrice(int p) {
        price_ = p;
    }

    private:
        std::string time_;
        int price_;
        int qty_;

};

void show(const Tick& tk) {
    tk.print();
}

void bump(Tick& tk) {
    tk.setPrice(88888);
}

int main() {
    Tick t("09:01:23", 71500, 12);
    t.print();

    Tick t2 = t;
    t2.print();

    t2.setPrice(9999);

    std::cout << "--- after change ---" << std::endl;
    t.print();
    t2.print();

    std::cout << "--- call show ---" << std::endl;
    show(t);
    std::cout << "--- show done ---" << std::endl;

    bump(t);
    t.print();

    return 0;
}






//
//class Tick {
//public:
//    Tick(std::string time, int price, int qty) {
//        time_ = time;
//        price_ = price;
//        qty_ = qty;
//
//        std::cout << "Tick create\n";
//    }
//
//
//    ~Tick() {
//    	std::cout << "Tick delete\n";
//    }
//
//    void print() {
//        std::cout << time_ << " | " << price_ << " | " << qty_ << std::endl;
//    }
//
//private:
//    std::string time_;
//    int price_;
//    int qty_;
//};
//
//int main() {
//    std::cout << "--- start ---\n";
//
//    // 1. 스택(Stack) 메모리에 객체 생성
//    Tick t("09:01:23", 71500, 12);
//    t.print();
//
//    // 2. 힙(Heap) 메모리에 동적 객체 생성
//    Tick* p = new Tick("09:01:24", 71600, 5);
//    p->print(); // (*p).print() 와 동일
//
//    std::cout << "--- end ---\n";
//
//    // 지시문에 따라 delete p; 를 의도적으로 생략함
//    return 0;
//}

//class Tick {
//public:
//	Tick() {
//		std::cout << "Tick create\n";
//	}
//
//	~Tick() {
//		std::cout << "Tick delete\n";
//	}
//};
//
//int main() {
//
//    std::cout << "--- before ---\n";
//
//    {
//        Tick* p = new Tick();
//		delete p;
//
//    }
//    std::cout << "--- after ---\n";
//
//
//    return 0;
//}

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

