#ifndef _VISITOR_H
#define _VISITOR_H

#include "AST.h"

class Visitor{
public:
        virtual void visit(NodeList* nl) = 0;
        virtual void visit(ExpressionList* el) = 0;
        virtual void visit(Identifier* i) =0;
        virtual void visit(StrConst* sc) =0;
        virtual void visit(NumConst* nc) =0;
        virtual void visit(BoolConst* bc) =0;
        virtual void visit(OpCode* op) =0;
        virtual void visit(NullPtr* ptr) =0;
        virtual void visit(IdentLHS* ilhs) =0;
        virtual void visit(IndexLHS* ixlhs) =0;
        virtual void visit(DerefLHS* dlhs) =0;
        virtual void visit(FunctionCall* fc) =0;
        virtual void visit(AssnExpression* aexpr) =0;
        virtual void visit(UnaryExpression* uexpr) =0;
        virtual void visit(BinaryCondExpr* bcexpr) =0;
        virtual void visit(BinaryCompExpr* bcexpr) =0;
        virtual void visit(BinaryLogicExpr* blexpr) =0;
        virtual void visit(BinaryArithExpr* baexpr) =0;
        virtual void visit(TypePredExpr* typeexpr) =0;
        virtual void visit(TypeDeclStmt* tstmt) =0;
        virtual void visit(TypeDeclStmtList* tstmtlist) =0;
        virtual void visit(RegIDExpr* regidexpr) =0;
        virtual void visit(RegValExpr* regvalexpr) =0;
        virtual void visit(ConstraintExpr* condexpr) =0;
        virtual void visit(StatementList* stmtlst) =0;
        virtual void visit(SelectStatementList* stmtlst) =0;
        virtual void visit(ExprStatement* estmt) =0;
        virtual void visit(ifStmt* ifstmt) =0;
        virtual void visit(elseIfStmt* elseifstmt) =0;
//        virtual void visit(ifElseStmt* ifelsestmt) =0;
//        virtual void visit(ifElseIfStmt* ifelseifstmt) =0;
        virtual void visit(ForStmt* forstmt) =0;
        virtual void visit(WhileStmt* whilestmt) =0;
        /*virtual void visit(TypeDecl* typedecl) =0;*/
        //virtual void visit(TypeDeclList* typedecllst) =0;
        virtual void visit(VTypeList* vtypelst) =0;
        virtual void visit(PrimitiveType* primtype) =0;
        virtual void visit(ArrayType* array) =0;
        virtual void visit(PairType* pair) =0;
        virtual void visit(DictType* dict) =0;
        virtual void visit(VectType* vect) =0;
        virtual void visit(SetType* set) =0;
        virtual void visit(TupleType* tuple) =0;
        virtual void visit(PrimitiveTypeDecl* ptypedecl) =0;
        virtual void visit(FileTypeDecl* ftypedecl) =0;
        virtual void visit(CompositeTypeDecl* ctypedecl) =0;
        virtual void visit(CommandBlock* cmdblock) =0;
        virtual void visit(Action* action) =0;
        virtual void visit(ProgramBlock* prog) =0;
        virtual void visit(Component* comp) =0;

};
#endif
