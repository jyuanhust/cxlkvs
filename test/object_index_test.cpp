#include <iostream>

#include "objectindex.h"


int main() {
    // std::cout << "__cplusplus = " << __cplusplus << std::endl;
    ObjectIndex *obj_index = new ObjectIndex(4, 1000, 1000);

    cout << "hello obj_index" << endl;

    delete(obj_index);
    return 0;
}