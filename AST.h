#ifndef _AST_H
#define _AST_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std; 

class Visitor;
class Statement;
class TStatement;
class SelectionStmt;
class Expression;
class Node;
class Component;
class CommandBlock;
class Action;
class TypeDecl;
class TypeDeclStmt;
class TypeDeclStmtList;
class PrimitiveType;
class CompositeType;
class VType;
class PrimtiveTypeDecl;

typedef std::vector<Statement*>         stmtList;
typedef std::vector<SelectionStmt*> selectStmtList;
typedef std::vector<Expression*>        exprList;
typedef std::vector<TypeDecl*>          typeDeclList;
typedef std::vector<TypeDeclStmt*>       tstmtList;
typedef std::vector<VType*>             vtypelist;
typedef std::vector<Node*>              nodelist;

class ASTnode {
    public:
        virtual ~ASTnode() {}
        virtual void accept(Visitor& v)=0;
};

/*******************Expressions**********************/
/*all expressions inherit from this class*/
class Expression: public ASTnode{
   public:
};
class ExpressionList: public Expression{
    public:
    exprList expressions;
    ExpressionList(){}
    void accept(Visitor& v);
};
class Identifier: public Expression{
    public:
    std::string name;
    Identifier(const std::string& name): name(name){}
    void accept(Visitor& v);
};
class StrConst: public Expression{
    public:
        std::string value;
        StrConst(const std::string& value): value(value){}
    void accept(Visitor& v);
};
class NumConst: public Expression{
    public:
        int value;
        NumConst(int value): value(value){}
    void accept(Visitor& v);
};
class BoolConst: public Expression{
    public:
        int b_val;
        BoolConst(int b_val) : b_val(b_val){ } /*need to change the token value*/
        void accept(Visitor& v);
};
class NullPtr: public Expression{
    public:
        std::string null_ptr;
        NullPtr() : null_ptr("NULL"){ } /*need to change the token value*/
        void accept(Visitor& v);
};
class OpCode: public Expression{
    public:
        int value;
        OpCode(int value) : value(value){} /*need to change the token value*/
        void accept(Visitor& v);
};
class LHS: public Expression{
    public:
};
class IdentLHS: public LHS{
    public:
        Identifier* name;
        IdentLHS(Identifier* name): name(name){}
        void accept(Visitor& v);
};
class IndexLHS: public LHS{
    public:
        LHS* base;
        Expression* index;
        IndexLHS(LHS* base,Expression* index): base(base), index(index){}
        void accept(Visitor& v);
};
class DerefLHS: public LHS{
    public:
        LHS* base;
        Identifier* field;
        DerefLHS(LHS* base, Identifier* field): base(base), field(field){}
        void accept(Visitor& v);
};

class FunctionCall : public Expression{
    public:
         //Identifier* name;
         LHS* name;
         ExpressionList* arguments;
         FunctionCall(LHS* name, ExpressionList* arguments): name(name), arguments(arguments){}
         //FunctionCall(Identifier* name, ExpressionList* arguments): name(name), arguments(arguments){}
        void accept(Visitor& v);
};
class AssnExpression: public Expression{
    public:
       LHS* lhs;
       Expression* rhs;
       AssnExpression(LHS* lhs, Expression* rhs): lhs(lhs), rhs(rhs){}
        void accept(Visitor& v);
};
class TypePredExpr: public Expression{
   public:
       // LHS* lhs;
        Expression* lhs;
        //Identifier* type_name;
        string type_name;
        TypePredExpr(Expression* lhs, string type_name): lhs(lhs), type_name(type_name){}
        //TypePredExpr(Expression* lhs, Identifier* type_name): lhs(lhs), type_name(type_name){}
        void accept(Visitor& v);
};
class RegIDExpr: public Expression{
   public:
        int type;
        int reg;
        LHS* reg_expr;  //could change it to expr
        RegIDExpr(int type, int reg): type(type), reg(reg) { reg_expr = NULL;}
        RegIDExpr(int type, LHS* reg_expr): type(type), reg_expr(reg_expr){ reg = -1;}
        void accept(Visitor& v);
};
class RegValExpr: public Expression{
   public:
       // LHS* lhs;
        int type;
        int reg;
        LHS* reg_expr;  //could change it to expr
        RegValExpr(int type, int reg): type(type), reg(reg) { reg_expr = NULL;}
        RegValExpr(int type, LHS* reg_expr): type(type), reg_expr(reg_expr){ reg = -1;}
        void accept(Visitor& v);
};
class ConstraintExpr: public Expression{
        
