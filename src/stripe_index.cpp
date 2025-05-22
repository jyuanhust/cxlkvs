#include "stripe_index.h"

StripeIndex::StripeIndex(int key_size, int stripe_num, int stripe_size) {
    this->key_size = key_size;
    this->stripe_num = stripe_num;
    this->stripe_size = stripe_size;
    alloc_cxl_mem();
}

void StripeIndex::alloc_cxl_mem() {
    // 为free_area分配内存
    size_t size = (sizeof(uint32_t) + (key_size + sizeof(uint8_t)) * stripe_size) * stripe_num;
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
    unsigned long nodemask = (1UL << 1);

    if (mbind(this->free_area, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
        perror("mbind failed");
        exit(-1);
    }
    memset(this->free_area, 0, size);
}

void StripeIndex::push_keys(uint32_t stripe_id, vector<char*> keys_encode) {
    assert(keys_encode.size() == stripe_size);

    char* ptr = this->free_area + stripe_id * (sizeof(uint32_t) + (key_size + sizeof(uint8_t)) * stripe_size);
    ptr = ptr + sizeof(uint32_t);
    for (int i = 0; i < keys_encode.size(); i++) {
        memcpy(ptr, keys_encode[i], key_size);
        ptr = ptr + key_size;
        memset(ptr, 0, sizeof(uint8_t));
        ptr = ptr + sizeof(uint8_t);
    }
}

// 将指定条带的key置为无效
// void invalidate_keys(uint32_t stripe_id) {

// }

StripeIndex::~StripeIndex() {
    if (shmctl(this->free_area_shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(1);
    }
}