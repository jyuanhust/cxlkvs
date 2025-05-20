#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

std::mutex mtx;  // 全局互斥锁
int counter = 0;

char* mtx_area = nullptr;
mutex* mtx_ptr = nullptr;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        mtx.lock();        // 加锁
        ++counter;         // 临界区
        mtx.unlock();      // 解锁
    }
}

void increment2() {
    for (int i = 0; i < 100000; ++i) {
        mtx_ptr->lock();        // 加锁
        ++counter;         // 临界区
        mtx_ptr->unlock();      // 解锁
    }
}

int main() {
    mtx_area = (char*)malloc(sizeof(mutex));
    mtx_ptr = new (mtx_area) mutex;

    std::thread t1(increment2);
    std::thread t2(increment2);

    t1.join();
    t2.join();

    std::cout << "Counter: " << counter << std::endl;
    // printf("sizeof(mutex) %d\n", sizeof(mutex));
    return 0;
}
