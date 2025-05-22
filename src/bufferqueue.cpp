#include "bufferqueue.h"

#include <iostream>

// #include "params.h" // 不能再，否则会出现重定义的错误
char* buffer_queue;
int queue_shmid;

int queue_size = 100;  // 缓存队列的大小，最多缓存100个key

void init_buffer_queue() {
    size_t size = (sizeof(atomic<uint32_t>) + key_size * queue_size) * nthreads;

    queue_shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0664);
    if (queue_shmid == -1) {
        perror("shmget failed");
        exit(-1);
    }

    buffer_queue = (char*)shmat(queue_shmid, NULL, 0);
    if (buffer_queue == (char*)-1) {
        perror("shmat failed");
        exit(-1);
    }
    unsigned long nodemask = (1UL << 1);
    if (mbind(buffer_queue, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
        perror("mbind failed");
        exit(-1);
    }
    memset(buffer_queue, 0, size);
}

void destroy_buffer_queue() {
    if (shmctl(queue_shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(1);
    }
}

// 通过thread_id/服务器id获取key（内存分配）
char* get_unec_key(int thread_id) {
    atomic<uint32_t>* index_ptr = (atomic<uint32_t>*)(buffer_queue + thread_id * (sizeof(atomic<uint32_t>) + key_size * queue_size));
    char* queue_start = (char*)index_ptr + sizeof(atomic<uint32_t>);

    char* key = (char*)malloc(key_size);

    uint32_t index;
    atomic<uint32_t> new_index;

    do {
        // index为0表示队列为空
        index = index_ptr->load();

        if (index == 0) {
            return nullptr;
        }

        memcpy(key, queue_start + (index - 1) * key_size, key_size);

        new_index.store(index - 1);

    } while (!index_ptr->compare_exchange_weak(index, new_index));

    return key;
}

// 每个服务器的用于缓存未编码的key值
void cache_unec_key(int thread_id, char* key) {
    atomic<uint32_t>* index_ptr = (atomic<uint32_t>*)(buffer_queue + thread_id * (sizeof(atomic<uint32_t>) + key_size * queue_size));
    char* queue_start = (char*)index_ptr + sizeof(atomic<uint32_t>);

    uint32_t index;
    atomic<uint32_t> new_index;

    do {
        // index为0表示队列为空
        index = index_ptr->load();

        if (index == queue_size) {
            perror("queue is full");
        }

        memcpy(queue_start + index * key_size, key, key_size);

        new_index.store(index + 1);

    } while (!index_ptr->compare_exchange_weak(index, new_index));
}

/**
 *
 */
bool check_encode(vector<char*>& keys_encode) {
    vector<int> threads_encode;  // 待编码的服务器id

    int count = 0;
    int inc1 = encode_inc;
    for (int i = 0; i < nthreads; i++) {
        atomic<uint32_t>* index_ptr = (atomic<uint32_t>*)(buffer_queue + (inc1 % nthreads) * (sizeof(atomic<uint32_t>) + key_size * queue_size));

        if (index_ptr->load() != 0) {
            threads_encode.push_back(inc1 % nthreads);
            count++;
            if (count == K) {
                break;
            }
        }

        inc1++;
    }
    // round_rabin the start tranverse point
    encode_inc++;

    if (count == K) {
        for (size_t i = 0; i < threads_encode.size(); i++) {
            char* key = get_unec_key(threads_encode[i]);
            if (key == nullptr) {
                // 删除之前获取的key的内存
                for (size_t j = 0; j < keys_encode.size(); j++) {
                    free(keys_encode[j]);
                }

                return false;
            }
            keys_encode.push_back(key);
        }
    }

    return true;
}

/**
 * 在cxlkvs中分配校验块的内存，对传入的key进行编码并存储
 * keys_encode为待编码的键
 * 为了获取到stripeID, 加上了一个参数用于写入中间的stripeID
 */
bool encode_store(vector<char*>& keys_encode, int stripe_id) {
    if (kvs == nullptr || keys_encode.size() != K)
        return false;

    unsigned char* data[K];
    unsigned char* parity[N - K];

    vector<char*> parity_keys;

    // 设置校验块的key
    for (int i = 0; i < N - K; i++) {
        char* parity_key = (char*)malloc(key_size);
        sprintf(parity_key, "SID%u-P%d", stripe_id, i);
        parity_keys.push_back(parity_key);
    }

    // 从cxlkvs中获取数据块和校验块的内存
    for (int i = 0; i < K; i++) {
        data[i] = (unsigned char*)kvs->get(keys_encode[i], false);
    }

    for (int i = 0; i < N - K; i++)
        parity[i] = (unsigned char*)kvs->get(parity_keys[i], true);

    // 编码（这里的一些数据需不需要变成全局变量）
    unsigned char encode_gftbl[32 * K * (N - K)];
    unsigned char encode_matrix[N * K];

    gf_gen_rs_matrix(encode_matrix, N, K);
    ec_init_tables(K, N - K, &(encode_matrix[K * K]), encode_gftbl);

    ec_encode_data(value_size, K, N - K, encode_gftbl, data, parity);

    return true;
}
