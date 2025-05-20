#include <iostream>
#include <atomic>
#include <thread>

std::atomic<uint32_t> counter(0);

// uint32_t counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        counter.fetch_add(1);
        // counter++;  // 循环次数足够大才会出现错误，否则很快就执行完了
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "Final counter: " << (int)counter.load() << std::endl;
    // printf("Final counter: %d\n", counter);
    return 0;
}





