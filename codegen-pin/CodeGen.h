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
    FUNC_C=4, //INS=0, BBL=1, RTN=2 and IMG=3
    FUNC_H,
    EXIT_C,
    INIT_C,
    TEMP_C,
    INST_PTYPE
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
    string type;
    //int type;
    int size;
    string ctype;
} map_info;

typedef struct
{
    std::string type; 
    std::string id;
} arg_data;

class CodeGen : public Visitor {
    
    fstream outfile_ins; 
    fstream outfile_bbl; 
    fstream outfile_rtn; 
    fstream outfile_img; 
    fstream outfile_f;
    fstream outfile_fh;
    fstream outfile_e;
    fstream outfile_t;
    fstream outfile_i;
    fstream outfile_p;
    fstream *outfile[10];
    //ofstream *outfile[6];
    int curr;
   public:
        
    CodeGen(std::string filename);
    ~CodeGen();
    
    void indent();
    void create_handler_table();
    void printInstrFunc(int type, std::string scopevar);
    void visit(StatementList* stmtlst);
    
    void visit(SelectStatementList* stmtlst);

    void visit(NodeList* nl);
    
    void visit(ExpressionList* el);

    void visit(Identifier* id);
    
    void visit(StrConst* sc);    
    
    void visit(NumConst* nc);    
    
    void visit(BoolConst* bc);

    void visit(OpCode* op);
    
    void visit(NullPtr* ptr);
    
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
    
    void visit(ConstraintExpr* constrexpr);

    void visit(RegIDExpr* regidexpr);
    
    void visit(RegValExpr* regvalexpr);

    void visit(ExprStatement* estmt);
   
    //TODO: merge ifelse statements into one
    void visit(ifStmt* ifstmt);

//    void visit(ifElseStmt* ifelsestmt);
    
    //void visit(ifElseIfStmt* ifelseifstmt);
    
    void visit(elseIfStmt* elseifstmt);

    void visit(ForStmt* forstmt);

    void visit(WhileStmt* whilestmt);

    //void visit(TypeDeclList* typedecllst);

    void visit(VTypeList* vtypelst);

    void visit(PrimitiveType* primtype);

    void visit(ArrayType* array);

    void visit(PairType* pair);

    void visit(DictType* dict);

    void visit(VectType* vect);
    
    void visit(SetType* set_);

    void visit(TupleType* tuple);
    
    void visit(PrimitiveTypeDecl* ptypedecl);
    
    void visit(FileTypeDecl* ftypedecl);
    
    void visit(CompositeTypeDecl* ctypedecl);
    
    void visit(CommandBlock* cmdblock);
    
    void visit(Action* action);
     
    void visit(ProgramBlock* prog);
    
    void visit(Component* comp);
};
#endif
