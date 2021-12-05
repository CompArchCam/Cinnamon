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
    STAT=0,
    GLOBAL_H,
    DYN,
    FUNC_C,
    FUNC_H,
    EXIT_C,
    INIT_C,
    TEMP_C,
    STAT_H,
    DYN_H,
    ACT_C,
    AT_C
};
typedef struct{
  int type;
  int scope;
} attribute;
typedef struct{
    string name;
    int type;
} comp;
struct var_use{
    int ref;
    int def;
    var_use(): ref(0), def(0) {}
};
typedef struct {
    string var; 
    string type;
    int size;
    int cat;
} map_info;

typedef struct
{
    std::string type; 
    std::string id;
} arg_data;

class CodeGen : public Visitor {
    
    fstream outfile_s; 
    fstream outfile_gh; 
    fstream outfile_d; 
    fstream outfile_f;
    fstream outfile_fh;
    fstream outfile_e;
    fstream outfile_i;
    fstream outfile_t;
    fstream outfile_sh;
    fstream outfile_dh;
    fstream outfile_ac;
    fstream outfile_at;
    fstream *outfile[12];
    int curr;
   public:
        
    CodeGen(std::string filename);
    ~CodeGen();
    
    void indent();
    void create_handler_table();

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
    
    void visit(ConstraintExpr* condexpr);

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

    void visit(TupleType* tuple);
    
    void visit(VectType* vect);

    void visit(SetType* set_);

    void visit(PrimitiveTypeDecl* ptypedecl);
    
    void visit(CompositeTypeDecl* ctypedecl);
    
    void visit(FileTypeDecl* ftypedecl);

    void visit(InstTypeDecl* itypedecl);

    void visit(BasicblockTypeDecl* bbtypedecl);

    void visit(CommandBlock* cmdblock);
    
    void visit(Action* action);
     
    void visit(ProgramBlock* prog);
    
    void visit(Component* comp);

    void generateMapFunctions();

    void generateSetFunctions();

    void generateUtilFunctions();

    void generateVectorFunctions();
};
#endif
