

#include "cxlkvs.h"


int main(int argc, char const* argv[]) {
    /* code */
    // KVStore kvs(4, 4, 4, 4);
    
    printf("%d  %d\n", sizeof(atomic<char*>), sizeof(char*));
    

    char key[4] = "123";
    char value[4] = "789";
    

    KVStore *kvs = new KVStore(4, 4, 4, 4);

    kvs->put(key, value);
    printf("%s\n", kvs->get(key, false));

    delete(kvs);
    return 0;

}