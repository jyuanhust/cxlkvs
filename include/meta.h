// 系统全局参数
#pragma once
#include <atomic>
#include "cxlkvs.h"
#include "objectindex.h"
#include "stripe_index.h"
#include <map>
#include "utils.h"

using namespace std;

#ifndef CXLMEM
#define CXLMEM
#endif

#ifndef THREAD_PINNING
#define THREAD_PINNING
#endif

class Txn {
   public:
    uint8_t op;  // read = 0, update = 1
    char* key;
};

extern int nthreads;  // 线程数量，也即内存节点服务器的数量

extern int key_size;    // key的大小（字节）
extern int value_size;  // value的大小（字节）
extern int table_size;  // 哈希表的大小（桶的数量），多个结构共用
extern int area_size;   // 空闲区域的entry的数量
extern int stripe_num;   // stripe_index存储的最多条带数量


extern int kvs_size;

extern int K;  // 默认k为所有服务器的数量

extern int M;  // 生成的校验块的数量，默认为2

extern int N;

// 这里的一些全局参数可能后续需要放到cxlmem上
// extern unsigned int encode_inc;  // for random encoding
extern atomic<unsigned int> encode_inc;

extern atomic<int> stripe_count;  // 条带计数

extern KVStore* kvs;

extern ObjectIndex* obj_index;

extern StripeIndex* stripe_index;

// 存储 thread_id->keys 的map，即 服务器id -》 其所要处理的数据
extern map<int, vector<char*>> data_insert;
extern map<int, vector<Txn>> data_txn;

// 每个服务器的kvs，
extern map<int, map<string, char*>> local_kvs;

extern uint8_t* encode_matrix;
extern uint8_t* g_tbls;

// XOR
extern uint8_t encode_gftbl_xor[32 * 2 * 1];
extern uint8_t encode_matrix_xor[3 * 2];