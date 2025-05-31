
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include "bufferqueue.h"
#include "fred.h"
#include "meta.h"
#include "utils.h"

#include "timer.h"
#include "update.h"

#include <functional>  // std::hash

#ifndef THREAD_PINNING
#define THREAD_PINNING
#endif

pthread_barrier_t barrier;

/**
 * 多线程插入数据
 * 传入 thread_id 的地址 thread_i
 */
extern "C" void* insert(void* arg) {
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

    // 遍历要插入到本线程的key
    for (auto it = data_insert[thread_id].begin(); it != data_insert[thread_id].end(); it++) {
        // std::cout << "key: " << *it << std::endl;
        char* key = *it;

        // // 这里分配内存会不会影响呢？
        // // 应该就应该这样的，因为服务器必然要分配内存来缓存到来的数据
        // memset(local_kvs[key], 0, value_size * sizeof(char));  // 模拟缓存数据

        string key_str = *it;
        char* value = (char*)malloc(value_size * sizeof(char));
        if(value == NULL){
            cerr << "insert: malloc value error" << endl;
            exit(1);
        }


        local_kvs[thread_id][key_str] = value;
        memset(value, key[0], value_size * sizeof(char));  // 模拟缓存数据，设置为随机值

        // 将收到的数据块拷贝到cxlkvs上
        char* cxl_value = kvs->get(key, true);
        assert(cxl_value != nullptr);
        memcpy(cxl_value, local_kvs[thread_id][key_str], value_size);

        // 缓存key
        // cache_unec_key(thread_id, key);

        vector<char*> keys_encode;  // 存储要编码的key

        // if (check_encode(keys_encode)) {
        //     // 如果有足够的非空闲队列，则进行编码

        //     // for (size_t i = 0; i < keys_encode.size(); i++) {
        //     //     printf("%s ", keys_encode[i]);
        //     //     kvs->put(keys_encode[i], keys_encode[i]);  // 向cxlkvs插入kv
        //     // }
        //     // cout << "keys_encode.size: " << keys_encode.size() << endl
        //     //      << endl;

        //     // 多线程编码对stripe_count的竞争，fetch_add是先返回旧值再自增
        //     int32_t stripe_id = stripe_count.fetch_add(1);  // 初始化为-1，先自增1，形成独占，这里从 uint32_t 改成了 int32_t，可以过
        //     // cout << "stripe_id:" << stripe_id << endl;
        //     encode_store(keys_encode, stripe_id);
        //     // cout << "encode_store completed" << endl;

        //     // // 存储条带元数据信息，object_index和stripe_index
        //     // // 插入操作不设置热度
        //     for (uint32_t offset = 0; offset < keys_encode.size(); offset++) {
        //         obj_index->insert(keys_encode[offset], stripe_id, offset);
        //     }

        //     stripe_index->push_keys(stripe_id, keys_encode);
        // }
    }

    // 至此，插入数据完成

    // 让所有线程等待全部数据都插入完成后再开始执行更新操作
    // pthread_barrier_wait(&barrier);

    // 就地更新
    // for (auto it = data_txn[thread_id].begin(); it != data_txn[thread_id].end(); it++) {
    //     if (it->op == 0) {
    //         continue;  // read, skip
    //     } else {
    //     }
    // }

    // 插入和更新能分离嘛？
    // 本地kvs的变量定义可能导致很难分离开

    // 释放value占用的内存

    return NULL;
}

/**
 * 就地更新，在insert后调用
 */

int main(int argc, char* argv[]) {
    // if (argc >= 2) {
    //     nthreads = atoi(argv[1]);
    // }

    // 全局变量的定义和初始化（内存池中的数据） meta.h
    nthreads = 8;
    key_size = 30;
    value_size = 100;
    table_size = 1000;
    area_size = 10000;
    stripe_num = 1000;
    queue_size = 1000;

    // 编码相关
    K = 5;
    M = 2;
    N = K + M;

    kvs = new KVStore(key_size, value_size, table_size, area_size);
    obj_index = new ObjectIndex(key_size, table_size, area_size);
    stripe_index = new StripeIndex(key_size, stripe_num, K);
    stripe_count.store(0);

    init_buffer_queue();

    // 数据集读取，防止路径错误，使用绝对路径
    string path_load = "/home/hjy/Projects/ComputeTransfer/workloadGen/workloads_arrange/ycsb_load_workloada";
    string path_txn = "/home/hjy/Projects/ComputeTransfer/workloadGen/workloads_arrange/ycsb_txn_workloada";

    workload_load(data_insert, path_load);
    workload_txn(data_txn, path_txn);

    int data_insert_num = 0;
    for (auto it = data_insert.begin(); it != data_insert.end(); it++) {
        data_insert_num += it->second.size();
    }

    int data_txn_num = 0;
    for (auto it = data_txn.begin(); it != data_txn.end(); it++) {
        data_txn_num += it->second.size();
    }

    printf("data_insert_num:%d  data_txn_num:%d\n", data_insert_num, data_txn_num);

    // 打印数据集内容
    // workload_print(data_insert, data_txn);

    // pthread_barrier_init(&barrier, NULL, nthreads);

    encode_matrix = (uint8_t*)malloc(K * N);
    g_tbls = (uint8_t*)malloc(32 * K * M);

    gf_gen_rs_matrix(encode_matrix, N, K);
    ec_init_tables(K, M, &encode_matrix[K * K], g_tbls);

    gf_gen_rs_matrix(encode_matrix_xor, 3, 2);
    ec_init_tables(2, 1, &(encode_matrix_xor[2 * 2]), encode_gftbl_xor);

    HL::Fred* threads;
    HL::Timer t;
    threads = new HL::Fred[nthreads];

    int i;
    int* threadArg = (int*)malloc(nthreads * sizeof(int));  // task_id 数组

    cout << "start data insert" << endl;
    // 多线程插入数据
    for (i = 0; i < nthreads; i++) {
        threadArg[i] = i;
        threads[i].create(insert, &threadArg[i]);
    }

    for (i = 0; i < nthreads; i++) {
        threads[i].join();
    }

    cout << "数据插入完成" << endl;

    // stripe_print(stripe_index, stripe_count);
    // cout << "execute the inplace update" << endl;
    // t.start();
    // // 多线程执行就地更新
    // for (i = 0; i < nthreads; i++) {
    //     threadArg[i] = i;
    //     threads[i].create(inplace, &threadArg[i]);
    // }

    // for (i = 0; i < nthreads; i++) {
    //     threads[i].join();
    // }

    // t.stop();

    // printf("Time elapsed = %f\n", (double)t);  // 返回经过的时间，单位为：秒

    delete[] threads;

    delete (kvs);
    delete (obj_index);
    delete (stripe_index);
    destroy_buffer_queue();

    // 释放分配的key的内存
    for (auto it = data_insert.begin(); it != data_insert.end(); it++) {
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            free(*it2);
        }
    }
    for (auto it = data_txn.begin(); it != data_txn.end(); it++) {
        for (size_t i = 0; i < it->second.size(); i++) {
            free(it->second[i].key);
        }
    }

    // 释放分配的value的内存
    for (auto it = local_kvs.begin(); it != local_kvs.end(); it++) {
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            free(it2->second);
        }
    }

    free(encode_matrix);
    free(g_tbls);

    return 0;
}
