// 系统全局参数

#ifndef CXLMEM
#define CXLMEM
#endif

int nthreads = 8;  // 线程数量，也即内存节点服务器的数量

int key_size = 4;
int value_size = 4;

int kvs_size = 100;

int K = nthreads; // 默认k为所有服务器的数量

int M = 2; // 生成的校验块的数量，默认为2

int N = K + M;