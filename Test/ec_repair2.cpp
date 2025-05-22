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
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
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
    uint8_t encode_matrix[(data_num + parity_num) * data_num];
    uint8_t g_tbls[32 * data_num * parity_num];  // 存储生成矩阵表
    gf_gen_rs_matrix(encode_matrix, data_num + parity_num, data_num);
    ec_init_tables(data_num, parity_num, &encode_matrix[data_num * data_num], g_tbls);

    // XOR
    uint8_t encode_gftbl_xor[32 * 2 * 1];
    uint8_t encode_matrix_xor[3 * 2];

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

    uint8_t* recover_srcs[data_num];    // 保存用于恢复的k个完好的块
    uint8_t* recover_outp[parity_num];  // 保存恢复后的块
    for (i = 0; i < parity_num; i++) {
        recover_outp[i] = (uint8_t*)malloc(block_size);
    }

    // 假如错的个数不是parity_num呢

    uint8_t temp_matrix[data_num * data_num] = {0};
    uint8_t invert_matrix[(data_num + parity_num) * data_num] = {0};
    uint8_t decode_matrix[(data_num + parity_num) * data_num] = {0};

    uint8_t decode_gftbl[32 * data_num * parity_num];

    vector<int> erased = {1, 2, 3};  // 出错块的索引
    map<int, bool> block_in_err;
    for (int ele : erased) {
        block_in_err[ele] = true;
    }

    for (int i = 0, r = 0; i < data_num; i++, r++) {
        while (block_in_err[r])
            r++;

        for (int j = 0; j < data_num; j++) {
            temp_matrix[data_num * i + j] = encode_matrix[data_num * r + j];
        }

        if (r < data_num)
            recover_srcs[i] = data_blocks[r];
        else
            recover_srcs[i] = parity_blocks[r - data_num];
    }

    if (gf_invert_matrix(temp_matrix, invert_matrix, data_num) < 0)
        return -1;

    i = 0;
    for (int ele : erased) {
        if (ele < data_num) {
            for (int j = 0; j < data_num; j++) {
                decode_matrix[data_num * i + j] = invert_matrix[data_num * ele + j];
            }
        } else {
            // 校验块出错
            for (int j = 0; j < data_num; j++) {
                uint8_t s = 0;
                for (int p = 0; p < data_num; p++) {
                    s ^= gf_mul(encode_matrix[data_num * ele + p], invert_matrix[p * data_num + j]);
                }

                decode_matrix[data_num * i + j] = s;
            }
        }
        i++;
    }

    ec_init_tables(data_num, erased.size(), decode_matrix, g_tbls);
    ec_encode_data(block_size, data_num, erased.size(), g_tbls, recover_srcs, recover_outp);

    // 查看恢复后的块的内容
    cout << "Recovered blocks:" << endl;
    for (i = 0; i < erased.size(); i++) {
        for (int j = 0; j < 16; j++) {  // 只打印前 16 字节
            printf("%02x ", recover_outp[i][j]);
        }
    }

    // 释放内存
    for (i = 0; i < data_num; i++)
        free(data_blocks[i]);
    for (i = 0; i < parity_num; i++)
        free(parity_blocks[i]);

    return 0;
}