    public:
        Expression* cond;
        ConstraintExpr(){ cond = NULL;};
        ConstraintExpr(Expression* cond): cond(cond){}
        void accept(Visitor& v);

};
class UnaryExpression: public Expression{
    public:
        int op;
        Expression* operand_expr;
        UnaryExpression(int op, Expression* operand_expr): op(op), operand_expr(operand_expr){}
        void accept(Visitor& v);
};
class BinaryExpression: public Expression{
    public: 
        void accept(Visitor& v);
};
class BinaryCondExpr: public Expression{
    public:
        Expression* lhs;
        Expression* rhs;
        int op;
        BinaryCondExpr(Expression* lhs, Expression* rhs, int op): lhs(lhs), rhs(rhs), op(op){}
        void accept(Visitor& v);
};
class BinaryLogicExpr: public Expression{
    public:
        Expression* lhs;
        Expression* rhs;
        int op;
        BinaryLogicExpr(Expression* lhs, Expression* rhs, int op): lhs(lhs), rhs(rhs), op(op){}
        void accept(Visitor& v);
};
class BinaryCompExpr: public Expression{
    public:
        Expression* lhs;
        Expression* rhs;
        int op;
        BinaryCompExpr(Expression* lhs, Expression* rhs, int op): lhs(lhs), rhs(rhs), op(op){}
        void accept(Visitor& v);
};
class BinaryArithExpr: public Expression{
    public:
        Expression* lhs;
        Expression* rhs;
        int op;
        BinaryArithExpr(Expression* lhs, Expression* rhs, int op): lhs(lhs), rhs(rhs), op(op){}
        void accept(Visitor& v);
};


/***************************Statements**************************/
/* all statements inherit from this class*/
class Statement: public ASTnode {
    public:
};
class StatementList: public Statement{
    public:
        stmtList statements;
        StatementList(){}
        void accept(Visitor& v);
};
class TStatement : public Statement{
    public:    
};
class TypeDeclStmtList: public TStatement{
    public:
        tstmtList t_stmts;
        TypeDeclStmtList(){}
        void accept(Visitor& v);
};
class ExprStatement: public Statement{
    public:
        Expression* expr;
        ExprStatement() {/*this->expr = new Expression();*/}
        ExprStatement(Expression* expr) : expr(expr){}
        void accept(Visitor& v);
};
class SelectionStmt: public Statement{
    public:
};
class SelectStatementList: public SelectionStmt{
    public:
        selectStmtList stmts;
        SelectStatementList(){}
        void accept(Visitor& v);
};
/*class ifStmt: public SelectionStmt{
    public:
        Expression* condition; 
        StatementList* ifBody;
       
       ifStmt(Expression* condition, StatementList* ifBody): condition(condition), ifBody(ifBody){}
        void accept(Visitor& v);
};*/
class ifStmt: public SelectionStmt{
    public:
        Expression* condition; 
        StatementList* ifBody;
        SelectStatementList* elseIfBranch;
        StatementList* elseBody;
       
       ifStmt(Expression* condition, StatementList* ifBody, SelectStatementList* elseIfBranch, StatementList* elseBody): condition(condition), ifBody(ifBody), elseIfBranch(elseIfBranch), elseBody(elseBody){}
        void accept(Visitor& v);
};
class elseIfStmt: public SelectionStmt{
    public:
        Expression* elseIfCondition; 
        StatementList* elseIfBody;
        
