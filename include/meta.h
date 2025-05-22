// 系统全局参数
#pragma once
#include <atomic>
#include "cxlkvs.h"

using namespace std;

#ifndef CXLMEM
#define CXLMEM
#endif

extern int nthreads;  // 线程数量，也即内存节点服务器的数量

extern int key_size;
extern int value_size;

extern int kvs_size;

extern int K; // 默认k为所有服务器的数量

extern int M; // 生成的校验块的数量，默认为2

extern int N;

// 这里的一些全局参数可能后续需要放到cxlmem上
extern unsigned int encode_inc;  //for random encoding

extern atomic<int> stripe_count;  // 条带计数

extern KVStore *kvs;