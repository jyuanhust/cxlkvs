
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include "bufferqueue.h"
#include "fred.h"
#include "meta.h"
#include "utils.h"

#include "timer.h"

#include <functional>  // std::hash

#ifndef THREAD_PINNING
#define THREAD_PINNING
#endif

// 存储 thread_id->keys 的map，即 服务器id -》 其所要处理的数据
map<int, vector<char*>> data_insert;
map<int, vector<Txn>> data_txn;

pthread_barrier_t barrier;

/**
 * 多线程插入数据
 * 传入 task_id 的地址 task_id 即相当于线程id
 */
extern "C" void* insert(void* arg) {
#ifdef THREAD_PINNING
    // 这里是要进入的
    int task_id;
    int core_id;
    cpu_set_t cpuset;
    int set_result;
    int get_result;
    CPU_ZERO(&cpuset);
    task_id = *(int*)arg;

    core_id = get_core_id(task_id);

    core_id = 2;

    // std::cout << " core_id: " << core_id << std::endl;
    // core_id为绑定的cpu核心号，这里使用的是虚拟核心号，在本台机器上有48个虚拟核心

    CPU_SET(core_id, &cpuset);

    set_result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (set_result != 0) {
        fprintf(stderr, "setaffinity failed for thread %d to cpu %d\n", task_id, core_id);
        exit(1);
    }
    get_result = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (set_result != 0) {
        fprintf(stderr, "getaffinity failed for thread %d to cpu %d\n", task_id, core_id);
        exit(1);
    }
    if (!CPU_ISSET(core_id, &cpuset)) {
        fprintf(stderr, "WARNING: thread aiming for cpu %d is pinned elsewhere.\n", core_id);
    } else {
        // fprintf(stderr, "thread pinning on cpu %d succeeded.\n", core_id);
    }
#endif

    map<char*, char*> local_kvs;  // 服务器本地的kv存储

    // int queue_index = task_id;
    // char* queue_start = meta_data + sizeof(atomic<uint32_t>) + (sizeof(atomic<uint32_t>) + key_size * queue_size) * queue_index;

    // atomic<uint32_t>* count = (atomic<uint32_t>*)queue_start;
    // char* key_arr = queue_start + sizeof(atomic<uint32_t>);

    // 遍历要插入到本线程的key
    for (auto it = data_insert[task_id].begin(); it != data_insert[task_id].end(); it++) {
        // std::cout << "key: " << *it << std::endl;
        char* key = *it;
        local_kvs[key] = (char*)malloc(value_size * sizeof(char));  // 1kb
        // 这里分配内存会不会影响呢？
        // 应该就应该这样的，因为服务器必然要分配内存来缓存到来的数据
        memset(local_kvs[key], 0, value_size * sizeof(char));  // 模拟缓存数据

        // 空闲队列数和
        cache_unec_key(task_id, key);

        vector<char*> keys_encode;  // 存储要编码的key

        if (check_encode(keys_encode)) {
            // 如果有足够的非空闲队列，则进行编码

            // 多线程编码对stripe_count的竞争
            uint32_t stripe_id = stripe_count.fetch_add(1);  // 初始化为-1，先自增1，形成独占
            encode_store(keys_encode, stripe_id);

            // 存储条带元数据信息，object_index和stripe_index
            // 插入操作不设置热度
            for (uint32_t offset = 0; offset < keys_encode.size(); offset++) {
                obj_index->insert(keys_encode[offset], stripe_id, offset);
            }

            stripe_index->push_keys(stripe_id, keys_encode);
        }
    }

    // 至此，插入数据完成
    pthread_barrier_wait(&barrier);

    // 让所有线程等待全部数据都插入完成后再开始执行更新操作

    // 插入和更新能分离嘛？
    // 本地kvs的变量定义可能导致很难分离开

    return NULL;
}

int main(int argc, char* argv[]) {
    // 全局变量的定义和初始化（内存池中的数据） meta.h
    kvs = new KVStore(key_size, value_size, table_size, area_size);
    obj_index = new ObjectIndex(key_size, table_size, area_size);
    stripe_index = new StripeIndex(key_size, stripe_num, K);
    stripe_count.store(-1);

    init_buffer_queue();

    // 数据集读取，防止路径错误，使用绝对路径
    string path_load = "/home/hjy/Projects/ComputeTransfer/workloadGen/workloads_arrange/ycsb_load_workloada";
    string path_txn = "/home/hjy/Projects/ComputeTransfer/workloadGen/workloads_arrange/ycsb_txn_workloada";

    workload_load(data_insert, path_load);
    workload_txn(data_txn, path_txn);

    // 打印数据集内容
    // workload_print(data_insert, data_txn);
    
    pthread_barrier_init(&barrier,NULL,nthreads);

    delete (kvs);
    delete (obj_index);
    delete (stripe_index);
    destroy_buffer_queue();
    return 0;

    HL::Fred* threads;

    // if (argc >= 2) {
    //     nthreads = atoi(argv[1]);
    // }

    threads = new HL::Fred[nthreads];

    return 0;

    HL::Timer t;

    t.start();

    int i;
    int* threadArg = (int*)malloc(nthreads * sizeof(int));  // task_id 数组
    for (i = 0; i < nthreads; i++) {
        threadArg[i] = i;
        threads[i].create(insert, &threadArg[i]);
    }

    for (i = 0; i < nthreads; i++) {
        threads[i].join();
    }
    t.stop();

    printf("Time elapsed = %f\n", (double)t);  // 返回经过的时间，单位为：秒

    delete[] threads;

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
}
