
#include "params.h"
#include "cxlkvs.h"
#include "isa-l.h"

#ifndef CXLMEM
#define CXLMEM
#endif

using namespace std;

char* meta_data;
int meta_shmid;

int queue_size = 100; // 缓存队列的大小，最多缓存100个key

// 这里的一些全局参数可能后续需要放到cxlmem上
unsigned int encode_inc = 0;  //for random encoding
unsigned int stripe_count = 0;  // 条带计数

KVStore *kvs = nullptr;

void init_meta();

void destroy_meta();

char* get_unec_key(int thread_id);

void cache_unec_key(int thread_id, char* key);

bool check_encode(vector<char*>& keys_encode);

bool encode_store(vector<char*>& keys_encode);