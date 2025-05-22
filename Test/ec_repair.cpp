/**
 * 原更新代码示例
 */

#include <isa-l.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

typedef unsigned char u8;

int main() {
    // 添加判断出错的块个数，大于m时说明不可修复

    // order the failed fragment id

    int k = 4;
    int m = 2;
    int n = k + m;
    int len = 25;

    vector<int> erased = {1, 2};

    u8* data[k];
    u8* parity[m];
    u8* parityManual[m];
    u8* recover_srcs[k];  // 保存用于恢复的k个完好的块
    u8* recover_outp[m];

    for (int i = 0; i < k; i++) {
        if (NULL == (data[i] = (u8*)malloc(len))) {
            perror("data malloc error: Fail\n");
        }
    }

    for (int i = 0; i < m; i++) {
        if (NULL == (parity[i] = (u8*)malloc(len))) {
            perror("parity malloc error: Fail\n");
        }
        if (NULL == (parityManual[i] = (u8*)malloc(len))) {
            perror("parity malloc error: Fail\n");
        }
    }

    for (int i = 0; i < k; i++) {
        for (int j = 0; j < len; j++) {
            data[i][j] = 'a' + i;
        }
    }

    for (int i = 0; i < k; i++) {
        for (int j = 0; j < len; j++) {
            printf("%u ", data[i][k]);
        }
        cout << endl;
    }

    // Coefficient matrices
    u8 *encode_matrix, *decode_matrix;
    u8 *invert_matrix, *temp_matrix;
    u8* g_tbls;

    encode_matrix = (u8*)malloc(n * k);
    decode_matrix = (u8*)malloc(n * k);
    invert_matrix = (u8*)malloc(k * k);  // not
    temp_matrix = (u8*)malloc(k * k);
    g_tbls = (u8*)malloc(k * m * 32);    // if can be reused, set maximun size. recover m blocks mostly

    if (encode_matrix == NULL || decode_matrix == NULL || invert_matrix == NULL || g_tbls == NULL) {
        printf("Test failure! Error with malloc\n");
        return -1;
    }

    gf_gen_cauchy1_matrix(encode_matrix, n, k);
    ec_init_tables(k, m, &encode_matrix[k * k], g_tbls);
    ec_encode_data(len, k, m, g_tbls, data, parity);

    cout << "parity: " << endl;

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < len; j++) {
            printf("%u ", parity[i][k]);
        }
        cout << endl;
    }

    cout << "manual parity: " << endl;

    // calculate parity manually  手动计算校验块
    for (int i = 0; i < m; i++) {
        for (size_t l = 0; l < len; l++) {
            u8 s = 0;
            for (int j = 0; j < k; j++) {
                s ^= gf_mul(encode_matrix[k * (k + i) + j], data[j][l]);
            }
            // printf("%u ", s);
            parityManual[i][l] = s;
        }
        // cout << endl;
    }

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < len; j++) {
            printf("%u ", parityManual[i][j]);
        }
        cout << endl;
    }

    // recover
    sort(erased.begin(), erased.end());
    map<int, bool> block_in_err;
    for (int ele : erased) {
        block_in_err[ele] = true;
    }
    vector<int> decode_index;

    for (int i = 0, r = 0; i < k; i++, r++) {
        while (block_in_err[r])
            r++;

        for (int j = 0; j < k; j++) {
            temp_matrix[k * i + j] = encode_matrix[k * r + j];
        }
        decode_index.push_back(r);
        if (r < k)
            recover_srcs[i] = data[r];
        else
            recover_srcs[i] = parity[r - k];
    }

    if (gf_invert_matrix(temp_matrix, invert_matrix, k) < 0)
        return -1;

    int i = 0;
    for (int ele : erased) {
        if (ele < k) {
            for (int j = 0; j < k; j++) {
                decode_matrix[k * i + j] = invert_matrix[k * ele + j];
            }
        } else {
            for (int j = 0; j < k; j++) {
                u8 s = 0;
                for (int p = 0; p < k; p++) {
                    s ^= gf_mul(encode_matrix[k * ele + p], invert_matrix[p * k + j]);
                }

                decode_matrix[k * i + j] = s;
            }
        }
        i++;
    }
    // encode_matrix -> temp_matrix -> invert_matrix -> decode_matrix

    // Allocate buffers for recovered data，这里分配了m个
    for (i = 0; i < m; i++) {
        if (NULL == (recover_outp[i] = (u8*)malloc(len))) {
            printf("alloc error: Fail\n");
            return -1;
        }
    }

    ec_init_tables(k, erased.size(), decode_matrix, g_tbls);
    ec_encode_data(len, k, erased.size(), g_tbls, recover_srcs, recover_outp);

    for (int i = 0; i < erased.size(); i++) {
        if (erased[i] < k) {
            if (memcmp(recover_outp[i], data[erased[i]], len)) {
                printf(" Fail erasure recovery %d, data %d\n", i, erased[i]);
                return -1;
            }
        } else {
            if (memcmp(recover_outp[i], parity[erased[i] - k], len)) {
                printf(" Fail erasure recovery %d, parity %d\n", i, erased[i] - k);
                return -1;
            }
        }
    }

    for (size_t i = 0; i < erased.size(); i++) {
        cout << erased[i] << " ";
    }
    cout << endl;

    // although do not free the block, the sys can do it automatically

    for (int i = 0; i < k; i++) {
        free(data[i]);
    }

    for (int i = 0; i < m; i++) {
        free(parity[i]);
    }

    free(encode_matrix);

    free(decode_matrix);

    free(invert_matrix);
    free(g_tbls);
    // cout << "hello" << endl;
    return 0;
}