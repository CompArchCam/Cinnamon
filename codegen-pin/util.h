#ifndef _UTIL_H
#define _UTIL_H

#include <map>
#include <string>
using namespace std;

int indentLevel[10]={0,0,0,0,0,0,0,0,0,0};   //separate indent level for each outfile; *_static, *_dynamic, func.cpp, func.h 

#define DYNAMIC 1
#define STATIC 0
#define SEMICOLON                        ";"
#define OPENPARAN                        "("
#define CLOSEPARAN                        ")"
#define OPENBRACK                        "{"
#define CLOSEBRACK                        "}"
#define ARRAY 0
#define FILE_T 1

#define STR(s)                          #s
#define STRVAL(s)                       s
//#define GET_FIELD(func, arg)            "get_" STR(STRVAL(func)) "(" STR(STRVAL(arg)) ")"
#define INCLUDE(file)                   "#include \"" file "\" \n"                   
#define ADD_INSTR_FUNC                  "AddInstrumentFunction"                   
#define INS_ROUTINE(varname)            "VOID Instruction(INS " varname ", VOID *v)"
#define ALLOC_ARGS                       "ARGLIST args = IARGLIST_Alloc();"
#define FREE_ARGS                        "IARGLIST_Free(args);"
#define ADD_ARGS                         "IARGLIST_AddArguments(args, "
#define END_ARGS                         ", IARG_END);"
#define ARG_LIST                         ", IARG_IARGLIST, args, IARG_END);"
map<string,string> get_func[2];
map<int,string> get_reg[3];
map<int, string> opcodes={
        {OP_CALL, "CALL"},
        {OP_MOV, "MOV"},
        {OP_ADD, "ADD"},
        {OP_SUB, "SUB"},
        {OP_MUL, "MUL"},
        {OP_DIV, "DIV"},
        {OP_NOP, "NOP"},
        {OP_RETURN, "RET"},
        {OP_BRANCH, "BR"},
        {OP_LOAD, "LOAD"},
        {OP_STORE, "STORE"},
        {OP_GETPTR, "LEA"},
        {OP_CMP, "CMP"}
};
map<int, string> regtable_16={
    {REG_64_RAX, "REG_AX"},
    {REG_64_RBX, "REG_BX"}
};
map<int, string> regtable_32={
    {REG_64_RAX, "REG_EAX"},
    {REG_64_RBX, "REG_EBX"}
};
map<int, string> regtable_64={
    {REG_64_RAX, "REG_RAX"},
    {REG_64_RBX, "REG_RBX"},
    {REG_64_SP, "REG_RSP"},
    {REG_64_FP, "REG_RBP"},
    {REG_64_PC, "REG_RIP"}
};

map<string, string> toPinType={
        {"bool", "IARG_BOOL"},
        {"int", "IARG_UINT32"},
        {"uint32_t", "IARG_UINT32"},
        {"uint64_t", "IARG_UINT64"},
        {"ADDRINT", "IARG_ADDRINT"}
        /*{DOUBLE, "IARG_PTR"},            //TODO: confirm this
        {TUPLE, "IARG_PTR"}*/
};
map<int, string> toCType={
        {BOOL, "bool"},
        {INT, "int"},
        {UINT32, "uint32_t"},
        {UINT64, "uint64_t"},
        {ADDR_INT, "ADDRINT"},
        {DOUBLE, "double"}            //TODO: confirm this
};

