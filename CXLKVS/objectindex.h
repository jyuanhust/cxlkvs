/**
 * 在numa节点上分配内存，保存key -> stripID, offset 数据块的key到该key对应的条带id和块在条带中的偏移的映射
 */

#include <numa.h>
#include <numaif.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <atomic>
#include "params.h"

using namespace std;

// 每个entry由key(key_size字节) + stripe_id(4字节) + offset(4字节) + char*（8字节，指向下一个entry）

class ObjectIndex {
    public:
    // 单位为字节
    int key_size;

    int entry_size;

    char** hash_table;
    int table_size;

    int area_size;          // entry的数量
    char* free_area;        // 指向额外的空闲区域
    char* free_block_list;  // 指向空闲块列表

    int hash_table_shmid;
    int free_area_shmid;

    hash<string> hasher;

    ObjectIndex() {
        this->key_size = 4;
        this->entry_size = key_size + sizeof(int) * 2 + sizeof(char*);

        this->table_size = 1000;
        this->area_size = 1000;

        alloc_cxl_mem();

        init_free_area();
    }


    void alloc_cxl_mem() {
#ifndef CXLMEM
        this->hash_table = (char**)malloc(this->table_size * sizeof(char*));

        memset(hash_table, 0, this->table_size * sizeof(char*));

        free_area = (char*)malloc(this->entry_size * this->area_size);
        memset(free_area, 0, this->entry_size * this->area_size);
#else
        // 为hash_table分配内存
        size_t size = this->table_size * sizeof(char*);
        this->hash_table_shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0664);
        if (this->hash_table_shmid == -1) {
            perror("shmget failed");
            exit(-1);
        }

        this->hash_table = (char**)shmat(this->hash_table_shmid, NULL, 0);
        if (this->hash_table == (char**)-1) {
            perror("shmat failed");
            exit(-1);
        }
        unsigned long nodemask = (1UL << 1);
        if (mbind(this->hash_table, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
            perror("mbind failed");
            exit(-1);
        }
        memset(this->hash_table, 0, size);

        // 为free_area分配内存
        size = this->entry_size * this->area_size;
        this->free_area_shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0664);
        if (this->free_area_shmid == -1) {
            perror("shmget failed");
            exit(-1);
        }

        this->free_area = (char*)shmat(this->free_area_shmid, NULL, 0);
        if (this->free_area == (char*)-1) {
            perror("shmat failed");
            exit(-1);
        }

        if (mbind(this->free_area, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
            perror("mbind failed");
            exit(-1);
        }
        memset(this->free_area, 0, size);

#endif
    }

    // 每个entry由key(key_size字节) + stripe_id(4字节) + offset(4字节) + char*（8字节，指向下一个entry）
    void init_free_area() {
        free_block_list = free_area;
        char* cur = free_block_list;

        for (int i = 0; i < this->area_size - 1; i++) {
            *(char**)(cur + this->key_size + sizeof(int) * 2) = cur + this->entry_size;
            cur += this->entry_size;
        }
    }

    // 分配一个entry，
    // 每个entry由key(key_size字节) + stripe_id(4字节) + offset(4字节) + char*（8字节，指向下一个entry）
    // 多线程同时分配时会出现竞争
    char* alloc_entry() {
        if (free_block_list == NULL) {
            perror("free_block_list is NULL");
            return NULL;  // 实验中要保证这种情况不能出现
        }

        // char* ret = free_block_list;
        // free_block_list = *(char**)(ret + this->key_size + sizeof(int) * 2);
        // *(char**)(ret + this->key_size + sizeof(int) * 2) = NULL;
        // return ret;

        char* ret;
        atomic<char*>* ptr = (atomic<char*>*) & free_block_list;
        atomic<char*> new_free_block_list;

        do {
            ret = free_block_list;
            new_free_block_list.store(ret + this->key_size + sizeof(int) * 2);
        } while (!ptr->compare_exchange_weak(ret, new_free_block_list));

        return ret;
    }

    void free_entry(char* entry) {
        // *(char**)(entry + this->key_size + sizeof(int) * 2) = free_block_list;
        // free_block_list = entry;

        char* tmp;
        atomic<char*>* ptr = (atomic<char*>*)&free_block_list;
        atomic<char*> new_free_block_list(entry);

        do {
            tmp = free_block_list;
            *(char**)(entry + this->key_size + sizeof(int) * 2) = free_block_list;
        } while (!ptr->compare_exchange_weak(tmp, new_free_block_list));

    }

    int hash_index(char* key) {
        string s(key);
        int index = hasher(s) % table_size;
        return index;
    }

    void insert(char* key, int stripe_id, int offset) {
        char* entry = alloc_entry();
        memcpy(entry, key, this->key_size);
        *(int*)(entry + this->key_size) = stripe_id;
        *(int*)(entry + this->key_size + sizeof(int)) = offset;

        int index = hash_index(key);

        // if (hash_table[index] == NULL) {
        //     hash_table[index] = entry;
        // }
        // else {
        //     char* cur = hash_table[index];
        //     while (*(char**)(cur + this->key_size + sizeof(int) * 2) != NULL) {
        //         cur = cur + this->key_size + sizeof(int) * 2;
        //     }
        //     *(char**)(cur + this->key_size + sizeof(int) * 2) = entry;
        // }

        atomic<char*>* ptr = (atomic<char*>*)&hash_table[index];
        atomic<char*> new_head(entry);
        char* tmp;

        do {
            tmp = hash_table[index];
            *(char**)(entry + this->key_size + sizeof(int) * 2) = tmp;
        } while (!ptr->compare_exchange_weak(tmp, new_head));

    }

    void get_info(char* key, int& stripe_id, int& offset) {
        int index = hash_index(key);
        char* entry = hash_table[index];
        while (entry != NULL) {
            if (memcmp(entry, key, this->key_size) == 0) {
                stripe_id = *(int*)(entry + this->key_size);
                offset = *(int*)(entry + this->key_size + sizeof(int));
                return;
            }
            entry = *(char**)(entry + this->key_size + sizeof(int) * 2);
        }
    }
    ~ObjectIndex() {
#ifndef CXLMEM
        free(hash_table);
        free(free_area);
#else
        if (shmctl(this->hash_table_shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl failed");
            exit(1);
        }
        if (shmctl(this->free_area_shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl failed");
            exit(1);
        }
#endif
    }
};