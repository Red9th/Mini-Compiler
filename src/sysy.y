// 把大括号里的内容塞到 Bison 生成的头文件里
// Bison 生成的头文件中主要是 parser 函数的定义, 和 yylval 的定义
%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

// 把括号中的内容塞到 Bison 生成的源文件里
%{

#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include <vector>
#include "ast.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
// 后续定义 AST 后，不再生成和源码相同的字符串了，而是要生成一个 AST, 所以这里需要修改, 其他相关声明也应该被修改
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST EQUAL_OR_LESSER EQUAL_OR_GREATER EQUAL NOT_EQUAL AND OR
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt Exp UnaryExp PrimaryExp Number MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <ast_val> Decl ConstDecl VarDecl BType ConstDef VarDef InitVal ConstInitVal BlockItem LVal
%type <str_val> UnaryOp

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | VarDecl {
    auto ast = new DeclAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDef ';' {
    auto ast = new ConstDeclAST();
    ast->btype = unique_ptr<BaseAST>($2);
    ast->const_def = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
    ast->type = *new string("int");
    $$ = ast;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->constInitVal = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | IDENT '=' ConstInitVal ',' ConstDef {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->constInitVal = unique_ptr<BaseAST>($3);
    ast->const_def = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDef ';' {
    auto ast = new VarDeclAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->var_def = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  } | IDENT ',' VarDef {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->var_def = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | IDENT '=' InitVal ',' VarDef {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    ast->var_def = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncType();
    auto i = new string("int");
    ast->_int = *unique_ptr<string>(i);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItem '}' {
    auto ast = new BlockAST();
    ast->block_item = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

BlockItem
  : {
    auto ast = new BlockItemAST();
    ast->decl = NULL;
    ast->stmt = NULL;
    ast->block_item = NULL;
    $$ = ast;
  } | Decl BlockItem {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    ast->stmt = NULL;
    ast->block_item = unique_ptr<BaseAST>($2);
    $$ = ast;
  } | Stmt BlockItem {
    auto ast = new BlockItemAST();
    ast->decl = NULL;
    ast->stmt = unique_ptr<BaseAST>($1);
    ast->block_item = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = 0;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = 1;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->p_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  } | LVal {
    auto ast = new PrimaryExpAST();
    ast->p_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | Number {
    auto ast = new PrimaryExpAST();
    ast->p_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->val = $1;
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->op_ident = *new string("");
    ast->u_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op_ident = *unique_ptr<string>($1);
    ast->u_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    $$ = new string("+");
  } | '-' {
    $$ = new string("-");
  } | '!' {
    $$ = new string("!");
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->op_ident = *new string("");
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->op_ident = *new string("*");
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->op_ident = *new string("/");
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->op_ident = *new string("%");
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->op_ident = *new string("");
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->op_ident = *new string("+");
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->op_ident = *new string("-");
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->op_ident = *new string("");
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | RelExp '<' AddExp {
    auto ast = new RelExpAST();
    ast->op_ident = *new string("<");
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | RelExp '>' AddExp {
    auto ast = new RelExpAST();
    ast->op_ident = *new string(">");
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | RelExp EQUAL_OR_LESSER AddExp {
    auto ast = new RelExpAST();
    ast->op_ident = *new string("<=");
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | RelExp EQUAL_OR_GREATER AddExp {
    auto ast = new RelExpAST();
    ast->op_ident = *new string(">=");
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->op_ident = *new string("");
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | EqExp EQUAL RelExp {
    auto ast = new EqExpAST();
    ast->op_ident = *new string("==");
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } | EqExp NOT_EQUAL RelExp {
    auto ast = new EqExpAST();
    ast->op_ident = *new string("!=");
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->op_ident = *new string("");
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | LAndExp AND EqExp {
    auto ast = new LAndExpAST();
    ast->op_ident = *new string("&&");
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->op_ident = *new string("");
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } | LOrExp OR LAndExp {
    auto ast = new LOrExpAST();
    ast->op_ident = *new string("||");
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  extern int yylineno;    // defined and maintained in lex
  extern char *yytext;    // defined and maintained in lex
  int len = strlen(yytext);
  int i;
  char buf[512] = {0};
  for (i = 0; i < len; i ++){
    sprintf(buf, "%s%d ", buf, yytext[i]);
  }
  fprintf(stderr, "ERROR: %s at symbol '%s' on line %d\n", s, buf, yylineno);
}
