

int main() {
    cout << "hello main" << endl;
    kvs = new KVStore(4, 4, 4, 4);
    char key[4] = "123";
    char value[4] = "789";

    kvs->put(key, value);
    printf("%s\n", kvs->get(key, false));
    // free(kvs);

    delete(kvs);  // new一个新对象时，手动调用delete才会自动调用类的析构函数

    return 0;
}