//TODO: make these specific to component type
#define ANLS 0
#define EXE 1
map<string, string>acc_func_stat = {
        {"address","get_addr("},           //Instruction Addr, Function Addr
        {"startAddr","get_addr("},           //Instruction Addr, Function Addr
        {"id","get_id("},           //Instruction id or reg id
        {"base","get_base("},           //base reg
        {"opcode","get_opcode("},       //Instruction opcode
        {"trgname","get_trgname("},  //I.trgname . only valid if instruction is direct call
        {"trgaddr","get_trgaddr("},  //I.trgaddr, only valid if instrucion is direct call
        {"falladdr","get_falladdr("},  //I.falladdr
        {"nextaddr","get_nextaddr("},  //I.falladdr
        {"type","get_type("},           //Operand.type 
        {"size", "get_size("},          //I.size, F.size, BB.size
        {"dest", "get_dest("},
        {"src1", "get_src1("},
        {"src", "get_src("},
        {"memopnd", "get_memory_opnd("},
        {"src2", "get_src2("},
        {"stackFrameSize", "get_stackFrameSize("},
        {"totalFrameSize", "get_totalFrameSize("},
        {"hasConstantSP", "has_constantSP("},
        {"stackSize", "get_stackSize("}

};
map<string, string> acc_func_dyn = {
        {"val","get_value("},           //IARG_UINT64 
        {"address","get_addr("},   //IARG_ADDRINT
        {"startAddr","get_addr("},   //IARG_ADDRINT
        {"nextaddr","get_nextaddr("},  //I.nextaddr
        {"opcode","get_opcode("},    //IARG_UINT32
        {"trgname","get_trgname("}, //IARG_PTR
        //{"trgaddr","get_trgaddr("},  //IARG_ADDRINT
        {"trgaddr","IARG_BRANCH_TARGET_ADDR"},  //IARG_ADDRINT
        {"id","get_id("},           //IARG_UINT32
        {"base","get_base("},           //IARG_REG_REFERENCE
        //{"trgaddr","IARG_BRANCH_TARGET_ADDR"},  //for branch, call and return etc
        {"type","get_type("},            //IARG_UINT32
        {"size", "get_size("},           //IARG_UINT32
        {"lea","IARG_EXPLICIT_MEMORY_EA"}, //NULL             
        {"falladdr","IARG_FALLTHROUGH_ADDR"},  //IARG_ADDRINT
        {"arg1", "IARG_FUNCARG_CALLSITE_VALUE, 0"}, //at call sites. if called through instruction 
        {"arg2", "IARG_FUNCARG_CALLSITE_VALUE, 1"},     //NULL
        {"arg3", "IARG_FUNCARG_CALLSITE_VALUE, 2"},
        {"arg4", "IARG_FUNCARG_CALLSITE_VALUE, 3"},
        {"arg5", "IARG_FUNCARG_CALLSITE_VALUE, 4"},
        {"arg6", "IARG_FUNCARG_CALLSITE_VALUE, 5"},  //IARG_FUNCARG_ENTRYPOINT_VALUE at the entry point of function of called through function.
        {"rtnval", "IARG_REG_VALUE, REG_RAX"}, //if instruction is a call then read the value of RAX
        {"dest", "get_dest("},
        {"memopnd", "get_memory_opnd("},
        {"src1", "get_src1("},
        {"src", "get_src("},
        {"src2", "get_src2("},
        {"leaddr" , "IARG_EXPLICIT_MEMORY_EA"},
        {"memaddr" , "INS_IsMemoryWrite(ins) ? IARG_MEMORYWRITE_EA : (INS_IsMemoryRead(ins) ? IARG_MEMORYREAD_EA"},
        {"memaddr_r", "IARG_MEMORYREAD_EA"},
        {"memaddr_w", "IARG_MEMORYWRITE_EA"},
        {"stackSize", "get_stackSize("}

};
map<string, int> acc_func_dyn_cat = {
        {"val",1},           //IARG_UINT64 
        {"address",1},   //IARG_ADDRINT
        {"opcode",1},    //IARG_UINT32
        {"trgname",1}, //IARG_PTR
        //{"trgaddr",1},  //IARG_ADDRINT
        {"nextaddr",1},  //IARG_ADDRINT
        {"trgaddr",0},  //IARG_ADDRINT
        {"type",1},            //IARG_UINT32
        {"size", 1},           //IARG_UINT32
        {"id",1},           //IARG_UINT32
        {"base",1},           //base reg, IARG_REG_REFERENCE
        {"stackSize", 1},
        {"lea",0}, //NULL             
        {"falladdr",0},  //IARG_ADDRINT
        {"arg1", 0}, //at call sites. if called through instruction 
        {"arg2", 0},     //NULL
        {"arg3", 0},
        {"arg4", 0},
        {"arg5", 0},
        {"arg6", 0},  //IARG_FUNCARG_ENTRYPOINT_VALUE at the entry point of function of called through function.
        {"rtnval", 0}, //if instruction is a call then read the value of RAX
        //{"rtnval", " IARG_FUNCRET_EXITPOINT_VALUE"}, //if instruction is a function then  then read the value of RAX
        {"dest", 1},
        {"src1", 1},
        {"src2", 1},
        {"memaddr" , 0},
        {"memaddr_r", 0},
        {"memaddr_w", 0}

};
map<string, string> acc_type_dyn_C = {
        {"val","int64_t"},           //IARG_UINT64 
        {"address","ADDRINT"},   //IARG_ADDRINT
        {"opcode","uint32_t"},    //IARG_UINT32
        {"trgname","PTR"}, //TODO: correct it
        {"trgaddr","ADDRINT"},  //IARG_ADDRINT
        {"type","uint32_t"},            //IARG_UINT32
        {"size", "uint32_t"},           //IARG_UINT32
        {"id", "uint32_t"},           //IARG_UINT32
        {"base","REG"}, //NULL             
        {"lea","ADDRINT"}, //NULL             
        {"falladdr","ADDRINT"},  //IARG_ADDRINT
        {"nextaddr","ADDRINT"},  //IARG_ADDRINT
        {"stackSize","uint64_t"},  //IARG_ADDRINT
        {"arg1", "uint64_t"}, //at call sites. if called through instruction 
        {"arg2", "uint64_t"},     //NULL
        {"arg3", "uint64_t"},
        {"arg4", "uint64_t"},
        {"arg5", "uint64_t"},
        {"arg6", "uint64_t"},  //IARG_FUNCARG_ENTRYPOINT_VALUE at the entry point of function of called through function.
       {"rtnval", "uint64_t"}, //if instruction is a call then read the value of RAX
        {"dest", "NULL"},
        {"src1", "NULL"},
        {"src2", "NULL"},
        {"memaddr" , "ADDRINT"},
        {"memaddr_r", "ADDRINT"},
        {"memaddr_w", "ADDRINT"}

};
map<string, string> acc_type_dyn_Pin = {
        {"val","IARG_UINT64"},           //IARG_UINT64 
        {"address","IARG_ADDRINT"},   //IARG_ADDRINT
        {"opcode","IARG_UINT32"},    //IARG_UINT32
        {"trgname","IARG_PTR"}, //IARG_PTR
        {"trgaddr","NULL"},  //IARG_ADDRINT
        {"nextaddr","IARG_ADDRINT"},  //IARG_ADDRINT
        {"type","IARG_UINT32"},            //IARG_UINT32
        {"size", "IARG_UINT32"},           //IARG_UINT32
        {"id", "IARG_UINT32"},           //IARG_UINT32
        {"base", "IARG_REG_REFERENCE"},           //IARG_UINT32
        {"stackSize", "IARG_UINT64"},           //IARG_UINT32
        {"lea","NULL"}, //NULL             
        {"falladdr","IARG_ADDRINT"},  //IARG_ADDRINT
        {"arg1", "NULL"}, //at call sites. if called through instruction 
        {"arg2", "NULL"},     //NULL
        {"arg3", "NULL"},
        {"arg4", "NULL"},
        {"arg5", "NULL"},
        {"arg6", "NULL"},  //IARG_FUNCARG_ENTRYPOINT_VALUE at the entry point of function of called through function.
       {"rtnval", "NULL"}, //if instruction is a call then read the value of RAX
        {"dest", "NULL"},
        {"src1", "NULL"},
        {"src2", "NULL"},
        {"memaddr" , "NULL"},
        {"memaddr_r", "NULL"},
        {"memaddr_w", "NULL"}

};
map<string,string> types={
        {"mem", "MEM"},
        {"reg", "REGISTER"},
        {"Const", "CONST"},
};
#endif

