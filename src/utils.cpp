#include "utils.h"

hash<string> hash_fn;

int hash_to_range(const std::string& str, int range) {
    // 使用 std::hash 获取字符串的哈希值
    size_t hash_value = hash_fn(str);

    // 通过对哈希值取模，将哈希值限制在指定范围内
    return hash_value % range;
}

/**
 * node 0 cpus: 0 1 2 3 4 5 6 7 8 9 10 11
 * 24 25 26 27 28 29 30 31 32 33 34 35
 *
 * 根据task_id选取一个的cpu核心
 */
int get_core_id(int task_id) {
    int core_ids[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
        24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
    int num_cores = sizeof(core_ids) / sizeof(core_ids[0]);

    // 使用 task_id 对核心数量取模，返回对应核心 ID
    return core_ids[task_id % num_cores];
}

bool workload_load(map<int, vector<char*>>& data_insert, string path_load) {
    ifstream file_load(path_load);

    if (!file_load.is_open()) {
        cerr << "workload_load 无法打开文件!" << endl;
        return false;
    }

    string line;

    while (getline(file_load, line)) {  // 逐行读取文件

        // INSERT	user6284781860667377211

        istringstream ss(line);
        string part;
        ss >> part;
        string key_str;
        if (ss >> key_str) {
            // std::cout << "提取的部分: " << key_str << "长度为" << key_str.length() << std::endl;
            // key的长度
            int key_hash = hash_to_range(key_str, nthreads);
            char* key = (char*)malloc(key_size * sizeof(char));
            memset(key, 0, key_size * sizeof(char));
            memcpy(key, key_str.c_str(), key_str.size());
            data_insert[key_hash].push_back(key);
        }
    }

    file_load.close();
    return true;
}

bool workload_txn(map<int, vector<Txn>>& data_txn, string path_txn) {
    ifstream file_load(path_txn);

    if (!file_load.is_open()) {
        cerr << "workload_txn 无法打开文件!" << endl;
        return false;
    }

    string line;

    while (getline(file_load, line)) {  // 逐行读取文件

        // INSERT	user6284781860667377211

        istringstream ss(line);
        string op;
        ss >> op;
        string key_str;
        ss >> key_str;

        int key_hash = hash_to_range(key_str, nthreads);
        char* key = (char*)malloc(key_size * sizeof(char));
        memset(key, 0, key_size * sizeof(char));
        memcpy(key, key_str.c_str(), key_str.size());
        Txn txn;
        txn.key = key;
        // cout << op << " " << key_str << endl;

        if (op == "READ") {
            txn.op = 0;
        } else if(op == "UPDATE") {
            txn.op = 1;
        }

        data_txn[key_hash].push_back(txn);
    }

    file_load.close();

    return true;
}

void workload_print(map<int, vector<char*>>& data_insert, map<int, vector<Txn>>& data_txn){
    int j = 0;
    cout << "data_insert: " << endl;
    for (auto it = data_insert.begin(); it != data_insert.end(); it++) {
        cout << "thread " << j++ << ": " << it->second.size();
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            cout << *it2 << " ";
        }
        cout << endl;
    }
    cout << endl;

    j = 0;
    cout << "data_txn: " << endl;
    for (auto it = data_txn.begin(); it != data_txn.end(); it++) {
        cout << "thread " << j++ << ": size " << it->second.size() << " : ";
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            printf("%d %s ", it2->op, it2->key);
            // printf("%d ", it2->op);
        }
        cout << endl;
    }
}

/**
 * [ temperature | key0 | flag0 | key1 | flag1 | ... | keyN | flagN ]
 * temperature 用uint32存储，flag用uint8存储
 * 
 * 传入的num为条带数量
 */
void stripe_print(StripeIndex* stripe_index, int num){
    printf("there are %d stripes totally\n", num);
    char* ptr = stripe_index->free_area;
    for(int i = 0; i < num; i++){
        uint32_t temperature = *(uint32_t *)ptr;
        ptr = ptr + sizeof(uint32_t);
        printf("temperature: %d  keys: ", temperature);
        for(int j = 0; j < stripe_index->stripe_size; j++){
            char key[key_size];
            memcpy(key, ptr, key_size);
            ptr = ptr + key_size;
            uint8_t flag = *(uint8_t*)ptr;
            ptr = ptr + sizeof(uint8_t);
            printf("%d-%s  ", flag, key);
        }
        cout << endl;
    }
}