# MIPS Simulator

To run the simulator, build the simulator using 'make simulator' and execute using 'simulator [mips file]', where [mips file] is the file name of a mips assembly file for execution on the simulator.


The MIPS simulator has a pipelined architecture, and the pipeline has the following stages: IF1, IF2, ID, EXE1, EXE2, MEM1, MEM2, WB. Branches are resolved in the ID stage, and there is no branch delay. There is also no load delay slot, an instruction that reads a register written by an immediately-preceding load receives the loaded value. There are no structural hazards, and data is written to the register file in WB in the first half of the clock cycle and can be read in ID in the second half of that same clock cycle.

Full data forwarding is simulated and so bubbles are only required for data hazards
that cannot be resolved by the addition of a forwarding path. In the case of such a data hazard, the
processor stalls the instruction with the read-after-write (RAW) hazard in the ID stage by inserting bubbles
for the minimum number of cycles until a forwarding path can get source data to the instruction. Static scoreboarding is used to do this; ID tracks the destination registers and cycles-until-available information for instructions
later in the pipeline, so that it can detect hazards and insert the correct number of bubbles.

The simulator also models pipeline stalls due to memory latency of a L1 data cache. The simulated data cache stores 1 KiB (1024 bytes) of data in block sizes of 8 words (32 bytes). It is 4-way set associative, with a round-robin replacement policy and a write policy of write-back write-allocate. For simplicity, the cache has no write buffer: a store must be completely finished before the processor can proceed. Since the simulated cache is a data cache, only loads and stores access it; instruction fetches are assumed to hit in a perfect I-cache with immediate access (i.e., there is never a stall for an instruction fetch).

The simulator also reports the following statistics at the end of the program:
* The exact number of clock cycles it would take to execute the program on this CPU
* The CPI (cycle count / instruction count)
* The number of bubble cycles injected due to data dependencies
* The number of flush cycles in the shadows of jumps and taken branches
* The number of stall cycles due to cache/memory latency
* The total number of accesses, plus the number that were loads vs. stores
* The total number of misses, plus the number caused by loads vs. stores
* The number of writebacks
* The hit ratio
