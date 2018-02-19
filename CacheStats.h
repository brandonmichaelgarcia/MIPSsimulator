#ifndef __CACHE_STATS_H
#define __CACHE_STATS_H

#include <cstdint>
#include "Debug.h"
#include <vector>

using namespace std;

#ifndef CACHE_EN
#define CACHE_EN 1
#endif

#ifndef BLOCKSIZE
#define BLOCKSIZE 32
#endif

#ifndef SETS
#define SETS 8
#endif

#ifndef WAYS
#define WAYS 4
#endif

#ifndef LOOKUP_LATENCY
#define LOOKUP_LATENCY 0
#endif

#ifndef WRITE_CACHE_LATENCY
#define WRITE_CACHE_LATENCY 0
#endif

#ifndef READ_MEM_LATENCY
#define READ_MEM_LATENCY 30
#endif

#ifndef WRITE_MEM_LATENCY
#define WRITE_MEM_LATENCY 10
#endif


enum ACCESS_TYPE { LOAD, STORE };

class CacheStats {
  private:
    enum VALIDITY_TAGS { INVALID, VALID };
    enum DIRTY_TAGS { CLEAN, DIRTY };
    vector<uint32_t> cacheTags;
    vector<bool> cacheDirtys;
    vector<bool> cacheValids;
    vector<uint8_t> roundRobinStart;

    int loads;
    int stores;
    int load_misses;
    int store_misses;
    int writebacks;

  public:
    CacheStats();
    uint64_t access(uint32_t, ACCESS_TYPE);
    void drainWriteBacks();
    void printFinalStats();
};

#endif
