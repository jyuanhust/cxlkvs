/**
 * 在numa节点上分配内存，以数组形式保存 stripID -> temperature + (key + flag)s
 * temperature 用uint32存储，flag用uint8存储
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
#include <vector>
#include <cstdlib>
#include <cassert>

using namespace std;

class StripeIndex {
   public:
    int key_size;
    int stripe_num;   // 最大索引条带数
    int stripe_size;  // 每个条带的长度，设置为条带中数据块的数量

    char* free_area;  // 指向额外的空闲区域
    int free_area_shmid;

    StripeIndex(int key_size, int stripe_num, int stripe_size);

    void alloc_cxl_mem();

    void push_keys(uint32_t stripe_id, vector<char*> keys_encode);

    // 将指定条带的key置为无效
    // void invalidate_keys(uint32_t stripe_id) {
        
    // }

    ~StripeIndex();
};
