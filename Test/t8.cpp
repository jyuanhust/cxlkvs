#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

std::mutex mtx;  // 全局互斥锁
int counter = 0;

#define CAS

void increment2() {
    atomic<int> * ptr = (atomic<int>*)&counter;
    
    for (int i = 0; i < 100000; ++i) {
        // 这里不用fetch_and_add，而是cas，是因为为了模拟其他类型的数据
        #ifdef CAS
        int temp;
        atomic<int> temp2;
        do{
            temp = ptr->load();
            temp2.store(temp + 1);

        }while(!ptr->compare_exchange_weak(temp, temp2));
        #else
        counter++;
        #endif
    }
}

int main() {

    // std::thread t1(increment2);
    // std::thread t2(increment2);

    // t1.join();
    // t2.join();

    // std::cout << "Counter: " << counter << std::endl;
    printf("sizeof(mutex) %d\n", sizeof(mutex));
    return 0;
}
