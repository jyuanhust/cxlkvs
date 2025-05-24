/**
 * 实现线程通信的测试代码
 * 通过内存池中的存储key的队列
 *
 * client向队列中写入key，server在队列中读取key，然后将该key对应的value写入kvs
 * client再从kvs中读取value
 */

#include <fstream>
#include <map>
#include <sstream>
#include <string>
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

char* msg_queue;
int msg_queue_shmid;
int msg_queue_size = 10;

int client_id = 0;
int server_id = 1;

vector<char*> keys_to_send;
map<string, atomic<bool> > flags;
int keys_num = 10;  // 测试发送的key的数量

void init_msg_queue() {
    size_t size = (sizeof(atomic<uint32_t>) + key_size * msg_queue_size) * nthreads;

    msg_queue_shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0664);
    if (msg_queue_shmid == -1) {
        perror("shmget failed");
        exit(-1);
    }

    msg_queue = (char*)shmat(msg_queue_shmid, NULL, 0);
    if (msg_queue == (char*)-1) {
        perror("shmat failed");
        exit(-1);
    }
    unsigned long nodemask = (1UL << 1);
    if (mbind(msg_queue, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
        perror("mbind failed");
        exit(-1);
    }
    memset(msg_queue, 0, size);
}

void destroy_msg_queue() {
    if (shmctl(msg_queue_shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(1);
    }
}

/**
 * 向指定线程发送key
 * 本质为将key写入该thread对应的请求队列
 */
void send_msg_key(int thread_id, char* key) {
    atomic<uint32_t>* index_ptr = (atomic<uint32_t>*)(msg_queue + thread_id * (sizeof(atomic<uint32_t>) + key_size * msg_queue_size));
    char* queue_start = (char*)index_ptr + sizeof(atomic<uint32_t>);

    uint32_t index;
    atomic<uint32_t> new_index;

    do {
        // index为0表示队列为空
        index = index_ptr->load();

        if (index == msg_queue_size) {
            perror("queue is full");
        }

        memcpy(queue_start + index * key_size, key, key_size);

        new_index.store(index + 1);

    } while (!index_ptr->compare_exchange_weak(index, new_index));
}

// 从消息队列中取key，如果没有key则返回nullptr
char* get_msg_key(int thread_id) {
    atomic<uint32_t>* index_ptr = (atomic<uint32_t>*)(msg_queue + thread_id * (sizeof(atomic<uint32_t>) + key_size * msg_queue_size));
    char* queue_start = (char*)index_ptr + sizeof(atomic<uint32_t>);

    char* key = (char*)malloc(key_size);

    uint32_t index;
    atomic<uint32_t> new_index;

    do {
        // index为0表示队列为空
        index = index_ptr->load();

        if (index == 0) {
            free(key);
            return nullptr;
        }

        memcpy(key, queue_start + (index - 1) * key_size, key_size);

        new_index.store(index - 1);

    } while (!index_ptr->compare_exchange_weak(index, new_index));

    return key;
}

extern "C" void* client(void* arg) {
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

    // 发送请求
    for (size_t i = 0; i < keys_to_send.size(); i++) {
        send_msg_key(server_id, keys_to_send[i]);
        printf("request key: %s\n", keys_to_send[i]);
    }

    // 读取数据，轮询等待的方式
    for (size_t i = 0; i < keys_to_send.size(); i++) {
        char* key = keys_to_send[i];
        string key_str = key;

        // 循环等待为true
        while (!flags[key_str].load()) {
        }

        char* value = kvs->get(key, false);
        printf("get key: %s  value: %s\n", key, value);
    }

    return NULL;
}

extern "C" void* server(void* arg) {
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

    int count = 0;
    // 死循环监听接受数据
    while (1) {
        
        char* key = get_msg_key(server_id);

        printf("receive request key: %s\n", key);

        if (key == nullptr)
            continue;

        char* value = kvs->get(key, true);
        memset(value, 0, value_size);
        memset(value, key[0], value_size - 2);
        
        printf("value: %s\n", value);

        string key_str = key;
        flags[key_str].store(true);

        count++;
        if (count == keys_num)  // 搞完所有测试数据
            break;
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    cout << "thread_contact" << endl;
    key_size = 5;
    value_size = 10;
    table_size = 10;
    area_size = 30;

    // 全局变量的定义和初始化（内存池中的数据） meta.h
    kvs = new KVStore(key_size, value_size, table_size, area_size);

    init_msg_queue();

    for (int i = 0; i < keys_num; i++) {
        char* key = (char*)malloc(key_size);
        memset(key, 0, key_size);
        memset(key, 'a' + i, key_size - 2);

        keys_to_send.push_back(key);

        string key_str = key;
        flags[key_str].store(false);
    }

    HL::Fred* threads;
    nthreads = 2;
    threads = new HL::Fred[nthreads];

    int i;
    int* threadArg = (int*)malloc(nthreads * sizeof(int));  // task_id 数组
    for (i = 0; i < nthreads; i++) {
        threadArg[i] = i;
    }
    
    threads[0].create(client, &threadArg[0]);
    threads[1].create(server, &threadArg[1]);

    for (i = 0; i < nthreads; i++) {
        threads[i].join();
    }

    for(auto it = keys_to_send.begin(); it != keys_to_send.end(); it++){
        free(*it);
    }

    destroy_msg_queue();

    delete (kvs);
    delete[] threads;
}
