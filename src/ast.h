// 所有头文件都只 include 一次
#pragma once

#include <memory>
#include <iostream>

static int now = 0;

// 所有 AST 的基类
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  virtual void DumpAST() const = 0;
  virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
 public:
  // 用智能指针管理对象
  std::unique_ptr<BaseAST> func_def;

  void DumpAST() const override {
    std::cout << "CompUnitAST { ";
    func_def->DumpAST();
    std::cout << " }";
  }

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

  void DumpAST() const override {
    std::cout << "FuncDefAST { ";
    func_type->DumpAST();
    std::cout << ", " << ident << ", ";
    block->DumpAST();
    std::cout << " }";
  }

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

    void DumpAST() const override {
      std::cout << "FuncTypeAST { ";
      std::cout << _int;
      std::cout << " }";
    }

    void Dump() const override {
      std::cout << "i32 ";
    }
};

class BlockAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> stmt;

    void DumpAST() const override {
      std::cout << "BlockAST { ";
      stmt->DumpAST();
      std::cout << " }";
    }

    void Dump() const override {
      std::cout << "\%entry" << ": " << std::endl;
      stmt->Dump();
      std::cout << std::endl;
    }
};

class StmtAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    void DumpAST() const override { }

    void Dump() const override {
      exp->Dump();
      std::cout << "  ret %" << now;
    }
};

class ExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> unary_exp;

    void DumpAST() const override { }

    void Dump() const override {
      unary_exp->Dump();
    }
};

class PrimaryExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> p_exp;

    void DumpAST() const override { }

    void Dump() const override {
      p_exp->Dump();
    }
};

class NumberAST : public BaseAST {
  public:
    int val;

    void DumpAST() const override { }

    void Dump() const override {
      std::cout << "  %" << now << " = add 0, " << val << std::endl;
    }
};

class UnaryExpAST : public BaseAST {
  public:
    int type;
    std::unique_ptr<BaseAST> u_exp;
    std::string op_ident;

    void DumpAST() const override { }

    void Dump() const override {
      if(type == 0) {
        u_exp->Dump();
      } else if(type == 1) {
        u_exp->Dump();
        if(op_ident == "-") {
          std::cout << "  %" << now + 1 << " = sub 0, %" << now ++ << std::endl;
        } else if(op_ident == "!") {
          std::cout << "  %" << now + 1 << " = eq 0, %" << now ++ << std::endl;
        }
      }
    }
};