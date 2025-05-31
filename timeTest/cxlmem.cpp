/**
 * 测试将数据拷贝到cxlmem上的延迟
 */
#include <isa-l.h>
#include <numa.h>
#include <numaif.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include "libmemcached/memcached.h"

using namespace std;

#define DATA_BLOCKS 5    // 原始数据块数
#define PARITY_BLOCKS 3  // 校验块数
#define BLOCK_SIZE 4096  // 每个数据块的大小（字节）

int data_num = 5;
int parity_num = 3;
int block_size = 4096;

int main(int argc, char* argv[]) {
    cout << "cxlmem.cpp" << endl;

    if (argc >= 2) {
        data_num = atoi(argv[1]);
    }

    if (argc >= 3) {
        parity_num = atoi(argv[2]);
    }

    if (argc >= 4) {
        block_size = atoi(argv[3]);
    }

    uint8_t* data_blocks[data_num];
    uint8_t* parity_blocks[parity_num];
    uint8_t g_tbls[32 * data_num * parity_num];  // 存储生成矩阵表
    int i;

    // 分配数据块和校验块
    for (i = 0; i < data_num; i++) {
        data_blocks[i] = (uint8_t*)malloc(block_size);
        memset(data_blocks[i], i + 1, block_size);  // 填充数据
    }
    for (i = 0; i < parity_num; i++) {
        parity_blocks[i] = (uint8_t*)malloc(block_size);
        memset(parity_blocks[i], 0, block_size);  // 初始化校验块为 0
    }

    // for (i = 0; i < DATA_BLOCKS; i++) {
    //     for (int j = 0; j < 16; j++) {  // 只打印前 16 字节
    //         printf("%02x ", data_blocks[i][j]);
    //     }
    //     printf("\n");
    // }

    // 初始化生成矩阵表
    uint8_t encode_matrix[data_num * (data_num + parity_num)];
    gf_gen_rs_matrix(encode_matrix, data_num + parity_num, data_num);
    ec_init_tables(data_num, parity_num, &encode_matrix[data_num * data_num], g_tbls);

    // 计时
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // RS 编码
    ec_encode_data(block_size, data_num, parity_num, g_tbls, data_blocks, parity_blocks);

    gettimeofday(&end, NULL);  // 获取结束时间

    double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    printf("编码时间: %f 秒\n", duration);
    // cout << "编码时间: " << duration << " 秒" << endl;

    // 编码完成，分发给存储节点
    size_t size = block_size * (data_num + parity_num);
    int shm_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0664);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(-1);
    }

    void* cxlmem = shmat(shm_id, NULL, 0);

    if (cxlmem == (void*)-1) {
        printf("shmat failed: %s\n", strerror(errno));
    }
    unsigned long nodemask = (1UL << 1);
    if (mbind(cxlmem, size, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
        perror("mbind failed");
        return 1;
    }

    // 存储到cxlmem
    gettimeofday(&start, NULL);

    for (i = 0; i < data_num; i++) {
        memcpy(((uint8_t*)cxlmem) + i * block_size, data_blocks[i], block_size);
    }
    for (i = 0; i < parity_num; i++) {
        memcpy(((uint8_t*)cxlmem) + (i + data_num) * block_size, parity_blocks[i], block_size);
    }

    gettimeofday(&end, NULL);  // 获取结束时间
    duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // cout << "存储时间: " <<   << " 秒" << endl;
    printf("存储时间: %f 秒\n", duration);

    // 解除共享内存映射
    // shmdt(cxlmem);

    // 删除共享内存段
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(1);
    }

    // 释放内存
    for (i = 0; i < data_num; i++)
        free(data_blocks[i]);
    for (i = 0; i < parity_num; i++)
        free(parity_blocks[i]);

    cout << "Encoding completed." << endl;

    return 0;
}
