#include "Visitor.h"
#include "AST.h"
using namespace std; 


void NodeList::accept(Visitor& v){
    v.visit(this);
}
void ExpressionList::accept(Visitor& v){
    v.visit(this);
}
void Identifier::accept(Visitor& v){
    v.visit(this);
}
void StrConst::accept(Visitor& v){
    v.visit(this);
}
void NumConst::accept(Visitor& v){
    v.visit(this);
}
void BoolConst::accept(Visitor& v){
    v.visit(this);
}
void OpCode::accept(Visitor& v){
    v.visit(this);
}
void NullPtr::accept(Visitor& v){
    v.visit(this);
}
void IdentLHS::accept(Visitor& v){
    v.visit(this);
}
void IndexLHS::accept(Visitor& v){
    v.visit(this);
}
void DerefLHS::accept(Visitor& v){
    v.visit(this);
}

void FunctionCall::accept(Visitor& v){
    v.visit(this);
}
void AssnExpression::accept(Visitor& v){
    v.visit(this);
}
void UnaryExpression::accept(Visitor& v){
    v.visit(this);
}
void BinaryCondExpr::accept(Visitor& v){
    v.visit(this);
}
void BinaryLogicExpr::accept(Visitor& v){
    v.visit(this);
}
void BinaryCompExpr::accept(Visitor& v){
    v.visit(this);
}
void BinaryArithExpr::accept(Visitor& v){
    v.visit(this);
}
void TypePredExpr::accept(Visitor& v){
    v.visit(this);
}
void ConstraintExpr::accept(Visitor& v){
    v.visit(this);
}
void RegIDExpr::accept(Visitor& v){
    v.visit(this);
}
void RegValExpr::accept(Visitor& v){
    v.visit(this);
}
/***************************Statements**************************/
void StatementList::accept(Visitor& v){
    v.visit(this);
}
void SelectStatementList::accept(Visitor& v){
    v.visit(this);
}
void ExprStatement::accept(Visitor& v){
    v.visit(this);
}
void ifStmt::accept(Visitor& v){
    v.visit(this);
}
/*void ifElseStmt::accept(Visitor& v){
    v.visit(this);
}
void ifElseIfStmt::accept(Visitor& v){
    v.visit(this);
}*/
void elseIfStmt::accept(Visitor& v){
    v.visit(this);
}
void ForStmt::accept(Visitor& v){
    v.visit(this);
}
void WhileStmt::accept(Visitor& v){
    v.visit(this);
}

/**********************voidypes and Type Decl ***********************/
/*void TypeDeclList::accept(Visitor& v){
    v.visit(this);
}*/
void VTypeList::accept(Visitor& v){
    v.visit(this);
}
void PrimitiveType::accept(Visitor& v){
    v.visit(this);
}
void ArrayType::accept(Visitor& v){
    v.visit(this);
}
void PairType::accept(Visitor& v){
    v.visit(this);
}
void DictType::accept(Visitor& v){
    v.visit(this);
}
void VectType::accept(Visitor& v){
    v.visit(this);
}
void SetType::accept(Visitor& v){
    v.visit(this);
}
void TupleType::accept(Visitor& v){
    v.visit(this);
}
void PrimitiveTypeDecl::accept(Visitor& v){
    v.visit(this);
}
void CompositeTypeDecl::accept(Visitor& v){
    v.visit(this);
}
void FileTypeDecl::accept(Visitor& v){
    v.visit(this);
}
void InstTypeDecl::accept(Visitor& v){
    v.visit(this);
}
void BasicblockTypeDecl::accept(Visitor& v){
    v.visit(this);
}
void TypeDeclStmt::accept(Visitor& v){
    v.visit(this);
}
void TypeDeclStmtList::accept(Visitor& v){
    v.visit(this);
}

/********************Command********************************/
void CommandBlock::accept(Visitor& v){
    v.visit(this);
}
void Action::accept(Visitor& v){
    v.visit(this);
}
/************************Program - root node***********************/
void ProgramBlock::accept(Visitor& v){
    v.visit(this);
}
/**************************Component**********************/
void Component::accept(Visitor& v){
    v.visit(this);
}


