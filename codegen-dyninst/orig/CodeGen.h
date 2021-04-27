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
    TEMP_C
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

class CodeGen : public Visitor {
    
    fstream outfile_ins; 
    fstream outfile_load; 
    fstream outfile_f;
    fstream outfile_fh;
    fstream outfile_e;
    fstream outfile_i;
    fstream outfile_t;
    fstream *outfile[7];
    //ofstream *outfile[6];
    int curr;
   public:
        
    CodeGen(std::string filename);
    ~CodeGen();
    
    void indent();
    void create_handler_table();

    int generateTemplateCode(int, std::string);
    
    void visit(StatementList* stmtlst);

    void visit(NodeList* nl);
    
    void visit(ExpressionList* el);

    void visit(Identifier* id);
    
    void visit(StrConst* sc);    
    
    void visit(NumConst* nc);    
    
    void visit(BoolConst* bc);

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
    
    void visit(RuleExpr* ruleexpr);

    void visit(ExprStatement* estmt);
   
    //TODO: merge ifelse statements into one
    void visit(ifStmt* ifstmt);

    void visit(ifElseStmt* ifelsestmt);
    
    void visit(ifElseIfStmt* ifelseifstmt);

    void visit(ForStmt* forstmt);

    void visit(WhileStmt* whilestmt);

    void visit(TypeDeclList* typedecllst);

    void visit(VTypeList* vtypelst);

    void visit(PrimitiveType* primtype);

    void visit(ArrayType* array);

    void visit(PairType* pair);

    void visit(DictType* dict);

    void visit(TupleType* tuple);
    
    void visit(PrimitiveTypeDecl* ptypedecl);
    
    void visit(CompositeTypeDecl* ctypedecl);
    
    void visit(Command* cmd);
    
    void visit(CommandBlock* cmdblock);
    
    void visit(Action* action);
     
    void visit(ProgramBlock* prog);
    
    void visit(Component* comp);
};
#endif
