// 所有头文件都只 include 一次
#pragma once

#include <memory>
#include <iostream>
#include <unordered_map>
#include <vector>

/* 全局变量 */

static int now = 0;
// 作用域的个数
const int SCOPE_SIZE = 1000000;
// 标识符类型表，确定某一个标识符是常量(0)/变量(1)/...
static std::unordered_map<std::string, int> ident_type[SCOPE_SIZE];

// 符号表部分

// f[i] 表示作用域 i 的上一层作用域的标号
static int f[SCOPE_SIZE];
// 表示全部的作用域个数
static int deep;
// 表示当前处于哪个作用域
static int cur_deep;
// 标识符符号表，会保存作用域中常量的值
static std::unordered_map<std::string, int> ident_val[SCOPE_SIZE];

/* 函数声明 */

static int find_ident_depth(int deep, std::string ident);

// 所有 AST 的基类
class BaseAST {
  public:
    virtual ~BaseAST() = default;

    virtual std::string Dump() const = 0;
    virtual int Calc() {
      return 0;
    }
    virtual std::string get_ident() {
      return "";
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
    std::unique_ptr<BaseAST> decl;

    std::string Dump() const override {
      return decl->Dump();
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
      ident_val[cur_deep][ident] = constInitVal->Calc();
      ident_type[cur_deep][ident] = 0;
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

class VarDeclAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<BaseAST> var_def;

    std::string Dump() const override {
      return var_def->Dump();
    }
};

class VarDefAST : public BaseAST {
  public:
    std::string ident;
    std::unique_ptr<BaseAST> init_val;
    std::unique_ptr<BaseAST> var_def;

    std::string Dump() const override {
      std::string cur_ident = ident + "_" + std::to_string(cur_deep);
      std::cout << "  @" << cur_ident << " = alloc i32" << std::endl;
      ident_type[cur_deep][ident] = 1;
      if(init_val != NULL) {
        std::string res = init_val->Dump();
        std::cout << "  store " << res << ", @" << cur_ident << std::endl;
      }
      if(var_def != NULL) {
        var_def->Dump();
      }
      return "";
    }
};

class InitValAST : public BaseAST {
  public:
    std::unique_ptr<BaseAST> exp;

    std::string Dump() const override {
      return exp->Dump();
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
      std::cout << "\%entry" << ": " << std::endl;
      block->Dump();
      std::cout << std::endl;
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
      // 进入代码块时新建一个符号表，作为当前的符号表
      deep ++;
      f[deep] = cur_deep;
      cur_deep = deep;
      block_item->Dump();
      // 退出代码块时删除刚刚创建的符号表
      ident_val[cur_deep].clear();
      cur_deep = f[cur_deep];
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
    int type;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;

    std::string Dump() const override {
      std::string ident, res;
      int depth;
      if(type == 0) {
        depth = find_ident_depth(cur_deep, lval->get_ident());
        if(depth != -1) {
          ident = lval->get_ident() + "_" + std::to_string(depth);
          res = exp->Dump();
          std::cout << "  store " << res << ", @" << ident << std::endl;
        } else {
          // 抛出异常: 未定义的标识符
        }
      } else if(type == 2) {
        exp->Dump();
      } else if(type == 3) {
        block->Dump();
      } else if(type == 5) {
        res = exp->Dump();
        std::cout << "  ret " << res;
      }
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
      std::string res;
      int depth = find_ident_depth(cur_deep, ident);
      if(depth != -1) {
        std::string cur_ident = ident + "_" + std::to_string(depth);
        if(ident_type[depth][ident] == 0) {
          res = std::to_string(ident_val[depth][ident]);
        } else if(ident_type[depth][ident] == 1) {
          std::cout << "  %" << now << " = load @" << cur_ident << std::endl;
          res = "%" + std::to_string(now);
          now ++;
        }
      } else {
        // 抛出异常: 未定义的标识符
      }
      return res;
    }

    int Calc() override {
      int depth = find_ident_depth(cur_deep, ident);
      if(depth != -1) {
        if(ident_type[depth][ident] == 0) {
          return ident_val[depth][ident];
        }
      } else {
        // 抛出异常: 未定义的标识符
      }
      return 0;
    }

    std::string get_ident() override {
      return ident;
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
      return std::to_string(val);
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

static int find_ident_depth(int deep, std::string ident) {
  if(deep == 0) {
    return -1;
  }
  if(ident_type[deep].find(ident) != ident_type[deep].end()) {
    return deep;
  } else {
    return find_ident_depth(f[deep], ident);
  }
}