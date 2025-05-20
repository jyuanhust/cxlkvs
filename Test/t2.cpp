#include "libmemcached/memcached.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>

using namespace std;

//g++ -std=c++11 update.cpp -o update -lmemcached -lisal


char IP[100] = "127.0.0.1"; // node server


static double timeval_diff(struct timeval* start,
    struct timeval* end)
{
    double r = end->tv_sec - start->tv_sec;
    if (end->tv_usec > start->tv_usec)
        r += (end->tv_usec - start->tv_usec) / 1000000.0;
    else if (end->tv_usec < start->tv_usec)
        r -= (start->tv_usec - end->tv_usec) / 1000000.0;
    return r;
}

unsigned int N, K, SERVER, CHUNK_SIZE;

vector<memcached_st *> memc(N - K); //data +XOR,P2,P3

int main(){
    // memc[0] = memcached_create(NULL);
    // memc[1] = memcached_create(NULL);
    // memc[2] = memcached_create(NULL);
    // memc[3] = memcached_create(NULL);

    // memcached_return rc;

    // for(int i=0; i<SERVER;i++)
    // {
    //     memcached_server_add(memc[0], IP, 11211+i);
    // }

    // for(int i=0; i<N-K-1;i++)
    // {
    //     rc = memcached_server_add(memc[i+1], IP, 20000+i);
    // }

    // memcached_set(memc[j], p1, strlen(p1), pp[j].data(), CHUNK_SIZE, 0, 0);
    // char *old_data = memcached_get(memc[0], vrun[i].key.data(), vrun[i].key.length(), &val_len, &flags, &rc);
    // memcached_delete(memc[i], p1, strlen(p1), 0);
    // memcached_delete(memc[0], stripe_index[back][i].first.data(), stripe_index[back][i].first.length(), 0);


    memcached_st * mem = memcached_create(NULL);
    memcached_server_add(mem, IP, 11211);
    string key = "hello";
    string value = "world";
    memcached_set(mem, key.data(), key.length(), value.data(), value.length(), 0, 0);

    memcached_free(mem);
    return 0;

}