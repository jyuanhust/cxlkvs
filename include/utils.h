#pragma once
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <vector>
#include "meta.h"
#include "string"
#include "timer.h"

using namespace std;

class Txn {
   public:
    uint8_t op;  // read = 0, update = 1
    char* key;
};

extern hash<string> hash_fn;

// 将字符串哈希到指定范围
int hash_to_range(const std::string& str, int range);

/**
 * node 0 cpus: 0 1 2 3 4 5 6 7 8 9 10 11
 * 24 25 26 27 28 29 30 31 32 33 34 35
 *
 * 根据task_id选取一个的cpu核心
 */
int get_core_id(int task_id);

// load数据集的读取
bool workload_load(map<int, vector<char*>>& data_insert, string path_load);

bool workload_txn(map<int, vector<Txn>>& data_txn, string path_txn);

void workload_print(map<int, vector<char*>>& data_insert, map<int, vector<Txn>>& data_txn);