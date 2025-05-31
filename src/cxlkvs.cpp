

#include "cxlkvs.h"


KVStore::KVStore() {
    this->key_size = 4;
    this->value_size = 4096;

    this->entry_size = key_size + value_size + sizeof(char*);

    this->table_size = 1000;
    this->area_size = 1000;

    // cxlmem = (char*)malloc(this->entry_size * kvs_size);
    // memset(cxlmem, 0, this->entry_size * kvs_size);

    alloc_cxl_mem();

    init_free_area();
}

KVStore::KVStore(int key_size, int value_size, int table_size, int area_size) {
    this->key_size = key_size;
    this->value_size = value_size;

    this->entry_size = key_size + value_size + sizeof(char*);

    this->table_size = table_size;

    this->area_size = area_size;

    alloc_cxl_mem();

    init_free_area();
}

void KVStore::alloc_cxl_mem() {
    // 为hash_table分配内存
    size_t size = this->table_size * sizeof(char*);
    this->hash_table_shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0664);
    if (this->hash_table_shmid == -1) {
        perror("shmget failed");
        exit(-1);
    }

    this->hash_table = (char**)shmat(this->hash_table_shmid, NULL, 0);
    if (this->hash_table == (char**)-1) {
        perror("shmat failed");
        exit(-1);
    }
    unsigned long nodemask = (1UL << 1);
    if (mbind(this->hash_table, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
        perror("mbind failed");
        exit(-1);
    }
    memset(this->hash_table, 0, size);

    // 为free_area分配内存
    size = this->entry_size * this->area_size;
    this->free_area_shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0664);
    if (this->free_area_shmid == -1) {
        perror("shmget failed");
        exit(-1);
    }

    this->free_area = (char*)shmat(this->free_area_shmid, NULL, 0);
    if (this->free_area == (char*)-1) {
        perror("shmat failed");
        exit(-1);
    }

    if (mbind(this->free_area, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
        perror("mbind failed");
        exit(-1);
    }
    memset(this->free_area, 0, size);
}

// 初始化entry空闲区域
void KVStore::init_free_area() {
    free_block_list = free_area;
    char* cur = free_block_list;

    for (int i = 0; i < this->area_size - 1; i++) {
        *(char**)(cur + this->key_size + this->value_size) = cur + this->entry_size;
        cur += this->entry_size;
    }
}

char* KVStore::alloc_entry() {
    if (free_block_list == NULL) {
        cerr << "KVStore: free_block_list is NULL" << endl;
        exit(1);
        return NULL;  // 实验中要保证这种情况不能出现
    }

    // char* ret = free_block_list;
    // // free_block_list = *(char**)(ret + this->entry_size);
    // free_block_list = *(char**)(ret + this->key_size + this->value_size);
    // *(char**)(ret + this->key_size + this->value_size) = NULL;

    // return ret;

    char* ret;
    atomic<char*>* ptr = (atomic<char*>*)&free_block_list;
    atomic<char*> new_free_block_list;

    // 原子性地弹出free_block_list的第一个entry，防止多线程竞争
    do {
        ret = free_block_list;
        new_free_block_list.store(*(char**)(ret + this->key_size + this->value_size));
    } while (!ptr->compare_exchange_weak(ret, new_free_block_list));

    return ret;
}

void KVStore::free_entry(char* entry) {
    // *(char**)(entry + this->key_size + this->value_size) = free_block_list;
    // free_block_list = entry;

    char* tmp;
    atomic<char*>* ptr = (atomic<char*>*)&free_block_list;
    atomic<char*> new_free_block_list(entry);

    do {
        tmp = free_block_list;
        *(char**)(entry + this->key_size + this->value_size) = free_block_list;
    } while (!ptr->compare_exchange_weak(tmp, new_free_block_list));
}

int KVStore::hash_index(char* key) {
    string s(key);
    int index = hasher(s) % table_size;
    return index;
}

void KVStore::put(char* key, char* value) {
    int index = hash_index(key);
    char* entry = alloc_entry();
    memcpy(entry, key, key_size);
    memcpy(entry + key_size, value, value_size);

    // if (hash_table[index] == NULL) {
    //     hash_table[index] = entry;
    // } else {
    //     char* cur = hash_table[index];
    //     while (*(char**)(cur + this->key_size + this->value_size) != NULL) {
    //         cur = cur + this->key_size + this->value_size;
    //     }
    //     *(char**)(cur + this->key_size + this->value_size) = entry;
    // }
    // 上方的尾插法会存在竞争问题，下面改为头插法和cas

    atomic<char*>* ptr = (atomic<char*>*)&hash_table[index];
    atomic<char*> new_head(entry);
    char* tmp;

    do {
        tmp = hash_table[index];
        *(char**)(entry + this->key_size + this->value_size) = tmp;
    } while (!ptr->compare_exchange_weak(tmp, new_head));
}

// special为false时，从cxlkvs中获取key对应的value（char*），纯读取操作，要考虑竞争吗？假如一个在读一个在插入，头插法不会为此带来问题
// special为true时，为key分配其的value的内存，返回value（char*）
char* KVStore::get(char* key, bool special) {
    int index = hash_index(key);

    if (!special) {
        char* cur = hash_table[index];
        while (cur != NULL) {
            if (memcmp(cur, key, this->key_size) == 0) {
                return cur + this->key_size;
            }
            cur = *(char**)(cur + this->key_size + this->value_size);
        }
        return NULL;
    } else {
        // 暂时不进行条件判断
        char* entry = alloc_entry();
        memcpy(entry, key, this->key_size);

        // char* cur = hash_table[index];
        // while (*(char**)(cur + this->key_size + this->value_size) != NULL) {
        //     cur = cur + this->key_size + this->value_size;
        // }
        // *(char**)(cur + this->key_size + this->value_size) = entry;
        atomic<char*>* ptr = (atomic<char*>*)&hash_table[index];
        atomic<char*> new_head(entry);
        char* tmp;

        do {
            tmp = hash_table[index];
            *(char**)(entry + this->key_size + this->value_size) = tmp;
        } while (!ptr->compare_exchange_weak(tmp, new_head));

        return entry + this->key_size;
    }
}

void KVStore::del(char* key) {
}

KVStore::~KVStore() {
#ifndef CXLMEM
    free(hash_table);
    free(free_area);
#else
    if (shmctl(this->hash_table_shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(1);
    }
    if (shmctl(this->free_area_shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(1);
    }
#endif
}
