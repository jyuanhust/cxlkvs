
#include "meta.h"
#include "isa-l.h"

#ifndef CXLMEM
#define CXLMEM
#endif

using namespace std;

extern char* buffer_queue;
extern int queue_shmid;

extern int queue_size; // 缓存队列的大小，最多缓存100个key


void init_buffer_queue();

void destroy_buffer_queue();

char* get_unec_key(int thread_id);

void cache_unec_key(int thread_id, char* key);

bool check_encode(vector<char*>& keys_encode);

bool encode_store(vector<char*>& keys_encode, int* stripe_ID);