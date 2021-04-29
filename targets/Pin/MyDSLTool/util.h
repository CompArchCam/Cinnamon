#ifndef UTIL_H
#define UTIL_H
#include <iostream>
#include <fstream>
#include <string>
#include "pin.H"
using namespace std;
enum{
   NOT_VALID=0,
   MEM,
   REGISTER,
   CONST
};
enum{
    UNKNOWN=0,
    LOAD,
    STORE,
    MOV,
    CALL,
    JMP,
    CJMP,
    LEA,
    RET,
    NOP,
    MUL,
    DIV,
    ADD,
    SUB

};
void writeToFile(fstream &file, uintptr_t val ){
        file<<val<<endl;
}
void writeToFile(fstream &file, std::string val ){
        file<<val<<endl;
}
void writeToFile(fstream &file, char* val ){
        file<<val<<endl;
}
void print(int x){
   cout<<x<<endl;
}
void print(std::string s){
   cout<<s<<endl;
}
void print(const char* str){
   cout<<str<<endl;
}
void print(uint64_t x){
   cout<<x<<endl;
}
void print(double d){
    cout<<d<<endl;
}
bool hasBasePointer(RTN F){
    for (INS I = RTN_InsHead(F); INS_Valid(I); I = INS_Next(I)){
        if(INS_Opcode(I) == XED_ICLASS_MOV && INS_OperandIsReg(I,0) && INS_OperandIsReg(I,1) && REG(INS_OperandReg(I, 0)) == REG_RBP && REG(INS_OperandReg(I,1)) == REG_RSP)
        {
            return true;
        }
    }
    return false;
}

