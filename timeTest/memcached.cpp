/**
 * 测试将数据拷贝到memcached上的延迟
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <isa-l.h>
#include <iostream>
#include "libmemcached/memcached.h"
#include <sys/time.h>

using namespace std;

#define DATA_BLOCKS 5      // 原始数据块数
#define PARITY_BLOCKS 3    // 校验块数
#define BLOCK_SIZE 4096    // 每个数据块的大小（字节）

int main() {
    uint8_t *data_blocks[DATA_BLOCKS];
    uint8_t *parity_blocks[PARITY_BLOCKS];
    uint8_t g_tbls[32 * DATA_BLOCKS * PARITY_BLOCKS];  // 存储生成矩阵表
    int i;

    // 分配数据块和校验块
    for (i = 0; i < DATA_BLOCKS; i++) {
        data_blocks[i] = (uint8_t*)malloc(BLOCK_SIZE);
        memset(data_blocks[i], i + 1, BLOCK_SIZE);  // 填充数据
    }
    for (i = 0; i < PARITY_BLOCKS; i++) {
        parity_blocks[i] = (uint8_t*)malloc(BLOCK_SIZE);
        memset(parity_blocks[i], 0, BLOCK_SIZE);    // 初始化校验块为 0
    }

    // 初始化生成矩阵表
    uint8_t encode_matrix[DATA_BLOCKS * (DATA_BLOCKS + PARITY_BLOCKS)];
    gf_gen_rs_matrix(encode_matrix, DATA_BLOCKS + PARITY_BLOCKS, DATA_BLOCKS);
    ec_init_tables(DATA_BLOCKS, PARITY_BLOCKS, &encode_matrix[DATA_BLOCKS * DATA_BLOCKS], g_tbls);

    // 计时
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // RS 编码
    ec_encode_data(BLOCK_SIZE, DATA_BLOCKS, PARITY_BLOCKS, g_tbls, data_blocks, parity_blocks);

    gettimeofday(&end, NULL); // 获取结束时间

    double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    printf("编码时间: %f 秒\n", duration);
    // cout << "编码时间: " << duration << " 秒" << endl;


    // 编码完成，分发给存储节点
    char IP[100] = "127.0.0.1"; // node server
    memcached_st * memc = memcached_create(NULL);
    memcached_server_add(memc, IP, 11211);

    gettimeofday(&start, NULL);

    for (i = 0; i < DATA_BLOCKS; i++) {
        std::string key = "d" + std::to_string(i);
        memcached_set(memc, key.data(), key.length(), (const char *)data_blocks[i], BLOCK_SIZE, 0, 0);
    }

    for (i = 0; i < PARITY_BLOCKS; i++) {
        std::string key = "p" + std::to_string(i);
        memcached_set(memc, key.data(), key.length(), (const char *)parity_blocks[i], BLOCK_SIZE, 0, 0);
    }

    gettimeofday(&end, NULL); // 获取结束时间
    duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // cout << "存储时间: " << duration << " 秒" << endl;
    printf("存储时间: %f 秒\n", duration);

    // memcached_set(memc, key.data(), key.length(), value.data(), value.length(), 0, 0);
    memcached_free(memc);


    // printf("Encoding completed. Parity blocks:\n");
    // for (i = 0; i < PARITY_BLOCKS; i++) {
    //     printf("Parity block %d: ", i);
    //     for (int j = 0; j < 16; j++) {  // 只打印前 16 字节
    //         printf("%02x ", parity_blocks[i][j]);
    //     }
    //     printf("\n");
    // }


    // 释放内存
    for (i = 0; i < DATA_BLOCKS; i++) free(data_blocks[i]);
    for (i = 0; i < PARITY_BLOCKS; i++) free(parity_blocks[i]);

    cout << "Encoding completed." << endl;

    return 0;
}
