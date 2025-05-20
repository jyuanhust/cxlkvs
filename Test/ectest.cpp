// 完整的编码和解码过程

#include <iostream>
#include <cstring>
#include <vector>
#include <isa-l.h>

#define DATA_BLOCKS 5
#define PARITY_BLOCKS 3
#define TOTAL_BLOCKS (DATA_BLOCKS + PARITY_BLOCKS)
#define BLOCK_SIZE 1024  // 每个块的字节数

int main() {
    std::vector<uint8_t*> data(DATA_BLOCKS);
    std::vector<uint8_t*> parity(PARITY_BLOCKS);
    std::vector<uint8_t*> all_blocks(TOTAL_BLOCKS);

    // 初始化数据块
    for (int i = 0; i < DATA_BLOCKS; ++i) {
        data[i] = new uint8_t[BLOCK_SIZE];
        memset(data[i], i + 1, BLOCK_SIZE); // 简单填充测试数据
        all_blocks[i] = data[i];
    }

    // 初始化校验块
    for (int i = 0; i < PARITY_BLOCKS; ++i) {
        parity[i] = new uint8_t[BLOCK_SIZE];
        memset(parity[i], 0, BLOCK_SIZE);
        all_blocks[DATA_BLOCKS + i] = parity[i];
    }

    // 分配并生成编码矩阵
    uint8_t encode_matrix[TOTAL_BLOCKS * DATA_BLOCKS];
    uint8_t g_tbls[DATA_BLOCKS * PARITY_BLOCKS * 32];

    // 生成编码矩阵
    gf_gen_rs_matrix(encode_matrix, TOTAL_BLOCKS, DATA_BLOCKS);

    // 初始化Galois tables
    ec_init_tables(DATA_BLOCKS, PARITY_BLOCKS, &encode_matrix[DATA_BLOCKS * DATA_BLOCKS], g_tbls);

    // 编码
    ec_encode_data(BLOCK_SIZE, DATA_BLOCKS, PARITY_BLOCKS, g_tbls, data.data(), parity.data());

    std::cout << "Encoding complete.\n";

    // 模拟丢失两个数据块
    int erased_indexes[] = {1, 6}; // 比如丢失 data[1] 和 parity[1]（第6个块）
    all_blocks[1] = nullptr; // data[1]
    all_blocks[6] = nullptr; // parity[1]

    uint8_t* recover_blocks[2];
    recover_blocks[0] = new uint8_t[BLOCK_SIZE];
    recover_blocks[1] = new uint8_t[BLOCK_SIZE];

    uint8_t decode_matrix[DATA_BLOCKS * DATA_BLOCKS];
    uint8_t invert_matrix[DATA_BLOCKS * DATA_BLOCKS];
    uint8_t temp_matrix[TOTAL_BLOCKS * DATA_BLOCKS];
    uint8_t g_tbls_dec[32 * DATA_BLOCKS * 2];

    int valid_indexes[DATA_BLOCKS];
    int valid_idx = 0;
    for (int i = 0; i < TOTAL_BLOCKS && valid_idx < DATA_BLOCKS; ++i) {
        if (all_blocks[i] != nullptr) {
            valid_indexes[valid_idx++] = i;
        }
    }

    for (int i = 0; i < DATA_BLOCKS; ++i)
        memcpy(&temp_matrix[i * DATA_BLOCKS], &encode_matrix[valid_indexes[i] * DATA_BLOCKS], DATA_BLOCKS);

    gf_invert_matrix(temp_matrix, invert_matrix, DATA_BLOCKS);

    for (int i = 0; i < 2; ++i)
        memcpy(&decode_matrix[i * DATA_BLOCKS], &invert_matrix[erased_indexes[i] * DATA_BLOCKS], DATA_BLOCKS);

    ec_init_tables(DATA_BLOCKS, 2, decode_matrix, g_tbls_dec);
    std::vector<uint8_t*> available_blocks(DATA_BLOCKS);
    for (int i = 0; i < DATA_BLOCKS; ++i)
        available_blocks[i] = all_blocks[valid_indexes[i]];

    ec_encode_data(BLOCK_SIZE, DATA_BLOCKS, 2, g_tbls_dec, available_blocks.data(), recover_blocks);

    // 检查恢复是否成功（举例比较data[1]）
    if (memcmp(data[1], recover_blocks[0], BLOCK_SIZE) == 0) {
        std::cout << "Data recovery successful for block 1.\n";
    } else {
        std::cout << "Data recovery failed.\n";
    }

    // 清理
    for (auto ptr : data) delete[] ptr;
    for (auto ptr : parity) delete[] ptr;
    delete[] recover_blocks[0];
    delete[] recover_blocks[1];

    return 0;
}

