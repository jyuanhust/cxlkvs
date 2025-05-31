#include "meta.h"
#include <map>
#include "utils.h"
int nthreads = 8;  // 线程数量，也即内存节点服务器的数量

int key_size = 50;
int value_size = 100;

int table_size = 1000;  // 哈希表的大小（桶的数量），多个结构共用
int area_size = 1000;   // 空闲区域的entry的数量
int stripe_num = 100;   // 存储的最多条带数量

int kvs_size = 100;

int K = nthreads;  // 默认k为所有服务器的数量

int M = 2;  // 生成的校验块的数量，默认为2

int N = K + M;

// 这里的一些全局参数可能后续需要放到cxlmem上
// unsigned int encode_inc = 0;  // for random encoding
atomic<unsigned int> encode_inc{0}; // 考虑线程竞争，可能会导致++不正确

atomic<int> stripe_count;  // 条带计数，只会增加

KVStore* kvs;
ObjectIndex* obj_index;
StripeIndex* stripe_index;

// 存储 thread_id->keys 的map，即 服务器id -》 其所要处理的数据
map<int, vector<char*>> data_insert;
map<int, vector<Txn>> data_txn;

// 每个服务器的kvs，
map<int, map<string, char*>> local_kvs;

uint8_t* encode_matrix;
uint8_t* g_tbls;

// XOR
uint8_t encode_gftbl_xor[32 * 2 * 1];
uint8_t encode_matrix_xor[3 * 2];