        elseIfStmt(Expression* elseIfCondition, StatementList* elseIfBody): elseIfCondition(elseIfCondition), elseIfBody(elseIfBody){}
        void accept(Visitor& v);
};
/*class ifElseStmt: public SelectionStmt{
    public:
        Expression* condition; 
        StatementList* ifBody;
        StatementList* elseBody;
        
        ifElseStmt(Expression* condition, StatementList* ifBody, StatementList* elseBody): condition(condition), ifBody(ifBody), elseBody(elseBody){}
        void accept(Visitor& v);
};

class ifElseIfStmt: public SelectionStmt{
    // Need to change it to be ifElse statement only. Do it C-style
    public:
        Expression* ifcondition; 
        Expression* elsecondition; 
        StatementList* ifBody;
        StatementList* elseBody;
        
        ifElseIfStmt(Expression* ifcondition, StatementList* ifBody, Expression* elsecondition, StatementList* elseBody): ifcondition(ifcondition), ifBody(ifBody), elsecondition(elsecondition),elseBody(elseBody){}
        void accept(Visitor& v);
};
*/
class LoopStatement: public Statement{
   public:
};
class ForStmt: public LoopStatement{
    public:
        
        TypeDecl* loopInit;
        Expression* loopCondition;
        Expression* loopIteration;
        StatementList* loopBody;
        
        ForStmt(TypeDecl* loopInit, Expression* loopCondition,Expression* loopIteration, StatementList* loopBody): loopInit(loopInit), loopCondition(loopCondition), loopIteration(loopIteration), loopBody(loopBody){}
        
        void accept(Visitor& v);
};
class WhileStmt: public LoopStatement{
       public:
        Expression* loopCondition;
        StatementList* loopBody;
        
        WhileStmt(Expression* loopCondition,StatementList* loopBody): loopCondition(loopCondition), loopBody(loopBody){}
        
        void accept(Visitor& v);
};
/**********************Types and Type Decl ***********************/
class TypeDecl: public Expression{
    public:
};
class TypeDeclStmt: public TStatement{
    public: 
        TypeDecl* texpr;
        TypeDeclStmt(TypeDecl* texpr): texpr(texpr){}
        void accept(Visitor& v);
};
/*class TypeDecl: public ASTnode{
    public:
};*/
/*class TypeDeclList: public TypeDecl{
    public: 
        //typeDeclList* typedecls;
        typeDeclList typedecls;
        TypeDeclList(){ }
        void accept(Visitor& v);
};*/
class VType: public ASTnode{
    public:
};
class VTypeList: public VType{
        public:
        vtypelist typelist;
        VTypeList(){};
        void accept(Visitor& v);
};
/*class PrimitiveType: public VType{
        public:      
};*/
class PrimitiveType: public VType{
        public:  
        int type;
        PrimitiveType(int type): type(type) {}
        void accept(Visitor& v);
};
/*class PrimitiveTypeList: public PrimitiveType{
        public:
        primitiveTypeList* typelist;
        PrimitiveTypeList(){}
};*/
class CompositeType: public VType{
        public:
};
class ArrayType: public CompositeType{
        public:
        int size;
        VType* type; /*could be primitive or composite*/
        ArrayType(VType* type, int size): type(type), size(size){}
        void accept(Visitor& v);
};
class PairType: public CompositeType{
        public:
        PrimitiveType* type1;  /*or should we do primitiveType*/
        PrimitiveType* type2;
        PairType(PrimitiveType* type1, PrimitiveType* type2): type1(type1), type2(type2){}
        void accept(Visitor& v);
};
class DictType: public CompositeType{
        public:
        PrimitiveType* keytype;
        VType* valuetype;
        DictType(PrimitiveType* keytype, VType* valuetype): keytype(keytype), valuetype(valuetype){}
        void accept(Visitor& v);
};
class VectType: public CompositeType{
        public:
        PrimitiveType* etype;
        VectType(PrimitiveType* etype): etype(etype){}
        void accept(Visitor& v);
};
class SetType: public CompositeType{
        public:
        PrimitiveType* etype;
        SetType(PrimitiveType* etype): etype(etype){}
        void accept(Visitor& v);
};
class TupleType: public CompositeType{
        public:
        VTypeList* typelist;
        //PrimitiveTypeList* typelist;
        //TupleType(PrimitiveTypeList* typelist): typelist(typelist){}
        TupleType(VTypeList* typelist): typelist(typelist){}
        void accept(Visitor& v);
};
class PrimitiveTypeDecl: public TypeDecl{
        public:
        PrimitiveType* type;
        Identifier* name;
        Expression* init;