uint32_t get_size(RTN F){//there is another one RTN_Size which tells the size in bytes
   return RTN_NumIns(F);
}
uint32_t get_size(BBL B){
   return BBL_Size(B);
}
std::string get_trgname(INS I){
   if(!INS_IsDirectControlFlow(I)) return "NULL";
   ADDRINT addr = INS_DirectControlFlowTargetAddress(I);
   //ADDRINT addr = INS_DirectBranchOrCallTargetAddress(I);//deprecated
   return RTN_FindNameByAddress(addr);
}
ADDRINT get_falladdr(INS I){
   return INS_NextAddress(I);
}
ADDRINT get_startAddr(RTN F){
   return RTN_Address(F);
}
ADDRINT get_addr(INS I){
   return INS_Address(I);
}
ADDRINT get_nextaddr(INS I){
   return INS_NextAddress(I);
}
ADDRINT get_addr(RTN &F){
   return RTN_Address(F);
}
std::string get_name(RTN F){
   return RTN_Name(F);
}
ADDRINT get_trgaddr(INS I){
   return INS_DirectControlFlowTargetAddress(I); //deprecated
   //return INS_DirectBranchOrCallTargetAddress(I); //deprecated
}
uint32_t get_size(INS I){
   return INS_Size(I);
}
uint64_t get_stackFrameSize(RTN F){
    uint64_t value = 0;
    for (INS I = RTN_InsHead(F); INS_Valid(I); I = INS_Next(I)){
        if((INS_Opcode(I) == XED_ICLASS_SUB) && REG(INS_OperandReg(I, 0)) == REG_STACK_PTR && INS_OperandIsImmediate(I, 1))
        {
            value = INS_OperandImmediate(I, 1); 
        }
    }
    return value;
}
REG get_base(INS I){
   REG base_reg = REG_INVALID();
   for (unsigned int i=0; i< INS_OperandCount(I); i++)
   {
        if (((INS_OperandIsMemory (I, i) && INS_OperandRead (I, i)) ||
             (INS_OperandIsMemory (I, i) && INS_OperandWritten (I, i)))
           )
        {
            //displacement  = INS_OperandMemoryDisplacement (ins, i);
            base_reg = INS_OperandMemoryBaseReg (I, i);
            //indexReg = INS_OperandMemoryIndexReg (ins, i);
            //scale = INS_OperandMemoryScale (ins, i);
            break;
        }
    }
    return base_reg;
} 
int get_id(REG reg){
   int id = -1;
   if(reg!= REG_INVALID()){
      
   }
   return id;
}
int get_type_opnd(INS I, uint32_t n){
    if(INS_OperandCount(I) <= n) // index is from 0:count-1
        return NOT_VALID;
    if(INS_OperandIsMemory(I,n))
        return MEM;
    if(INS_OperandIsReg(I, n))
        return REGISTER;
    if(INS_OperandIsImmediate(I, n))
        return CONST;
    return NOT_VALID;
}
int get_type_srcopnd(INS I, uint32_t n){
    std::vector<int> src_index;
    if(INS_OperandCount(I) <= n) // index is from 0:count-1
        return NOT_VALID;
    uint32_t count = 0;
    for(uint32_t i = 0 ; i< INS_OperandCount(I); i++){
       if(INS_OperandReadOnly(I, i))
           src_index[count++] = i; //do I need to get the index too??
    }
    if(n > count)
        return NOT_VALID;
    if(INS_OperandIsMemory(I,src_index[n-1])) //src1 => src 0??  
        return MEM;
    if(INS_OperandIsReg(I, src_index[n-1]))
        return REGISTER;
    if(INS_OperandIsImmediate(I, src_index[n-1]))
        return CONST;
    return NOT_VALID;
}
bool check_type_srcopnd(INS I, uint32_t n, int type){
    std::vector<int> src_index;
    if(INS_OperandCount(I) <= n) // index is from 0:count-1
        return NOT_VALID;
    uint32_t count = 0;
    for(uint32_t i = 0 ; i< INS_OperandCount(I); i++){
       if(INS_OperandRead(I, i))
           src_index[count++] = i; //do I need to get the index too??
    }
    if(n > count)
        return false;
    switch(type){
       case MEM:
          if(INS_OperandIsMemory(I,src_index[n]))
              return true;
       case REGISTER: 
        if(INS_OperandIsReg(I, src_index[n]))
        return true;
       case CONST:
            if(INS_OperandIsImmediate(I, src_index[n]))
            return true;
    }
    return false;
}
bool check_type_destopnd(INS I, int type){
    int dest_index = -1;
    for(uint32_t i = 0 ; i< INS_OperandCount(I); i++){
       if(INS_OperandWritten(I, i)){ // how about read and written
           dest_index = i; //do I need to get the index too??
       }
    }
    switch(type){
       case MEM:
          if(INS_OperandIsMemory(I,dest_index))
              return true;
       case REGISTER: 
        if(INS_OperandIsReg(I, dest_index))
        return true;
       case CONST:
            if(INS_OperandIsImmediate(I, dest_index))
            return true;
    }
    return false;

}
REG get_srcopnd(INS I, uint32_t n){
    REG src_reg = REG_INVALID();
    uint32_t count=0;
    if(INS_OperandCount(I) <= n) // index is from 0:count-1
        return src_reg;
    for(uint32_t i = 0 ; i< INS_OperandCount(I); i++){
       if(INS_OperandRead(I, i)){
           if(count++ == n){
               src_reg = INS_OperandReg(I,i); 
           }
       }
    }
    return src_reg;
}
REG get_memopnd_base(INS I){
    REG base = REG_INVALID();
    for(unsigned int i=0; i< INS_MemoryOperandCount(I); i++){
        base = INS_OperandMemoryBaseReg(I, i); //assuming only one memory operand
        break;
    }
    return base;
}

