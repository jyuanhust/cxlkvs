#pragma once

#include <numa.h>
#include <numaif.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <vector>

#ifndef CXLMEM
#define CXLMEM
#endif

using namespace std;

class KVStore {
   public:
    // 单位为字节
    int key_size;
    int value_size;

    int entry_size;

    // 一个entry由key、value和next组成，next为指针，指向下一个具有相同哈希值的entry

    int kvs_size;  // entry的数量

    hash<string> hasher;
    char** hash_table;  //
    int table_size;     // hash_table的大小

    int area_size;          // entry的数量
    char* free_area;        // 指向额外的空闲区域
    char* free_block_list;  // 指向空闲块列表

    int hash_table_shmid;
    int free_area_shmid;

   public:
    KVStore();

    KVStore(int key_size, int value_size, int table_size, int area_size);

    ~KVStore();

    void init_free_area();

    void alloc_cxl_mem();

    // 分配一个entry，
    // 一个entry由key、value和next组成，next为指针，指向下一个具有相同哈希值的entry
    // 多线程同时分配时会出现竞争
    char* alloc_entry();

    void free_entry(char* entry);

    int hash_index(char* key);

    void put(char* key, char* value);

    // special为false时，从cxlkvs中获取key对应的value（char*），纯读取操作，要考虑竞争吗？假如一个在读一个在插入，头插法不会为此带来问题
    // special为true时，为key分配其的value的内存，返回value（char*）
    char* get(char* key, bool special);

    void del(char* key);
};