        PrimitiveTypeDecl(PrimitiveType* type, Identifier* name): type(type), name(name){ /*init= new Expression();*//*init undefined*/}
        PrimitiveTypeDecl(PrimitiveType* type, Identifier* name, Expression* init): type(type), name(name), init(init){}
        void accept(Visitor& v);
};
class FileTypeDecl: public TypeDecl{
   public: 
        Identifier* ident;
        Identifier* f_var;
        StrConst* f_name;
        //FileTypeDecl(Identifier* ident, Identifier* f_var): ident(ident), f_var(f_var){f_name = NULL;}
        FileTypeDecl(Identifier* ident): ident(ident){f_var = NULL; f_name = NULL;}
        FileTypeDecl(Identifier* ident, StrConst* f_name): ident(ident), f_name(f_name) {f_var = NULL;}
        //FileTypeDecl(Identifier* ident, StrConst* f_name): ident(ident), f_name(f_name) {f_var = NULL;}
        void accept(Visitor& v);
        
};
class CompositeTypeDecl: public TypeDecl{
    public:
        CompositeType*  type;
        Identifier*      name;
        ExpressionList*  init_list;
        CompositeTypeDecl(CompositeType* type, Identifier* name): type(type), name(name){
         /*init undefined*/
         init_list = new ExpressionList();
        }
        CompositeTypeDecl(CompositeType* type, Identifier* name, ExpressionList* init_list): type(type), name(name), init_list(init_list){}
        void accept(Visitor& v);

};



/********************Command********************************/
class Node: public ASTnode{
    public:
};
class NodeList: public Node{
    public:
    nodelist   nodes;
    NodeList(){}
    void accept(Visitor& v);
};

class CommandBlock: public Node{
    public:
        Component* component_type;
        TypeDeclStmtList* localdeclarations;
        //TypeDeclList* localdeclarations;
        NodeList* actions;
        StatementList* stmts;
        Expression* cond; 
        NodeList* inner_blocks;
        
        CommandBlock(){}
        CommandBlock(Component* component_type, Expression* cond, TypeDeclStmtList* localdeclarations, StatementList* stmts, NodeList* actions, NodeList* inner_blocks): component_type(component_type), cond(cond), localdeclarations(localdeclarations), stmts(stmts), actions(actions), inner_blocks(inner_blocks){}
          
          //the one to be skipped should be the last one. or it could be completely empty
        //CommandBlock(Component* component_type, NodeList* commandlist): component_type(component_type), commandlist(commandlist){}
        void accept(Visitor& v);

};
class Action: public Node{
        public:
        int trigger;
        Identifier* name;
        Expression* cond;
        TypeDeclStmtList* dyn_decls;
        StatementList* stmts;
        //Action(int trigger, TypeDeclList* dyn_decls, StatementList* stmts): trigger(trigger), dyn_decls(dyn_decls), stmts(stmts){}
        Action(int trigger, Identifier* name, Expression* cond, TypeDeclStmtList* dyn_decls, StatementList* stmts): trigger(trigger), name(name), cond(cond), dyn_decls(dyn_decls), stmts(stmts){}
        void accept(Visitor& v);
};
/************************Program - root node***********************/
class ProgramBlock: public Node{
        public:
        //TypeDeclList* globaldeclarations;
        TypeDeclStmtList* globaldeclarations;
        NodeList*    commandblocks;

        StatementList* exit_stmts;
        StatementList* init_stmts;
        /*ProgramBlock(NodeList* commandblocks, TypeDeclList* globaldeclarations): commandblocks(commandblocks), globaldeclarations(globaldeclarations){}
        
        ProgramBlock(NodeList* commandblocks): commandblocks(commandblocks){
               // globaldeclarations = new TypeDeclList(); 
        }*/
        ProgramBlock(NodeList* commandblocks, TypeDeclStmtList* globaldeclarations, StatementList* init_stmts, StatementList* exit_stmts): commandblocks(commandblocks), globaldeclarations(globaldeclarations), init_stmts(init_stmts), exit_stmts(exit_stmts){}
        
        /*ProgramBlock(NodeList* commandblocks, StatementList* exit_stmts): commandblocks(commandblocks), exit_stmts(exit_stmts){
               // globaldeclarations = new TypeDeclList(); 
        }*/
        void accept(Visitor& v);
};
/**************************Component**********************/
class Component: public Expression{
        public:
        int type;
        Identifier* name;
        Component(int type, Identifier* name): type(type),name(name){}
        void accept(Visitor& v);
};
#endif
