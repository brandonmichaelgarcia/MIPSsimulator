/******************************
 * Brandon Michael Garcia
 * CS 3339 - Spring 2017
 * Section 252
 * Project 2
 ******************************/
#include "CPU.h"

const string CPU::regNames[] = {"$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3",
                                "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
                                "$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
                                "$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra"};

CPU::CPU(uint32_t pc, Memory &iMem, Memory &dMem) : pc(pc), iMem(iMem), dMem(dMem) {
  for(int i = 0; i < NREGS; i++) {
    regFile[i] = 0;
  }
  hi = 0;
  lo = 0;
  regFile[28] = 0x10008000; // gp
  regFile[29] = 0x10000000 + dMem.getSize(); // sp

  instructions = 0;
  stop = false;
}

void CPU::run() {
  while(!stop) {
    instructions++;
    
    fetch();
    decode();
    execute();
    mem();
    writeback();
    
    stats.clock(IF1);  // increment the clock & pipeline

    D(printRegFile());
  }
}

void CPU::fetch() {
  instr = iMem.loadWord(pc);
  pc = pc + 4;
}

/////////////////////////////////////////
// ALL YOUR CHANGES GO IN THIS FUNCTION 
/////////////////////////////////////////
void CPU::decode() {
  uint32_t opcode;      // opcode field
  uint32_t rs, rt, rd;  // register specifiers
  uint32_t shamt;       // shift amount (R-type)
  uint32_t funct;       // funct field (R-type)
  uint32_t uimm;        // unsigned version of immediate (I-type)
  int32_t simm;         // signed version of immediate (I-type)
  uint32_t addr;        // jump address offset field (J-type)

  opcode = (instr >> 26); //highest 6 bits of instruction
  rs = (instr >> 21) & 0x1f; //bits 25 to 21 (R-type or I-type)
  rt = (instr >> 16) & 0x1f; //bits 20 to 16 (R-type or I-type)
  rd = (instr >> 11) & 0x1f; //bits 15 to 11 (R-type)
  shamt = (instr >> 6) & 0x1f;  //bits 10 to 6 (R-type)
  funct = (instr) &  0x3f;  //bits 5 to 0 (R-type)
  uimm = (instr) & 0xffff;  //bits 15 to 0 (I-type) // bits already have zero extension
  simm = ((signed)uimm << 16) >> 16;  //bits 15 to 0 (I-type) //Ensure proper sign extension
  addr = (instr) & 0x3ffffff;   //bits 25 to 0 (J-type)

// Setting all control signals to safe values
  opIsLoad = false;
  opIsStore = false;
  opIsMultDiv = false;
  aluOp = ADD;
  writeDest = false;
  destReg = 0;
  aluSrc1 = 0;
  aluSrc2 = 0;
  storeData = 0;
  

  D(cout << "  " << hex << setw(8) << pc - 4 << ": ");
  switch(opcode) {
    case 0x00:
      switch(funct) {
        case 0x00: D(cout << "sll " << regNames[rd] << ", " << regNames[rs] << ", " << dec << shamt);
                   writeDest = true; destReg = rd; stats.registerDest(destReg, PIPESTAGE::MEM1);
                   aluOp = SHF_L;
                   aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                   aluSrc2 = shamt;
                   break;
        case 0x03: D(cout << "sra " << regNames[rd] << ", " << regNames[rs] << ", " << dec << shamt);
                   writeDest = true; destReg = rd; stats.registerDest(destReg, PIPESTAGE::MEM1);
                   aluOp = SHF_R;
                   aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                   aluSrc2 = shamt;
                   break;
        case 0x08: D(cout << "jr " << regNames[rs]);
                   pc = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::ID);
                   stats.flush(2);
                   break;
        case 0x10: D(cout << "mfhi " << regNames[rd]);
                   writeDest = true; destReg = rd; stats.registerDest(destReg, PIPESTAGE::MEM1);
                   aluOp = ADD; // ADD to zero for move to hi
                   aluSrc1 = hi; stats.registerSrc(REG_HILO, PIPESTAGE::EXE1);
                   aluSrc2 = regFile[REG_ZERO];
                   break;
        case 0x12: D(cout << "mflo " << regNames[rd]);
                   writeDest = true; destReg = rd; stats.registerDest(destReg, PIPESTAGE::MEM1);
                   aluOp = ADD; // ADD to zero for move to lo
                   aluSrc1 = lo; stats.registerSrc(REG_HILO, PIPESTAGE::EXE1);
                   aluSrc2 = regFile[REG_ZERO];
                   break;
        case 0x18: D(cout << "mult " << regNames[rs] << ", " << regNames[rt]);
                   writeDest = true; destReg = REG_HILO;  stats.registerDest(destReg, PIPESTAGE::WB);
                   opIsMultDiv = true;
                   aluOp = MUL;
                   aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                   aluSrc2 = regFile[rt]; stats.registerSrc(rt, PIPESTAGE::EXE1);
                   break;
        case 0x1a: D(cout << "div " << regNames[rs] << ", " << regNames[rt]);
                   writeDest = true; destReg = REG_HILO;  stats.registerDest(destReg, PIPESTAGE::WB);
                   opIsMultDiv = true;
                   aluOp = DIV;
                   aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                   aluSrc2 = regFile[rt]; stats.registerSrc(rt, PIPESTAGE::EXE1);
                   break;
        case 0x21: D(cout << "addu " << regNames[rd] << ", " << regNames[rs] << ", " << regNames[rt]);
                   writeDest = true; destReg = rd; stats.registerDest(destReg, PIPESTAGE::MEM1);
                   aluOp = ADD;
                   aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                   aluSrc2 = regFile[rt]; stats.registerSrc(rt, PIPESTAGE::EXE1);
                   break;
        case 0x23: D(cout << "subu " << regNames[rd] << ", " << regNames[rs] << ", " << regNames[rt]);
                   writeDest = true; destReg = rd; stats.registerDest(destReg, PIPESTAGE::MEM1);
                   aluOp = ADD;
                   aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                   aluSrc2 = -(regFile[rt]); stats.registerSrc(rt, PIPESTAGE::EXE1);// negation allows for subtraction
                   break;
        case 0x2a: D(cout << "slt " << regNames[rd] << ", " << regNames[rs] << ", " << regNames[rt]);
                   writeDest = true; destReg = rd; stats.registerDest(destReg, PIPESTAGE::MEM1);
                   aluOp = CMP_LT;
                   aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                   aluSrc2 = regFile[rt]; stats.registerSrc(rt, PIPESTAGE::EXE1);
                   break;
        default: cerr << "unimplemented instruction: pc = 0x" << hex << pc - 4 << endl;
      }
      break;
    case 0x02: D(cout << "j " << hex << ((pc & 0xf0000000) | addr << 2)); // P1: pc + 4
               pc = (pc & 0xf0000000) | addr << 2; //unconditional jump of pc
               stats.flush(2);
               break;
    case 0x03: D(cout << "jal " << hex << ((pc & 0xf0000000) | addr << 2)); // P1: pc + 4
               writeDest = true; destReg = REG_RA;  stats.registerDest(destReg, PIPESTAGE::EXE1); // writes PC+4 to $ra
               aluOp = ADD; // ALU should pass pc thru unchanged
               aluSrc1 = pc;
               aluSrc2 = regFile[REG_ZERO]; // always reads zero
               pc = (pc & 0xf0000000) | addr << 2;
               stats.flush(2);
               break;
    case 0x04: D(cout << "beq " << regNames[rs] << ", " << regNames[rt] << ", " << pc + (simm << 2));
               stats.countBranch(); stats.registerSrc(rs, PIPESTAGE::ID); stats.registerSrc(rt, PIPESTAGE::ID);
               if ((signed)regFile[rs] == (signed)regFile[rt]){
                 pc = pc + (simm << 2); //conditional jump of pc
                 stats.countTaken();
                 stats.flush(2);
               } 
              
               break;
    case 0x05: D(cout << "bne " << regNames[rs] << ", " << regNames[rt] << ", " << pc + (simm << 2));
               stats.countBranch(); stats.registerSrc(rs, PIPESTAGE::ID); stats.registerSrc(rt, PIPESTAGE::ID);
               if ((signed)regFile[rs] != (signed)regFile[rt]){
                 pc = pc + (simm << 2); //conditional jump of pc
                 stats.countTaken();
                 stats.flush(2);
               } 
               break;
    case 0x09: D(cout << "addiu " << regNames[rt] << ", " << regNames[rs] << ", " << dec << simm);
               writeDest = true; destReg = rt;  stats.registerDest(destReg, PIPESTAGE::MEM1);
               aluOp = ADD;
               aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
               aluSrc2 = simm;
               break;
    case 0x0c: D(cout << "andi " << regNames[rt] << ", " << regNames[rs] << ", " << dec << uimm);
               writeDest = true; destReg = rt;  stats.registerDest(destReg, PIPESTAGE::MEM1);
               aluOp = AND;
               aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
               aluSrc2 = uimm;
               break;
    case 0x0f: D(cout << "lui " << regNames[rt] << ", " << dec << simm);
               writeDest = true; destReg = rt;  stats.registerDest(destReg, PIPESTAGE::MEM1);
               aluOp = SHF_L; // left shift in ALU puts lower half word of simm into the upper half word
               aluSrc1 = simm;
               aluSrc2 = 16;
               break;
    case 0x1a: D(cout << "trap " << hex << addr);
               switch(addr & 0xf) {
                 case 0x0: cout << endl; break;
                 case 0x1: cout << " " << (signed)regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
                           break;
                 case 0x5: cout << endl << "? "; cin >> regFile[rt];  stats.registerDest(destReg, PIPESTAGE::MEM1);
                           break;
                 case 0xa: stop = true; break;
                 default: cerr << "unimplemented trap: pc = 0x" << hex << pc - 4 << endl;
                          stop = true;
               }
               break;
    case 0x23: D(cout << "lw " << regNames[rt] << ", " << dec << simm << "(" << regNames[rs] << ")");
               opIsLoad = true; stats.countMemOp();
               writeDest = true; destReg = rt; stats.registerDest(destReg, PIPESTAGE::WB);
               aluOp = ADD; //add the offset to the address in register
               aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1);
               aluSrc2 = simm;
               addr = alu.op(aluOp, aluSrc1, aluSrc2); stats.stall( cacheStats.access(addr, ACCESS_TYPE::LOAD) );
               break;
    case 0x2b: D(cout << "sw " << regNames[rt] << ", " << dec << simm << "(" << regNames[rs] << ")");
               stats.countMemOp();
               opIsStore = true; storeData = regFile[rt];  //No writing to register
               aluOp = ADD; //add the offset to the address in register
               aluSrc1 = regFile[rs]; stats.registerSrc(rs, PIPESTAGE::EXE1); stats.registerSrc(rt, PIPESTAGE::MEM1);
               aluSrc2 = simm;
               addr = alu.op(aluOp, aluSrc1, aluSrc2); stats.stall( cacheStats.access(addr, ACCESS_TYPE::STORE) );
               break;
    default: cerr << "unimplemented instruction: pc = 0x" << hex << pc - 4 << endl;
  }
  D(cout << endl);
}

