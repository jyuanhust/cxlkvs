#include <iostream>
#include <atomic>
#include <cstdio>
using namespace std;


int main(){
    int a = 10;
    atomic<int> * ptr = (atomic<int> *)&a;
    ptr->store(20);

    printf("%d\n", a);
}