/**
 * 在numa节点上分配内存，保存key -> stripID, offset, temperature
 * 数据块的key 到 该key对应的条带id 和 块在条带中的偏移 和 温度 的映射
 *
 * 大小为：key: key_size
 *        stripe_id: uint32_t
 *        offset: uint32_t
 *        temperature: uint32_t
 */

#include <numa.h>
#include <numaif.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include "meta.h"


using namespace std;

// 每个entry由key(key_size字节) + stripe_id(4字节) + offset(4字节) + temperature(4字节) + char*（8字节，指向下一个entry）

class ObjectIndex {
   public:
    // 单位为字节
    int key_size;  // key的大小（字节）

    int entry_size;  // 每个entry的大小（字节）

    char** hash_table;
    int table_size;  // 哈希表的大小（桶的数量）

    int area_size;          // entry的数量
    char* free_area;        // 指向额外的空闲区域
    char* free_block_list;  // 指向空闲块列表

    int hash_table_shmid;
    int free_area_shmid;

    hash<string> hasher;

    ObjectIndex();

    ObjectIndex(int key_size, int table_size, int area_size);

    void alloc_cxl_mem();

    // 每个entry由key(key_size字节) + stripe_id(4字节) + offset(4字节) + temperature(4字节) + char*（8字节，指向下一个entry）
    void init_free_area();

    // 分配一个entry，
    // 每个entry由key(key_size字节) + stripe_id(4字节) + offset(4字节) + temperature(4字节) + char*（8字节，指向下一个entry）
    // 多线程同时分配时会出现竞争
    char* alloc_entry();

    void free_entry(char* entry);

    int hash_index(char* key);

    void insert(char* key, uint32_t stripe_id, uint32_t offset);

    void get_info(char* key, uint32_t& stripe_id, uint32_t& offset);

    ~ObjectIndex();
    
};