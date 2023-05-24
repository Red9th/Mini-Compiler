// 所有头文件都只 include 一次
#pragma once

#include <memory>
#include <iostream>
#include <unordered_map>

static int now = 0;
// 常量符号表，将一个常量名映射到其值(32 位整数)
static std::unordered_map<std::string, int> const_int_val;
// 常量对应的 koopa 存放位置
static std::unordered_map<std::string, int> const_int_pos;

// 所有 AST 的基类
class BaseAST {
  public:
    virtual ~BaseAST() = default;

    virtual std::string Dump() const = 0;
    virtual int Calc() {
      return 0;
    }
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
  public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    std::string Dump() const override {
      return func_def->Dump();
    }
};

class DeclAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> const_decl;

    std::string Dump() const override {
      return const_decl->Dump();
    }
};

class ConstDeclAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<BaseAST> const_def;

    std::string Dump() const override {
      return const_def->Dump();
    }
};

class BTypeAST : public BaseAST {
  public:
    std::string type;

    std::string Dump() const override {
      return "";
    }
};

class ConstDefAST : public BaseAST {
  public:
    std::string ident;
    std::unique_ptr<BaseAST> constInitVal;
    std::unique_ptr<BaseAST> const_def;

    std::string Dump() const override {
      const_int_val[ident] = constInitVal->Calc();
      std::cout << "  %" << now << " = add 0, " << const_int_val[ident] << std::endl;
      const_int_pos[ident] = now;
      now ++;
      if(const_def != NULL) {
        const_def->Dump();
      }
      return "";
    }
};

class ConstInitValAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> const_exp;

    std::string Dump() const override {
      return "";
    }

    int Calc() override {
      return const_exp->Calc();
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    std::string Dump() const override {
      std::cout << "fun ";
      std::cout << "@" << ident << "(): ";
      func_type->Dump();
      std::cout << "{ " << std::endl;
      block->Dump();
      std::cout << "} " << std::endl;
      return "";
    }
};

// ...
class FuncType : public BaseAST {
  public:
    std::string _int;

    std::string Dump() const override {
      std::cout << "i32 ";
      return "";
    }
};

class BlockAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> block_item;

    std::string Dump() const override {
      std::cout << "\%entry" << ": " << std::endl;
      block_item->Dump();
      std::cout << std::endl;
      return "";
    }
};

class BlockItemAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> block_item;

    std::string Dump() const override {
      if(decl != NULL) {
        decl->Dump();
      } else if(stmt != NULL) {
        stmt->Dump();
      }
      if(block_item != NULL) {
        block_item->Dump();
      }
      return "";
    }
};

class StmtAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    std::string Dump() const override {
      std::string res = exp->Dump();
      std::cout << "  ret " << res;
      return "";
    }
};

class ExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    std::string Dump() const override {
      return exp->Dump();
    }

    int Calc() override {
      return exp->Calc();
    }
};

class LValAST : public BaseAST {
  public:
    std::string ident;

    std::string Dump() const override {
      std::string res = "%" + std::to_string(const_int_pos[ident]);
      return res;
    }

    int Calc() override {
      return const_int_val[ident];
    }
};

class PrimaryExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> p_exp;

    std::string Dump() const override {
      return p_exp->Dump();
    }

    int Calc() override {
      return p_exp->Calc();
    }
};

class NumberAST : public BaseAST {
  public:
    int val;

    std::string Dump() const override {
      std::cout << "  %" << now << " = add 0, " << val << std::endl;
      std::string res = "%" + std::to_string(now);
      now ++;
      return res;
    }

    int Calc() override {
      return val;
    }
};

class UnaryExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> u_exp;
    std::string op_ident;

    std::string Dump() const override {
      std::string res, r;
      if(op_ident == "" || op_ident == "+") {
        res = u_exp->Dump();
      } else {
        r = u_exp->Dump();
        if(op_ident == "-") {
          std::cout << "  %" << now << " = sub 0, " << r << std::endl;
        } else if(op_ident == "!") {
          std::cout << "  %" << now << " = eq 0, " << r << std::endl;
        }
        res = "%" + std::to_string(now);
        now ++;
      }
      return res;
    }

    int Calc() override {
      if(op_ident == "") {
        return u_exp->Calc();
      } else if(op_ident == "+") {
        return u_exp->Calc();
      } else if(op_ident == "-") {
        return -u_exp->Calc();
      } else if(op_ident == "!") {
        return !u_exp->Calc();
      }
      return 0;
    }
};

class MulExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> unary_exp;

    std::string Dump() const override {
      std::string l, r, res;
      if(op_ident == "") {
        res = unary_exp->Dump();
      } else {
        l = mul_exp->Dump();
        r = unary_exp->Dump();
        if(op_ident == "*") {
          std::cout << "  %" << now << " = mul " << l << ", " << r << std::endl;
        } else if(op_ident == "/") {
          std::cout << "  %" << now << " = div " << l << ", " << r << std::endl;
        } else if(op_ident == "%") {
          std::cout << "  %" << now << " = mod " << l << ", " << r << std::endl;
        }
        res = "%" + std::to_string(now);
        now ++;
      }
      return res;
    }

    int Calc() override {
      if(op_ident == "") {
        return unary_exp->Calc();
      } else if(op_ident == "*") {
        return mul_exp->Calc() * unary_exp->Calc();
      } else if(op_ident == "/") {
        return mul_exp->Calc() / unary_exp->Calc();
      } else if(op_ident == "%") {
        return mul_exp->Calc() % unary_exp->Calc();
      }
      return 0;
    }
};

class AddExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> mul_exp;

    std::string Dump() const override {
      std::string l, r, res;
      if(op_ident == "") {
        res = mul_exp->Dump();
      } else {
        l = add_exp->Dump();
        r = mul_exp->Dump();
        if(op_ident == "+") {
          std::cout << "  %" << now << " = add " << l << ", " << r << std::endl;
        } else if(op_ident == "-") {
          std::cout << "  %" << now << " = sub " << l << ", " << r << std::endl;
        }
        res = "%" + std::to_string(now);
        now ++;
      }
      return res;
    }

    int Calc() override {
      if(op_ident == "") {
        return mul_exp->Calc();
      } else if(op_ident == "+") {
        return add_exp->Calc() + mul_exp->Calc();
      } else if(op_ident == "-") {
        return add_exp->Calc() - mul_exp->Calc();
      }
      return 0;
    }
};

class RelExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> add_exp;

    std::string Dump() const override {
      std::string l, r, res;
      if(op_ident == "") {
        res = add_exp->Dump();
      } else {
        l = rel_exp->Dump();
        r = add_exp->Dump();
        if(op_ident == "<") {
          std::cout << "  %" << now << " = lt " << l << ", " << r << std::endl;
        } else if(op_ident == ">") {
          std::cout << "  %" << now << " = gt " << l << ", " << r << std::endl;
        } else if(op_ident == "<=") {
          std::cout << "  %" << now << " = le " << l << ", " << r << std::endl;
        } else if(op_ident == ">=") {
          std::cout << "  %" << now << " = ge " << l << ", " << r << std::endl;
        }
        res = "%" + std::to_string(now);
        now ++;
      }
      return res;
    }

    int Calc() override {
      if(op_ident == "") {
        return add_exp->Calc();
      } else if(op_ident == "<") {
        return rel_exp->Calc() < add_exp->Calc();
      } else if(op_ident == ">") {
        return rel_exp->Calc() > add_exp->Calc();
      } else if(op_ident == "<=") {
        return rel_exp->Calc() <= add_exp->Calc();
      } else if(op_ident == ">=") {
        return rel_exp->Calc() >= add_exp->Calc();
      }
      return 0;
    }
};

class EqExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> rel_exp;

    std::string Dump() const override {
      std::string l, r, res;
      if(op_ident == "") {
        res = rel_exp->Dump();
      } else {
        l = eq_exp->Dump();
        r = rel_exp->Dump();
        if(op_ident == "==") {
          std::cout << "  %" << now << " = eq " << l << ", " << r << std::endl;
        } else if(op_ident == "!=") {
          std::cout << "  %" << now << " = ne " << l << ", " << r << std::endl;
        }
        res = "%" + std::to_string(now);
        now ++;
      }
      return res;
    }

    int Calc() override {
      if(op_ident == "") {
        return rel_exp->Calc();
      } else if(op_ident == "==") {
        return eq_exp->Calc() == rel_exp->Calc();
      } else if(op_ident == "!=") {
        return eq_exp->Calc() != rel_exp->Calc();
      }
      return 0;
    }
};

class LAndExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;

    std::string Dump() const override {
      std::string l, r, res;
      if(op_ident == "") {
        res = eq_exp->Dump();
      } else if(op_ident == "&&") {
        l = land_exp->Dump();
        r = eq_exp->Dump();
        std::cout << "  %" << now ++ << " = ne 0" << ", " << l << std::endl;
        std::cout << "  %" << now ++ << " = ne 0" << ", " << r << std::endl;
        std::cout << "  %" << now << " = add %" << now - 1 << ", %" << now - 2 << std::endl;
        std::cout << "  %" << now + 1 << " = eq 2" << ", %" << now << std::endl;
        res = "%" + std::to_string(now + 1);
        now += 2;
      }
      return res;
    }

    int Calc() override {
      if(op_ident == "") {
        return eq_exp->Calc();
      } else if(op_ident == "&&") {
        return land_exp->Calc() && eq_exp->Calc();
      }
      return 0;
    }
};

class LOrExpAST : public BaseAST {
  public:
    std::string op_ident;
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;

    std::string Dump() const override {
      std::string l, r, res;
      if(op_ident == "") {
        res = land_exp->Dump();
      } else if(op_ident == "||") {
        l = lor_exp->Dump();
        r = land_exp->Dump();
        std::cout << "  %" << now ++ << " = ne 0" << ", " << l << std::endl;
        std::cout << "  %" << now ++ << " = ne 0" << ", " << r << std::endl;
        std::cout << "  %" << now << " = add %" << now - 1 << ", %" << now - 2 << std::endl;
        std::cout << "  %" << now + 1 << " = ne 0" << ", %" << now << std::endl;
        res = "%" + std::to_string(now + 1);
        now += 2;
      }
      return res;
    }

    int Calc() override {
      if(op_ident == "") {
        return land_exp->Calc();
      } else if(op_ident == "||") {
        return lor_exp->Calc() || land_exp->Calc();
      }
      return 0;
    }
};

class ConstExpAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    std::string Dump() const override {
      return exp->Dump();
    }

    int Calc() override {
      return exp->Calc();
    }
};