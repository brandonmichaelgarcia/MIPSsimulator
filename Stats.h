#ifndef __STATS_H
#define __STATS_H
#include <iostream>
#include "Debug.h"
using namespace std;

enum PIPESTAGE { IF1 = 0, IF2 = 1, ID = 2, EXE1 = 3, EXE2 = 4, MEM1 = 5, 
                 MEM2 = 6, WB = 7, PIPESTAGES = 8 };

class Stats {
  private:
    long long cycles;
    uint64_t stalls;
    int flushes;
    int bubbles;

    int memops;
    int branches;
    int taken;

    int resultReg[PIPESTAGES];
    int resultAvailUpon[PIPESTAGES];
    int rawHazards[PIPESTAGES];

  public:
    Stats();

    void clock(short STAGE);

    void flush(short count);

    void registerSrc(int r, short stageNeeded);
    void registerDest(int r, short stageAvail);

    void countMemOp() { ++memops; }
    void countBranch() { ++branches; }
    void countTaken() { ++taken; }
    void stall(uint64_t numStalls);

    // getters
    long long getCycles() { return cycles; }
    int getStalls() { return stalls;}
    int getFlushes() { return flushes; }
    int getBubbles() { return bubbles; }
    int getMemOps() { return memops; }
    int getBranches() { return branches; }
    int getTaken() { return taken; }
    long long getTotalHazards();

  private:
    void bubble(short count);
};

#endif
