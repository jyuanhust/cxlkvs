#include <iostream>

using namespace std;

class MyClass {
    public:
    MyClass(int val) : value(val) {
        std::cout << "Constructor called. Value: " << value << std::endl;
    }

    ~MyClass() {
        std::cout << "Destructor called. Value: " << value << std::endl;
    }

    private:
    int value;
};


int main() {
    // 预先分配内存
    char buffer[sizeof(MyClass)];

    cout << "Size of MyClass: " << sizeof(MyClass) << endl;

    // 使用定位 new 在已有内存上构造对象
    MyClass* obj = new(buffer) MyClass(42);

    // 调用对象的方法
    obj->~MyClass();

    return 0;
}