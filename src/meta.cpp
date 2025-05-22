#include "meta.h"

int nthreads = 8;  // 线程数量，也即内存节点服务器的数量

int key_size = 30;
int value_size = 50;

int kvs_size = 100;

int K = nthreads; // 默认k为所有服务器的数量

int M = 2; // 生成的校验块的数量，默认为2

int N = K + M;

// 这里的一些全局参数可能后续需要放到cxlmem上
unsigned int encode_inc = 0;  //for random encoding

atomic<int> stripe_count;  // 条带计数

KVStore *kvs;
