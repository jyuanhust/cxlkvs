#include <iostream>
#include <string>
#include <cstring>
#include <functional>

using namespace std;

size_t hashToRange(const std::string& key, size_t range) {
    std::hash<std::string> hasher;
    return hasher(key) % range;
}

int main() {
    int key_size = 4, value_size = 4;
    int entry_size = key_size + value_size + sizeof(char*);
    int area_size = 100;
    char* free_area; // 指向额外的空闲区域
    char* free_block_list; // 指向空闲块列表
    free_area = (char*)malloc(entry_size * area_size);
    memset(free_area, 0, entry_size * area_size);

    free_block_list = free_area;
    char* cur = free_block_list;

    for (int i = 0; i < area_size - 1; i++) {
        *(char**)(cur + key_size + value_size) = cur + entry_size;
        cur += entry_size;
    }

    cur = free_block_list;
    while(cur != nullptr) {

        printf("%x\n", cur);
        cur = *(char**)(cur + key_size + value_size);
    }

    cout << entry_size << endl;

    return 0;
}
