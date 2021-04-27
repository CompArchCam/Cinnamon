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

#define DYNAMIC 1
#define STATIC 0
#define SEMICOLON                        ";"
#define OPENPARAN                        "("
#define CLOSEPARAN                        ")"
#define OPENBRACK                        "{"
#define CLOSEBRACK                        "}"


#define STR(s)                          #s
#define STRVAL(s)                       s
//#define GET_FIELD(func, arg)            "get_" STR(STRVAL(func)) "(" STR(STRVAL(arg)) ")"
#define GET_TRIGGER_INSTR               "get_trigger_instruction(bb,rule);"
//#define GET_TRIGGER_INSTR               "instr_t *trigger = get_trigger_instruction(bb,rule);"
#define INSERT_CLEAN_CALL_POSTFIX       ",false,"
#define INCLUDE(file)                   "#include \"" file "\" \n"                   
#define RESET_VAR_COUNT                 "var_count = 0;"
#define RTN_FAILURE                     "return EXIT_FAILURE;"
#define GET_MODULE                      "for(auto modules: appImage->getModules)"
//#define FUNC_ARGS                       "void* dr_context, instrlist_t *bb, instr_t *"
map<int, int> t_location={
        {T_BEFORE, 1},
        {T_AFTER, 2},
        {T_AT, 3},
        {T_ENTRY, 4},
        {T_EXIT, 5},
        {T_ITER, 6}
};
/*map<int, string> opcodes={
        {OP_CALL, "e_call"},
        {OP_MOV, "e_mov"},
        {OP_ADD, "e_add"},
        {OP_SUB, "e_sub"},
        {OP_MUL, "e_mul"},
        {OP_DIV, "e_div"},
        {OP_NOP, "e_nop"},
        {OP_RETURN, "Dyninst::InstructionAPI::c_ReturnInsn"},
        {OP_BRANCH, "Dyninst::InstructionAPI::c_BranchInsn"},
        {OP_LOAD, "Instruction::Load"},
        {OP_STORE, "Instruction::Store"},
        {OP_LEA, "e_lea"},
        {OP_GETPTR, "Instruction::GetPointer"},
        {OP_CMP, "e_cmp"}
};*/
map<int, string> opcodes={
        {OP_CALL, "Dyninst::InstructionAPI::c_CallInsn"},
        {OP_MOV, "Dyninst::InstructionAPI::c_MovInsn"},
        {OP_ADD, "Dyninst::InstructionAPI::c_AddInsn"},
        {OP_NOP, "Dyninst::InstructionAPI::c_NopInsn"},
        {OP_RETURN, "Dyninst::InstructionAPI::c_ReturnInsn"},
        {OP_BRANCH, "Dyninst::InstructionAPI::c_BranchInsn"},
        {OP_CMP, "Dyninst::InstructionAPI::c_CompareInsn"}
};


map<string, string> get_static_func = {
        {"val","get_value("},
        {"opcode","get_opcode("},
        {"id","get_id("},
        {"in","get_input("},
        {"out","get_output("},
        {"trgname","get_target_name("},
        {"trgaddr","get_target_addr("},
        {"type","get_type("},
        {"bb","get_basicblock("},
        {"size", "get_size("},
        {"parent", "get_parent("},
        {"arg1", "get_arg1("},
        {"arg2", "get_arg2("},
        {"arg3", "get_arg3("},
        {"arg4", "get_arg4("},
        {"arg5", "get_arg5("},
        {"arg6", "get_arg6("},
        {"rtn", "get_return("},
        {"dest", "get_dest("},
        {"src1", "get_src1("},
        {"src2", "get_src2("},
        {"numLoops", "get_num_loops("}
};
//TODO: make these specific to component type
map<string, string> get_dyn_func = {
        {"val","get_value("},
        {"addr","get_addr("},
        {"opcode","get_opcode("},
        {"in","get_input("},
        {"out","get_output("},
        {"trgaddr","get_target_addr("},
        {"lea","get_lea("},
        {"type","get_type("},
        {"bb","get_basicblock("},
        {"size", "get_size("},
        {"arg1", "get_arg1("},
        {"arg2", "get_arg2("},
        {"arg3", "get_arg3("},
        {"arg4", "get_arg4("},
        {"arg5", "get_arg5("},
        {"arg6", "get_arg6("},
        {"rtn", "get_return("},
        {"dest", "get_dest("},
        {"src1", "get_src1("},
        {"src2", "get_src2("}
};
/*map<string,string> types={
        {"mem", "JVAR_MEMORY"},
        {"reg", "JVAR_REGISTER"},
        {"const", "JVAR_CONSTANT"},
        {"poly", "JVAR_POLYNOMIAL"}
};*/
#endif


