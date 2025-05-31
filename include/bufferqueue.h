#pragma once

#include "isa-l.h"
#include "meta.h"

using namespace std;

extern char* buffer_queue;
extern int queue_shmid;

extern int queue_size;  // 缓存队列的大小，最多缓存100个key

// 初始化缓存队列
void init_buffer_queue();

// 销毁缓存队列
void destroy_buffer_queue();

// 从指定线程/服务器的缓存队列中获取一个未编码的key
char* get_unec_key(int thread_id);

// 指定线程/服务器换成未编码的key
void cache_unec_key(int thread_id, char* key);

/**
 * 传入的keys_encode为空
 * 检查是否有足够的未编码key，若有则返回true，并且在keys_encode中存储未编码key
 */
bool check_encode(vector<char*>& keys_encode);

/**
 * keys_encode为未编码keys，stripe_ID为此条带的id
 * 将传入的keys_encode进行编码，并存储到cxlkvs中
 */
bool encode_store(vector<char*>& keys_encode, int stripe_id);