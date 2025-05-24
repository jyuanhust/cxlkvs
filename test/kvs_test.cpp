#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "meta.h"
#include "utils.h"

#include "timer.h"

#include <functional>  // std::hash

using namespace std;

int main() {
    cout << "kvs_test" << endl;

    key_size = 5;
    value_size = 10;
    table_size = 10;
    area_size = 20;

    kvs = new KVStore(key_size, value_size, table_size, area_size);

    // char* key = (char*)malloc(key_size);
    // memset(key, 0, key_size);
    // memset(key, 'a', key_size - 1);

    // char* value = kvs->get(key, true);
    // memset(value, 0, value_size);
    // memset(value, key[0], value_size - 2);

    // 下面的put操作是正常的
    // char* value = (char*)malloc(value_size);
    // memset(value, 0, value_size);
    // memset(value, key[0], value_size - 2);
    // kvs->put(key, value);

    // char* value2 = kvs->get(key, false);  // value2返回null
    // printf("get key: %s  value: %s\n", key, value2);

    int key_num = 6;
    vector<char*> keys;
    for (int i = 0; i < key_num; i++) {
        char* key = (char*)malloc(key_size);
        memset(key, 0, key_size);
        memset(key, 'a' + i, key_size - 1);
        keys.push_back(key);

        char* value = kvs->get(key, true);
        memset(value, 0, value_size);
        memset(value, key[0], value_size - 2);
    }

    cout << "succeed" << endl;

    for (int i = 0; i < key_num; i++) {
        char * key = keys[i];
        char* value = kvs->get(key, false);

        printf("%s %s\n",keys[i], value);
    }

    for(auto it = keys.begin(); it != keys.end(); it++){
        free(*it);
    }

    delete (kvs);
}