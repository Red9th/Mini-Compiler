#include <iostream>
#include <cassert>
#include <stdio.h>
#include <unordered_map>
#include "koopa.h"

// 函数声明

// 访问 raw program
void Visit(const koopa_raw_program_t &program);
// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice);
// 访问函数
void Visit(const koopa_raw_function_t &func);
// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb);
// 访问指令
void Visit(const koopa_raw_value_t &value);
// 访问 return
void Visit(const koopa_raw_return_t &ret);
// 访问 int
void Visit(const koopa_raw_integer_t &i32);
// 访问 binary
void Visit(const koopa_raw_binary_t &bin);

// 全局变量

// 某条指令执行完毕之后的 rd 编号
static int cur;
// 访问 integer 后得到的值
std::string int_res;
// 将一条指令与存储其结果的寄存器对应起来
std::unordered_map<koopa_raw_value_t, std::string> map;

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  printf("  .text\n");
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  printf("  .globl %s\n", func->name + 1);
  printf("%s:\n", func->name + 1);
  // 执行一些其他的必要操作
  // ...
  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary);
      map[value] = "t" + std::to_string(cur);
      cur ++;
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 访问 return
void Visit(const koopa_raw_return_t &ret) {
  if(ret.value->kind.tag == KOOPA_RVT_INTEGER) {
    std::cout << "  li    a0, " << ret.value << std::endl;
    std::cout << "  ret" << std::endl;
  } else {
    std::string rd = "t" + std::to_string(cur - 1);
    std::cout << "  mv    a0, " << rd << std::endl;
    std::cout << "  ret" << std::endl;
  }
}

// 访问 int
void Visit(const koopa_raw_integer_t &i32) {
  if(i32.value == 0) {
    int_res = "x0";
  } else {
    std::string rd = "t" + std::to_string(cur);
    std::cout << "  li    " << rd << ", " << i32.value << std::endl;
    int_res = "t" + std::to_string(cur);
  }
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &bin) {
  std::string rd, rs1, rs2;
  if(bin.lhs->kind.tag == KOOPA_RVT_INTEGER) {
    Visit(bin.lhs);
    rs1 = int_res;
  } else {
    rs1 = map[bin.lhs];
  }
  if(bin.rhs->kind.tag == KOOPA_RVT_INTEGER) {
    Visit(bin.rhs);
    rs2 = int_res;
  } else {
    rs2 = map[bin.rhs];
  }
  rd = "t" + std::to_string(cur);
  switch (bin.op) {
    case KOOPA_RBO_NOT_EQ:
      std::cout << "  xor   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      std::cout << "  snez  " << rd << ", " << rd << std::endl;
      break;
    case KOOPA_RBO_EQ:
      std::cout << "  xor   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      std::cout << "  seqz  " << rd << ", " << rd << std::endl;
      break;
    case KOOPA_RBO_GT:
      std::cout << "  sgt   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      break;
    case KOOPA_RBO_LT:
      std::cout << "  slt   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      break;
    case KOOPA_RBO_GE:
      std::cout << "  slt   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      std::cout << "  seqz  " << rd << ", " << rd << std::endl;
      break;
    case KOOPA_RBO_LE:
      std::cout << "  sgt   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      std::cout << "  seqz  " << rd << ", " << rd << std::endl;
      break;
    case KOOPA_RBO_ADD:
      std::cout << "  add   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      break;
    case KOOPA_RBO_SUB:
      std::cout << "  sub   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      break;
    case KOOPA_RBO_MUL:
      std::cout << "  mul   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      break;
    case KOOPA_RBO_DIV:
      std::cout << "  div   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      break;
    case KOOPA_RBO_MOD:
      std::cout << "  rem   " << rd << ", " << rs1 << ", " << rs2 << std::endl;
      break;
    default:
      assert(false);
  }
}

// 解析字符串 str, 得到 Koopa IR 的内存表示
void parse_string_to_koopa(const char* str) {
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(str, &program);
  assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);

  // 处理 raw program
  Visit(raw);

  // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);
}