int get_opcode(INS I){
   int opcode = UNKNOWN;
   switch(INS_Opcode(I)){
       case   XED_ICLASS_MOV: 
       case   XED_ICLASS_MOVAPD: 
       case   XED_ICLASS_MOVAPS: 
       case   XED_ICLASS_MOVBE:
       case   XED_ICLASS_MOVD: 
       case   XED_ICLASS_MOVDDUP: 
       case   XED_ICLASS_MOVDIR64B: 
       case   XED_ICLASS_MOVDIRI: 
       case   XED_ICLASS_MOVDQ2Q: 
       case   XED_ICLASS_MOVDQA:
       case   XED_ICLASS_MOVDQU: 
       case   XED_ICLASS_MOVHLPS: 
       case   XED_ICLASS_MOVHPD:
       case   XED_ICLASS_MOVHPS: 
       case   XED_ICLASS_MOVLHPS: 
       case   XED_ICLASS_MOVLPD:
       case   XED_ICLASS_MOVLPS: 
       case   XED_ICLASS_MOVMSKPD: 
       case   XED_ICLASS_MOVMSKPS: 
       case   XED_ICLASS_MOVNTDQ:
       case   XED_ICLASS_MOVNTDQA: 
       case   XED_ICLASS_MOVNTI:
       case   XED_ICLASS_MOVNTPD: 
       case   XED_ICLASS_MOVNTPS: 
       case   XED_ICLASS_MOVNTQ:
       case   XED_ICLASS_MOVNTSD: 
       case   XED_ICLASS_MOVNTSS: 
       case   XED_ICLASS_MOVQ:
       case   XED_ICLASS_MOVQ2DQ: 
       case   XED_ICLASS_MOVSB:
       case   XED_ICLASS_MOVSD: 
       case   XED_ICLASS_MOVSD_XMM: 
       case   XED_ICLASS_MOVSHDUP:
       case   XED_ICLASS_MOVSLDUP:
       case   XED_ICLASS_MOVSQ:
       case   XED_ICLASS_MOVSS: 
       case   XED_ICLASS_MOVSW:
       case   XED_ICLASS_MOVSX: 
       case   XED_ICLASS_MOVSXD:
       case   XED_ICLASS_MOVUPD: 
       case   XED_ICLASS_MOVUPS:
       case   XED_ICLASS_MOVZX: 
       case   XED_ICLASS_MOV_CR:
       case   XED_ICLASS_MOV_DR:
        if(INS_IsMemoryRead(I))
            opcode = LOAD;
        else if(INS_IsMemoryWrite(I))
            opcode = STORE;
        else 
            opcode = MOV; 
       break;
       
       case   XED_ICLASS_CALL_FAR: 
       case   XED_ICLASS_CALL_NEAR:
          opcode = CALL;
       break;
      
      case XED_ICLASS_JMP: 
      case XED_ICLASS_JMP_FAR:
        opcode = JMP;
      break;
      
      case   XED_ICLASS_JB:
      case   XED_ICLASS_JBE:
      case   XED_ICLASS_JCXZ: 
      case   XED_ICLASS_JECXZ: 
      case   XED_ICLASS_JL:
      case   XED_ICLASS_JLE: 
      case   XED_ICLASS_JNB:
      case   XED_ICLASS_JNBE: 
      case   XED_ICLASS_JNL:
      case   XED_ICLASS_JNLE: 
      case   XED_ICLASS_JNO:
      case   XED_ICLASS_JNP: 
      case   XED_ICLASS_JNS: 
      case   XED_ICLASS_JNZ:
      case   XED_ICLASS_JO: 
      case   XED_ICLASS_JP:
      case   XED_ICLASS_JRCXZ: 
      case   XED_ICLASS_JS:
      case   XED_ICLASS_JZ: 
          opcode = CJMP;                //conditional jump 
      break;
      
      case XED_ICLASS_LEA:
          opcode = LEA;
      break;
      
      case   XED_ICLASS_RET_FAR: 
      case   XED_ICLASS_RET_NEAR:
          opcode = RET;
      break;
      
      case   XED_ICLASS_NOP:
      case   XED_ICLASS_NOP2:
      case   XED_ICLASS_NOP3: 
      case   XED_ICLASS_NOP4: 
      case   XED_ICLASS_NOP5: 
      case   XED_ICLASS_NOP6: 
      case   XED_ICLASS_NOP7: 
      case   XED_ICLASS_NOP8: 
      case   XED_ICLASS_NOP9: 
        opcode = NOP;
      break;
      
      case   XED_ICLASS_MUL:
      case   XED_ICLASS_MULPD:
      case   XED_ICLASS_MULPS: 
      case   XED_ICLASS_MULSD: 
      case   XED_ICLASS_MULSS: 
      case   XED_ICLASS_MULX:
      case   XED_ICLASS_IMUL:
          opcode = MUL;
      break;

      case   XED_ICLASS_DIV:
      case   XED_ICLASS_DIVPD:
      case   XED_ICLASS_DIVPS: 
      case   XED_ICLASS_DIVSD: 
      case   XED_ICLASS_DIVSS:
      case   XED_ICLASS_IDIV:
          opcode = DIV;
      break;

      case   XED_ICLASS_ADD:
      case   XED_ICLASS_ADDPD:
      case   XED_ICLASS_ADDPS: 
      case   XED_ICLASS_ADDSD: 
      case   XED_ICLASS_ADDSS: 
      case   XED_ICLASS_VADDPD:
      case   XED_ICLASS_VADDPS: 
      case   XED_ICLASS_VADDSD: 
      case   XED_ICLASS_VADDSS:
      opcode = ADD;
      break;

   }
   return opcode;
}
#endif
