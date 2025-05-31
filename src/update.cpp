/**
 * 条带更新方案
 */
#include "update.h"

/**
 * 就地更新，在insert后调用
 */
extern "C" void* inplace(void* arg) {
#ifdef THREAD_PINNING
    // 这里是要进入的
    int thread_id;
    int core_id;
    cpu_set_t cpuset;
    int set_result;
    int get_result;
    CPU_ZERO(&cpuset);
    thread_id = *(int*)arg;

    core_id = get_core_id(thread_id);

    CPU_SET(core_id, &cpuset);

    set_result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (set_result != 0) {
        fprintf(stderr, "setaffinity failed for thread %d to cpu %d\n", thread_id, core_id);
        exit(1);
    }
    get_result = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (set_result != 0) {
        fprintf(stderr, "getaffinity failed for thread %d to cpu %d\n", thread_id, core_id);
        exit(1);
    }
    if (!CPU_ISSET(core_id, &cpuset)) {
        fprintf(stderr, "WARNING: thread aiming for cpu %d is pinned elsewhere.\n", core_id);
    } else {
        // fprintf(stderr, "thread pinning on cpu %d succeeded.\n", core_id);
    }
#endif

    // 就地更新
    for (auto it = data_txn[thread_id].begin(); it != data_txn[thread_id].end(); it++) {
        if (it->op == 0) {
            continue;  // read, skip
        } else {
            char* key = it->key;
            string key_str = key;

            uint8_t* delta_data = (uint8_t*)malloc(value_size);  // 数据块增量

            uint8_t* delta_parity[M];  // 校验块增量

            uint32_t stripe_id;      // 更新的数据块所在的条带id
            uint32_t offset_update;  // 更新的数据块在条带中的索引
            obj_index->get_info(key, stripe_id, offset_update);

            // 生成新的数据块
            uint8_t* new_data = (uint8_t*)malloc(value_size);
            memset(new_data, 'u', value_size * sizeof(uint8_t));

            // data delta
            uint8_t* d[2];
            d[0] = (uint8_t*)local_kvs[thread_id][key_str];
            d[1] = new_data;

            ec_encode_data(value_size, 2, 1, encode_gftbl_xor, d, &delta_data);

            // create parity deltas
            for (int j = 0; j < M; j++) {
                delta_parity[j] = (uint8_t*)malloc(value_size);
            }

            ec_encode_data_update(value_size, K, M, offset_update, g_tbls, delta_data, delta_parity);

            // 从kvs中读取原校验块
            uint8_t* parity_blocks[M];

            char parity_key[key_size];
            for (int i = 0; i < N - K; i++) {
                memset(parity_key, 0, key_size);
                sprintf(parity_key, "SID%u-P%d", stripe_id, i);
                parity_blocks[i] = (uint8_t*)kvs->get(parity_key, false);
            }

            for (int j = 0; j < M; j++) {
                uint8_t* p[2];
                p[0] = parity_blocks[j];
                p[1] = delta_parity[j];

                // 直接原地更新旧校验块
                ec_encode_data(value_size, 2, 1, encode_gftbl_xor, p, &(parity_blocks[j]));
            }
            // 至此，以增量块计算的形式得到新校验块

            // 释放旧数据块的内存，用新数据块代替
            free(local_kvs[thread_id][key_str]);
            local_kvs[thread_id][key_str] = (char*)new_data;

            free(delta_data);
            for (int i = 0; i < M; i++) {
                free(delta_parity[i]);
            }
        }
    }

    return NULL;
}




















