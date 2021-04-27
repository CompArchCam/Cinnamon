#ifndef _CGEN_H
#define _CGEN_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <algorithm>
#include <map>
#include "../Visitor.h"
#include "../parser.hpp"


enum{
    INSTR=0,
    LOAD,
    FUNC_C,
    FUNC_H,
    INIT_C,
    EXIT_C,
    TEMP_C,
    GLOBAL_H,
    STAT_H,
    DYN_H
};
typedef struct{
  int type;
  int scope;
} attribute;
typedef struct{
    string name;
    int type;
} comp;

typedef struct {
    string var; 
    int type;
    int size;
} map_info;

typedef struct
{
    std::string type; 
    std::string id;
} arg_data;
struct var_use{
    int ref;
    int def;
    var_use(): ref(0), def(0) {}
};
typedef struct {
    string dtype; 
    string initval;
    string ctype;
} arg_map;

class CodeGen : public Visitor {
    
    fstream outfile_ins; 
    fstream outfile_load; 
    fstream outfile_f;
    fstream outfile_fh;
    fstream outfile_e;
    fstream outfile_i;
    fstream outfile_t;
    fstream outfile_sh;
    fstream outfile_dh;
    fstream outfile_gh;
    fstream *outfile[10];
    int curr;
    typedef void(CodeGen::*template_funcs)(bool);
    template_funcs generate_template[5];
   public:
        
    CodeGen(std::string filename);
    ~CodeGen();
    
    void indent();
    void create_handler_table();

    int generateTemplateCode(int, std::string);
    
    void generate_template_instr(bool called_direct);
    void generate_template_basicblock(bool called_direct);
    void generate_template_loop(bool called_direct);
    void generate_template_func(bool called_direct);
    void generate_template_module(bool called_direct);
    void visit(StatementList* stmtlst);

    void visit(SelectStatementList* stmtlst);

    void visit(NodeList* nl);

    void visit(ExpressionList* el);

    void visit(Identifier* id);
    
    void visit(StrConst* sc);    
    
    void visit(NumConst* nc);    
    
    void visit(BoolConst* bc);
    
    void visit(NullPtr* ptr);

    void visit(OpCode* op);
    
    void visit(IdentLHS* ilhs);
    
    void visit(IndexLHS* ixlhs);
    
    void visit(DerefLHS* dlhs);
    
    void visit(FunctionCall* fc);
    
    void visit(AssnExpression* aexpr);
         
    void visit(UnaryExpression* uexpr);
    
    void visit(BinaryCondExpr* bcexpr);

    void visit(BinaryLogicExpr* blexpr);
        
    void visit(BinaryArithExpr* baexpr);

    void visit(BinaryCompExpr* bcexpr);

    void visit(TypePredExpr* typeexpr);
   
    void visit(TypeDeclStmt* tstmt);

    void visit(TypeDeclStmtList* tstmtlist);
    
    void visit(ConstraintExpr* ruleexpr);

    void visit(RegIDExpr* regidexpr);

    void visit(RegValExpr* regvalexpr);

    void visit(ExprStatement* estmt);
   
    void visit(ifStmt* ifstmt);

    void visit(elseIfStmt* elseifstmt);

    void visit(ForStmt* forstmt);

    void visit(WhileStmt* whilestmt);

    void visit(VTypeList* vtypelst);

    void visit(PrimitiveType* primtype);

    void visit(ArrayType* array);

    void visit(PairType* pair);

    void visit(DictType* dict);

    void visit(VectType* vect);
    
    void visit(SetType* set_);

    void visit(TupleType* tuple);
    
    void visit(PrimitiveTypeDecl* ptypedecl);
    
    void visit(CompositeTypeDecl* ctypedecl);
   
    void visit(FileTypeDecl* ftypedecl);

    void visit(CommandBlock* cmdblock);
    
    void visit(Action* action);
     
    void visit(ProgramBlock* prog);
    
    void visit(Component* comp);
};
#endif