void CPU::execute() {
  aluOut = alu.op(aluOp, aluSrc1, aluSrc2);
}

void CPU::mem() {
  if(opIsLoad)
    writeData = dMem.loadWord(aluOut);
  else
    writeData = aluOut;
    
  if(opIsStore)
    dMem.storeWord(storeData, aluOut);
}

void CPU::writeback() {
  if(writeDest && destReg > 0) // never write to reg 0
    regFile[destReg] = writeData;
  
  if(opIsMultDiv) {
    hi = alu.getUpper();
    lo = alu.getLower();
  }
}

void CPU::printRegFile() {
  cout << hex;
  for(int i = 0; i < NREGS; i++) {
    cout << "    " << regNames[i];
    if(i > 0) cout << "  ";
    cout << ": " << setfill('0') << setw(8) << regFile[i];
    if( i == (NREGS - 1) || (i + 1) % 4 == 0 )
      cout << endl;
  }
  cout << "    hi   : " << setfill('0') << setw(8) << hi;
  cout << "    lo   : " << setfill('0') << setw(8) << lo;
  cout << dec << endl;
}

void CPU::printFinalStats() {
  cout << "Program finished at pc = 0x" << hex << pc << "  ("
       << dec << instructions << " instructions executed)" << endl;
  cout << endl;
  cout << "Cycles: " << stats.getCycles() << endl;
  cout << "CPI: " << fixed << setprecision(2) << static_cast<float>(stats.getCycles()) / static_cast<float>(instructions) << endl << endl;
  cout << "Bubbles: " << stats.getBubbles() << endl;
  cout << "Flushes: " << stats.getFlushes() << endl;
  cout << "Stalls: " << stats.getStalls() << endl;
  cout << endl;
  cacheStats.printFinalStats();
}
