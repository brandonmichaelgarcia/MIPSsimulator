/******************************
 * Brandon Michael Garcia
 * CS 3339 - Spr 2017
 ******************************/
#include "Stats.h"

Stats::Stats() {
    cycles = PIPESTAGES - 1; // pipeline startup cost
    flushes = 0;
    bubbles = 0;

    memops = 0;
    branches = 0;
    taken = 0;
    stalls = 0;

    for(int i = IF1; i < PIPESTAGES; i++) {
        resultReg[i] = -1;
        resultAvailUpon[i] = -1;
    }
}

void Stats::clock(short stage) { //Default parameter should be IF1
    cycles++;

    // pull the pipeline forward starting at STAGE
    for(int i = WB; i > stage; i--) {
        resultReg[i] = resultReg[i-1];
        resultAvailUpon[i] = resultAvailUpon[i-1];
    }
    // inject no-op into the start of the pipleine pull
    resultReg[stage] = -1;
    resultAvailUpon[stage] = -1;
}

void Stats::registerSrc(int r, short stageNeeded) {
    // Check if source register is one of previous resultReg
    // if so, inject appropriate number of bubbles
    // Then reset ID register
    short dependency = 0;
    for(short s = EXE1; s < WB; ++s){
        if (r == resultReg[s]){
            ++rawHazards[s];
            dependency = (resultAvailUpon[s] - s) - (stageNeeded - ID);
            break;
        }
    }
    if (dependency > 0)
    bubble(dependency);
}

long long Stats::getTotalHazards(){
    return rawHazards[EXE1] + rawHazards[EXE2] 
            + rawHazards[MEM1] + rawHazards[MEM2];
}

void Stats::registerDest(int r, short stageAvail) {
    // inject destination register into ID
    resultReg[ID] = r;
    // inject stageAvail into ID
    resultAvailUpon[ID] = stageAvail;
}

void Stats::flush(short count) { // count == how many ops to flush
    while (count > 0){
        clock(IF1);
        --count;
        ++flushes;
    }
}

void Stats::bubble(short count) {
    while (count > 0){
        clock(EXE1);
        --count;
        ++bubbles;
    }
}
  
void Stats::stall(uint64_t count) {
    stalls += count;
    cycles += count;
}

