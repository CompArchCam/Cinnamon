#ifndef _UTIL_H
#define _UTIL_H

#include <map>
#include <string>
using namespace std;

bool global=false;
bool lhs_assn=false;
int ruleID=1;

int mode=0;
int error_count=0;
int indentLevel[11]={0,0,0,0,0,0,0,0,0,0,0};   //separate indent level for each outfile; *_static, *_dynamic, func.cpp, func.h 
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
#define GET_TRIGGER_INSTR               "get_trigger_instruction(bb,rule);"
#define INSERT_CLEAN_CALL_PREFIX        "dr_insert_clean_call(drcontext,bb,trigger,(void*)func_"
#define INSERT_CLEAN_CALL_POSTFIX       ",false,"
#define INSERT_RULE_DATA                 ",false,4,OPND_CREATE_INT32(rule->reg0));"
#define INCLUDE(file)                   "#include \"" file "\" \n"                   
#define DR_MCONTEXT_DEF                 "dr_mcontext_t mc = {sizeof(mc), DR_MC_ALL};"
#define DR_MCONTEXT_GET                 "dr_get_mcontext(dr_get_current_drcontext(), &mc);"
#define RESET_VAR_COUNT                 "var_count = 0;"
map<int, int> t_location={
        {T_BEFORE, 1},
        {T_AT, 1},
        {T_AFTER, 2},
        {T_ENTRY, 4},
        {T_WITH, 4},
        {T_EXIT, 5},
        {T_ITER, 6}
};
map<int, string> to_str={
       {FUNC, "Function"},
       {LOOP, "Loop"},
       {INST, "Instruction"},
       {MODULE, "Module"},
       {BASICBLOCK, "BasicBlock"}
};
map<int, string> opcodes={
        {OP_CALL, "Instruction::Call"},
        {OP_MOV, "Instruction::Mov"},
        {OP_ADD, "Instruction::Add"},
        {OP_SUB, "Instruction::Sub"},
        {OP_MUL, "Instruction::Mul"},
        {OP_DIV, "Instruction::Div"},
        {OP_NOP, "Instruction::Nop"},
        {OP_RETURN, "Instruction::Return"},
        {OP_BRANCH, "Instruction::Branch"},
        {OP_LOAD, "Instruction::Load"},
        {OP_STORE, "Instruction::Store"},
        {OP_GETPTR, "Instruction::GetPointer"},
        {OP_CMP, "Instruction::Compare"},
        {OP_INC, "Instruction::Inc"}
};

map<int, string> instr_opcodes={
        {OP_CALL, "OP_call"},
        {OP_MOV, "OP_mov_imm"},
        {OP_ADD, "OP_add"},
        {OP_SUB, "OP_sub"},
        {OP_MUL, "OP_mul"},
        {OP_DIV, "OP_div"},
        {OP_NOP, "OP_nop"},
        {OP_RETURN, "OP_ret"},
        {OP_BRANCH, "Instruction::Branch"},
        {OP_LOAD, "OP_mov_ld"},
        {OP_STORE, "OP_mov_st"},
        {OP_GETPTR, "Instruction::GetPointer"},
        {OP_CMP, "OP_cmp"},
        {OP_INC, "OP_inc"},
        {OP_LABEL, "OP_LABEL"},
        {OP_JNL, "OP_jnl"},
        {OP_JL, "OP_jl"}
};

map<string, string> get_static_func = {
        {"val","get_value("},
        {"opcode","get_opcode("},
        {"nextaddr","get_nextaddr("},
        {"startAddr","get_fstartAddr("},
        {"id","get_id("},
        {"in","get_input("},
        {"out","get_output("},
        {"trgname","get_target_name("},
        {"trgaddr","get_target_addr("},
        {"type","get_type("},
        {"bb","get_basicblock("},
        {"size", "get_size("},
        {"parent", "get_parent("},
        {"dst", "get_dest("},
        {"src", "get_src2("},
        {"src2", "get_src3("},
        {"numLoops", "get_num_loops("}
};
//TODO: make these specific to component type
map<string, string> get_dyn_func = {
        {"val","get_value("},
        {"address","get_address("},
        {"opcode","get_opcode("},
        {"in","get_input("},
        {"out","get_output("},
        {"trgaddr","get_target_addr("},
        {"nextaddr","get_nextaddr("},
        {"memaddr","get_mem_rw_addr("},
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
        {"rtnval", "get_return_val("},
        {"dst", "get_dest("},
        {"src", "get_src1("},
        {"src2", "get_src2("},
        {"replace", "replace(drcontext, bb, "},
        {"delete", "delete_instr(drcontext, bb, "},
        {"insert_before", "insert_before(drcontext, bb, "},
        {"insert_after", "insert_after(drcontext, bb, "},
        {"print", "print(drcontext, bb, "},
        {"print_inst", "print_inst(drcontext, bb, "},
        {"print_bb", "print_bb(drcontext, bb, "},
        {"clear", "clear_bb(drcontext, bb, "},
        {"clear_but_last", "clear_bb_but_last(drcontext, bb, "},
        {"append", "bb_append(drcontext, bb, "},
        {"prepend", "bb_prepend(drcontext, bb, "},
        {"replace_bb", "replace_bb(drcontext, bb, "},
        {"replace_bb_but_last", "replace_bb_but_last(drcontext, bb, "}
};
map<string, int> acc_func_dyn_cat = {
        {"val",1},
        {"address",1},
        {"opcode",0},
        {"in",1},
        {"out",1},
        {"trgaddr",0},
        //{"nextaddr","get_nextaddr("},
        {"memaddr",1},
        {"lea",1},
        {"type",1},
        {"bb",1},
        {"size", 0},
        {"arg1", 0},
        {"arg2", 0},
        {"arg3", 0},
        {"arg4", 0},
        {"arg5", 0},
        {"arg6", 0},
        {"rtnval", 0},
        {"dest", 1},
        {"src1", 1},
        {"src2", 1}
};
map<string,string> types={
        {"mem", "JVAR_MEMORY"},
        {"reg", "JVAR_REGISTER"},
        {"const", "JVAR_CONSTANT"},
        {"poly", "JVAR_POLYNOMIAL"}
};
map<string,string> get_vector_func={
        {"push_back", "vectorPushback"},
        {"atpos", "vectorAt"}
};
map<string,string> get_set_func={
        {"count", "setCount"},
        {"insert", "setInsert"},
        {"size", "setSize"}
};
map<string,string> get_file_func={
        {"open", "fileOpen"},
        {"write", "fileWrite"},
        {"close", "fileClose"}
};
map<string,string> get_bb_func={
        {"append", "bb_append(drcontext, bb, "},
        {"prepend", "bb_prepend(drcontext, bb, "},
};
#endif


