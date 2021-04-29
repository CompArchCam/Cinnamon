#ifndef _DSL_UTIL_
#define _DSL_UTIL_
#include <cstdlib>
#include "BPatch.h"
#include "BPatch_point.h"
#include "Instruction.h"
#include "InstructionCategories.h"
using namespace Dyninst;
#define get_start_addr(F)       (char *)F->getBaseAddr()
#define f.open(var)             f.open(var, ios::in | ios::out)
void writeToFile(fstream &file, uintptr_t val ){
        file<<val<<endl;
}
void writeToFile(fstream &file, std::string val ){
        file<<val<<endl;
}
void writeToFile(fstream &file, char* val ){
        file<<val<<endl;
}
InstructionAPI::InsnCategory get_opcode(std::pair<InstructionAPI::Instruction,Address >& insn){
        if(insn.first.readsMemory()) return Dyninst::InstructionAPI::c_LoadInsn;
        else if(insn.first.writesMemory()) return Dyninst::InstructionAPI::c_StoreInsn;
        return insn.first.getCategory();
}
std::string get_target_name(BPatch_image *appImage, std::pair<InstructionAPI::Instruction,Address >& insn){
        std::string name;
        std::vector<BPatch_point *> points;
        appImage->findPoints(insn.second, points);
        //if(points[0]->getPointType() == BPatch_subroutine){}
        return points[0]->getCalledFunctionName();
}
uintptr_t get_next_addr(std::pair<InstructionAPI::Instruction,Address >& insn){
   uintptr_t nextaddr = insn.second + insn.first.size();;
   return  nextaddr;
}
uint64_t get_num_loops(BPatch_module *M){
     uint64_t num_loops = 0;
     for(auto &F : *(M->getProcedures())){
         std::vector <BPatch_basicBlockLoop*> AllLoops;
         BPatch_flowGraph * CFG = F->getCFG();
         CFG->getLoops(AllLoops);
         num_loops += AllLoops.size();
     }
     return num_loops;
}
uint64_t get_num_loops(BPatch_function *F){
     std::vector <BPatch_basicBlockLoop*> AllLoops;
     BPatch_flowGraph * CFG = F->getCFG();
     CFG->getLoops(AllLoops);
     return AllLoops.size();
}
#endif
