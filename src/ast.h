// 所有头文件都只 include 一次
#pragma once

#include <memory>
#include <iostream>

static int now = 0;

// 所有 AST 的基类
class BaseAST {
  public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
  public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
      func_def->Dump();
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
      std::cout << "fun ";
      std::cout << "@" << ident << "(): ";
      func_type->Dump();
      std::cout << "{ " << std::endl;
      block->Dump();
      std::cout << "} " << std::endl;
    }
};

// ...
class FuncType : public BaseAST {
  public:
    std::string _int;

    void Dump() const override {
      std::cout << "i32 ";
    }
};

class BlockAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
      std::cout << "\%entry" << ": " << std::endl;
      stmt->Dump();
      std::cout << std::endl;
    }
};

class StmtAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
      exp->Dump();
      std::cout << "  ret %" << now - 1;
    }
};

class ExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
      exp->Dump();
    }
};

class PrimaryExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> p_exp;

    void Dump() const override {
      p_exp->Dump();
    }
};

class NumberAST : public BaseAST {
  public:
    int val;

    void Dump() const override {
      std::cout << "  %" << now << " = add 0, " << val << std::endl;
      now ++;
    }
};

class UnaryExpAST : public BaseAST {
  public:
    int type;
    std::unique_ptr<BaseAST> u_exp;
    std::string op_ident;

    void Dump() const override {
      if(type == 0) {
        u_exp->Dump();
      } else if(type == 1) {
        u_exp->Dump();
        if(op_ident == "-") {
          std::cout << "  %" << now << " = sub 0, %" << now - 1 << std::endl;
          now ++;
        } else if(op_ident == "!") {
          std::cout << "  %" << now << " = eq 0, %" << now - 1 << std::endl;
          now ++;
        }
      }
    }
};

class MulExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> unary_exp;

    void Dump() const override {
      int l, r;
      if(op_ident == "") {
        unary_exp->Dump();
      } else {
        mul_exp->Dump();
        l = now - 1;
        unary_exp->Dump();
        r = now - 1;
        if(op_ident == "*") {
          std::cout << "  %" << now << " = mul %" << l << ", %" << r<< std::endl;
        } else if(op_ident == "/") {
          std::cout << "  %" << now << " = div %" << l << ", %" << r << std::endl;
        } else if(op_ident == "%") {
          std::cout << "  %" << now << " = mod %" << l << ", %" << r << std::endl;
        }
        now ++;
      }
    }
};

class AddExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> mul_exp;

    void Dump() const override {
      int l, r;
      if(op_ident == "") {
        mul_exp->Dump();
      } else {
        add_exp->Dump();
        l = now - 1;
        mul_exp->Dump();
        r = now - 1;
        if(op_ident == "+") {
          std::cout << "  %" << now << " = add %" << l << ", %" << r << std::endl;
        } else if(op_ident == "-") {
          std::cout << "  %" << now << " = sub %" << l << ", %" << r << std::endl;
        }
        now ++;
      }
    }
};