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
#include <algorithm>
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

    // 初始化生成矩阵表
    uint8_t encode_matrix[data_num * (data_num + parity_num)];
    uint8_t g_tbls[32 * data_num * parity_num];  // 存储生成矩阵表
    gf_gen_rs_matrix(encode_matrix, data_num + parity_num, data_num);
    ec_init_tables(data_num, parity_num, &encode_matrix[data_num * data_num], g_tbls);

    //XOR
    unsigned char encode_gftbl_xor[32 * 2 * 1];
    unsigned char encode_matrix_xor[3 * 2];

    gf_gen_rs_matrix(encode_matrix_xor, 3, 2);
    ec_init_tables(2, 1, &(encode_matrix_xor[2 * 2]), encode_gftbl_xor);

    // RS 编码
    ec_encode_data(block_size, data_num, parity_num, g_tbls, data_blocks, parity_blocks);

    // 查看数据块和校验块的内容
    cout << "Data blocks:" << endl;
    for (i = 0; i < DATA_BLOCKS; i++) {
        for (int j = 0; j < 16; j++) {  // 只打印前 16 字节
            printf("%02x ", data_blocks[i][j]);
        }
        printf("\n");
    }
    cout << "Parity blocks:" << endl;
    for (i = 0; i < PARITY_BLOCKS; i++) {
        for (int j = 0; j < 16; j++) {  // 只打印前 16 字节
            printf("%02x ", parity_blocks[i][j]);
        }
        printf("\n");
    }

    uint8_t* delta_data = (uint8_t*)malloc(block_size);
    uint8_t* delta_parity[parity_num];
    uint8_t* new_parity[parity_num];

    int index_wrong = 1; // 出错数据块的索引

    // 生成新的数据块
    uint8_t *new_data = (uint8_t*)malloc(block_size);
    memset(new_data, data_num + 1, block_size * sizeof(uint8_t));

    //data delta
    uint8_t* d[2];
    d[0] = data_blocks[index_wrong];
    d[1] = new_data;

    ec_encode_data(block_size, 2, 1, encode_gftbl_xor, d, &delta_data);

    //4. create parity deltas
    for (int j = 0; j < parity_num; j++)
    {
        delta_parity[j] = (uint8_t*)malloc(block_size);
        new_parity[j] = (uint8_t*)malloc(block_size);
    }

    ec_encode_data_update(block_size, data_num, parity_num, index_wrong, g_tbls, delta_data, delta_parity);

    for (int j = 0; j < parity_num; j++)
    {
        uint8_t* p[2];
        p[0] = parity_blocks[j];
        p[1] = delta_parity[j];

        ec_encode_data(block_size, 2, 1, encode_gftbl_xor, p, &(new_parity[j]));
    }
    // 至此，以增量块计算的形式得到新校验块

    cout << endl << "New parity blocks calculated by delta:" << endl;
    // 打印新校验块的内容
    for (i = 0; i < PARITY_BLOCKS; i++) {
        for (int j = 0; j < 16; j++) {  // 只打印前 16 字节
            printf("%02x ", new_parity[i][j]);
        }
        printf("\n");
    }

    // 以普通编码形式获得新检验快
    swap(data_blocks[index_wrong], new_data);
    ec_encode_data(block_size, data_num, parity_num, g_tbls, data_blocks, parity_blocks);

    cout << endl << "New parity blocks calculated by normal encoding:" << endl;
    // 打印新校验块的内容
    for (i = 0; i < PARITY_BLOCKS; i++) {
        for (int j = 0; j < 16; j++) {  // 只打印前 16 字节
            printf("%02x ", parity_blocks[i][j]);
        }
        printf("\n");
    }


    // 释放内存
    for (i = 0; i < data_num; i++)
        free(data_blocks[i]);
    for (i = 0; i < parity_num; i++)
        free(parity_blocks[i]);

    free(delta_data);
    for (i = 0; i < parity_num; i++){
        free(delta_parity[i]);
        free(new_parity[i]);
    }
    free(new_data);

    // memcached_set_index();


    return 0;
}
