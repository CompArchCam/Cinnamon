#ifndef _UTIL_H
#define _UTIL_H

#include <map>
#include <string>
using namespace std;

bool global=false;
bool lhs_assn=false;
int actionID=0;

int mode=0;
int error_count=0;
int indentLevel[6]={0,0,0,0,0,0};   //separate indent level for each outfile; *_static, *_dynamic, func.cpp, func.h 
map<string,string> get_func[2];
map<string,string> cType[2];
map<string,string> dType[2];
map<int, string> bpatch_type{
  {INST, "BPatch_point"},
  {BASICBLOCK, "BPatch_basicBlock"},
  {LOOP, "BPatch_basicBlockLoop"},
  {FUNC, "BPatch_function"},
  {MODULE, "BPatch_module"}
};
#define DYNAMIC 1
#define STATIC 0
#define SEMICOLON                        ";"
#define OPENPARAN                        "("
#define CLOSEPARAN                        ")"
#define OPENBRACK                        "{"
#define CLOSEBRACK                        "}"
#define RTN_FAILURE                     "return EXIT_FAILURE;"
#define STR(s)                          #s
#define STRVAL(s)                       s
#define INCLUDE(file)                   "#include \"" file "\" \n"                   
map<int, int> t_location={
        {T_BEFORE, 1},
        {T_AFTER, 2},
        {T_AT, 3},
        {T_ENTRY, 4},
        {T_EXIT, 5},
        {T_ITER, 6}
};
map<int, string> opcodes={
        {OP_CALL, "Dyninst::InstructionAPI::c_CallInsn"},
        {OP_MOV, "Dyninst::InstructionAPI::c_MovInsn"},
        {OP_ADD, "Dyninst::InstructionAPI::c_AddInsn"},
        {OP_NOP, "Dyninst::InstructionAPI::c_NopInsn"},
        {OP_RETURN, "Dyninst::InstructionAPI::c_ReturnInsn"},
        {OP_BRANCH, "Dyninst::InstructionAPI::c_BranchInsn"},
        {OP_CMP, "Dyninst::InstructionAPI::c_CompareInsn"},
        {OP_LOAD, "Dyninst::InstructionAPI::c_LoadInsn"},
        {OP_STORE, "Dyninst::InstructionAPI::c_StoreInsn"}
};

map<string, string> cTypeStat={
    {"opcode","uint64_t"},
    {"id","uint64_t"},
    {"trgname","const char *"},
    {"nextaddr","uint64_t"},
    {"address","uint64_t"},
    {"startAddr","uint64_t"},
    {"size", "uint64_t"},
    {"numLoops", "uint64_t"}
};
map<string, string> dTypeStat={
    {"opcode","BPatch_constExpr"},
    {"id","BPatch_constExpr"},
    {"trgname","BPatch_constExpr"},
    {"nextaddr","BPatch_constExpr"},
    {"address","BPatch_constExpr"},
    {"startAddr","BPatch_constExpr"},
    {"size", "BPatch_constExpr"},
    {"numLoops", "BPatch_constExpr"}
};
map<string, string> cTypeDyn={
    {"trgaddr","uint64_t"},
        //{"trgaddr","get_target_addr("},
    {"memaddr","uint64_t"},
    {"rtnval","uint64_t"},
    {"arg1", "uint64_t"},
    {"arg2", "uint64_t"},
    {"arg3", "uint64_t"},
    {"arg4", "uint64_t"},
    {"arg5", "uint64_t"},
    {"arg6", "uint64_t"},
};
map<string,string> init={
  {"arg1", "0"},
  {"arg2", "1"},
  {"arg3", "2"},
  {"arg4", "3"},
  {"arg5", "4"},
  {"arg6", "5"},
};

map<string, string> get_static_func = {
        {"val","get_value("},
        {"opcode","get_opcode("},
        {"id","get_id("},
        {"trgname","get_target_name(appImage,"},
        {"nextaddr","get_next_addr("},
        {"address","get_addr("},
        {"startAddr","get_start_addr("},
        {"type","get_type("},
        {"bb","get_basicblock("},
        {"size", "get_size("},
        {"parent", "get_parent("},
        {"dest", "get_dest("},
        {"src1", "get_src1("},
        {"src2", "get_src2("},
        {"numLoops", "get_num_loops("}
};
//TODO: make these specific to component type
map<string, string> get_dyn_func = {
        {"val","get_value("},
        {"address","get_addr("},
        {"in","get_input("},
        {"out","get_output("},
        {"trgaddr","BPatch_dynamicTargetExpr"},
        {"memaddr","BPatch_effectiveAddressExpr"},
        {"lea","get_lea("},
        {"type","get_type("},
        {"bb","get_basicblock("},
        {"size", "get_size("},
        {"arg1", "BPatch_paramExpr"},
        {"arg2", "get_arg2("},
        {"arg3", "get_arg3("},
        {"arg4", "get_arg4("},
        {"arg5", "get_arg5("},
        {"arg6", "get_arg6("},
        {"rtnval", "BPatch_retExpr"},
        {"dest", "get_dest("},
        {"src1", "get_src1("},
        {"src2", "get_src2("}
};
map<string,string> types={
        {"mem", "JVAR_MEMORY"},
        {"reg", "JVAR_REGISTER"},
        {"const", "JVAR_CONSTANT"},
        {"poly", "JVAR_POLYNOMIAL"}
};
#endif


