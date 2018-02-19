#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include "CacheStats.h"
using namespace std;

CacheStats::CacheStats() {
    cout << "Cache Config: ";
    if(!CACHE_EN) {
        cout << "cache disabled" << endl;
    } else {
        cout << (SETS * WAYS * BLOCKSIZE) << " B (";
        cout << BLOCKSIZE << " bytes/block, " << SETS << " sets, " << WAYS << " ways)" << endl;
        cout << "  Latencies: Lookup = " << LOOKUP_LATENCY << " cycles, ";
        cout << "Read = " << READ_MEM_LATENCY << " cycles, ";
        cout << "Write = " << WRITE_MEM_LATENCY << " cycles" << endl;
    }

    loads = 0;
    stores = 0;
    load_misses = 0;
    store_misses = 0;
    writebacks = 0;
    
    cacheTags = vector<uint32_t>(SETS * WAYS, 0);
    cacheDirtys = vector<bool>(SETS * WAYS, CLEAN);
    cacheValids = vector<bool>(SETS * WAYS, VALID);
    roundRobinStart = vector<uint8_t>(SETS, 0);
}

uint64_t CacheStats::access(uint32_t addr, ACCESS_TYPE type) {
    if(!CACHE_EN) { // no cache
        return (type == LOAD) ? READ_MEM_LATENCY : WRITE_MEM_LATENCY;
    }

    // Read Offset and Tag information of acessed data
    uint32_t block = (addr >> 5) & 0x07;
    uint32_t locus = block * WAYS;
    uint32_t tag = (addr >>  8);

    // Determine if hit
    bool hit = false;
    uint8_t i = 0;
    while (i < WAYS && !hit)
        (tag == cacheTags[locus + i])? hit = true : ++i;

    // Cache update on a hit
    if (hit) {
        if (type == LOAD) {
            ++loads;
            return LOOKUP_LATENCY;
        } else { // if STORE
            ++stores;
            cacheDirtys[locus + i] = DIRTY;
            return WRITE_CACHE_LATENCY;
        }
    } else { // if miss
        // Prepare the next appropriate cache locus according to
        // the Round Robin access policy 
        locus += roundRobinStart[block];
        roundRobinStart[block] = ( ++roundRobinStart[block] ) % WAYS;
        cacheTags[locus] = tag;

        // Cache update on a misses for LOADS and STORES
        if (type == LOAD){
            ++loads;
            ++load_misses;
            if (cacheDirtys[locus] == CLEAN) {
                cacheValids[locus] = VALID;
                return READ_MEM_LATENCY; // Simply load from memory
            } else { // if DIRTY
                ++writebacks;
                cacheDirtys[locus] = CLEAN;
                return WRITE_MEM_LATENCY  + READ_MEM_LATENCY; // Writeback then load from memory
            }
        } else { // if STORE
            ++stores;
            ++store_misses;
            if (cacheDirtys[locus] == CLEAN) {
                cacheDirtys[locus] = DIRTY;
                cacheValids[locus] = VALID;
                return READ_MEM_LATENCY + WRITE_CACHE_LATENCY; // Write allocate policy applies
            } else { // if DIRTY
                ++writebacks;
                return WRITE_MEM_LATENCY  + READ_MEM_LATENCY + WRITE_CACHE_LATENCY; // Write allocate policy applies
            }
        }
    }
}

void CacheStats::drainWriteBacks() {
    uint32_t drainedWB = 0;
    uint32_t numLoci = SETS * WAYS;
    for (uint32_t i = 0; i < numLoci; ++i)
        if (cacheDirtys[i] == DIRTY) ++drainedWB;
    writebacks += drainedWB;
}

void CacheStats::printFinalStats() {
    drainWriteBacks();
    int accesses = loads + stores;
    int misses = load_misses + store_misses;
    cout << "Accesses: " << accesses << endl;
    cout << "  Loads: " << loads << endl;
    cout << "  Stores: " << stores << endl;
    cout << "Misses: " << misses << endl;
    cout << "  Load misses: " << load_misses << endl;
    cout << "  Store misses: " << store_misses << endl;
    cout << "Writebacks: " << writebacks << endl;
    cout << "Hit Ratio: " << fixed << setprecision(1) << 100.0 * (accesses - misses) / accesses;
    cout << "%" << endl;
}
