
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include "fred.h"

#include "meta.cpp"
#include "timer.h"

// #include "params.h"  // 加这一行代码就会出现重定义的错误

#include <functional>  // std::hash

#ifndef THREAD_PINNING
#define THREAD_PINNING
#endif

map<int, vector<char*>> data_insert;  // 存储 thread_id->keys 的map

std::hash<std::string> hash_fn;
int hash_to_range(const std::string& str, int range) {
    // 使用 std::hash 获取字符串的哈希值
    size_t hash_value = hash_fn(str);

    // 通过对哈希值取模，将哈希值限制在指定范围内
    return hash_value % range;
}

/**
 * node 0 cpus: 0 1 2 3 4 5 6 7 8 9 10 11
 * 24 25 26 27 28 29 30 31 32 33 34 35
 *
 * 根据task_id选取一个的cpu核心
 */
int get_core_id(int task_id) {
    int core_ids[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
    int num_cores = sizeof(core_ids) / sizeof(core_ids[0]);

    // 使用 task_id 对核心数量取模，返回对应核心 ID
    return core_ids[task_id % num_cores];
}

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

    // 这两个的返回值不同
    // printf("worker  %lu \n", pthread_self());
    // printf("worker  ::syscall(SYS_gettid)  %ld\n", ::syscall(SYS_gettid));

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

        vector<char*> keys_encode;
        if(check_encode(keys_encode)) {
            // 如果有足够的非空闲队列，则进行编码
            encode_store(keys_encode);

            // 存储条带元数据信息
            
        }
    }

    return NULL;
}


int main(int argc, char* argv[]) {
    // 初始化内存池中的数据
    cout << "hello" << endl;
    init_meta();
    destroy_meta();
    return 0;

    HL::Fred* threads;

    if (argc >= 2) {
        nthreads = atoi(argv[1]);
    }

    threads = new HL::Fred[nthreads];

    // 文本文件读取
    string path = "../data/ycsb_set.txt";

    ifstream file(path);

    if (!file.is_open()) {
        cerr << "无法打开文件!" << endl;
        return 1;
    }

    string line;

    while (getline(file, line)) {  // 逐行读取文件

        // INSERT	user6284781860667377211

        istringstream ss(line);
        string part;
        ss >> part;
        string key_str;
        if (ss >> key_str) {
            // std::cout << "提取的部分: " << key << std::endl;
            int key_hash = hash_to_range(key_str, nthreads);
            char* key = (char*)malloc(key_size * sizeof(char));
            memset(key, 0, key_size * sizeof(char));
            memcpy(key, key_str.c_str(), key_str.size());
            data_insert[key_hash].push_back(key);
        }
    }

    file.close();

    HL::Timer t;
    // Timer t;

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
}
