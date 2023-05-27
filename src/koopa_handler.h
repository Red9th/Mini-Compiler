#include <iostream>
#include <cassert>
#include <stdio.h>
#include <unordered_map>
#include "koopa.h"

/* 函数声明 */

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
// 访问 load
void Visit(const koopa_raw_load_t &load);
// 访问 store
void Visit(const koopa_raw_store_t &store);

// 计算需要分配的栈空间总量(单位：字节)
int cal_alloc_size(const koopa_raw_slice_t &slice);
int cal_alloc_size(const koopa_raw_function_t &func);
int cal_alloc_size(const koopa_raw_basic_block_t &bb);
int cal_alloc_size(const koopa_raw_value_t &value);

// 输出 lw/sw 指令
void dump_lw_sw(std::string rs1, std::string rs2, int offset, std::string type);

/* 全局变量 */

// 某条指令执行完毕之后的 rd 编号
static int cur;
// 访问 integer 后得到的值
std::string int_res;
// 记录函数分配的内存大小
static int alloc_size;
// 记录当前栈顶的偏移量
static int offset;
// 记录 koopa 指令对应的栈帧偏移量
std::unordered_map<koopa_raw_value_t, int> value_offset;

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
  // 计算程序中需要分配的栈空间总量
  alloc_size = cal_alloc_size(func);
  // 将栈空间总量对齐到 16
  alloc_size = (alloc_size + 15) & ~15;
  // 函数的 prologue
  if(-alloc_size >= -2048 && -alloc_size <= 2047) {
    std::cout << "  addi  sp, sp, " << -alloc_size << std::endl;
  } else {
    std::cout << "  li    t" << cur << ", " << -alloc_size << std::endl;
    std::cout << "  add   sp, sp, t0" << std::endl;
  }
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
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_ALLOC:
      break;
    case KOOPA_RVT_LOAD:
      // 访问 load 指令
      Visit(kind.data.load);
      break;
    case KOOPA_RVT_STORE:
      // 访问 store 指令
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BINARY:
      // 访问 binary 指令
      Visit(kind.data.binary);
      break;
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    value_offset[value] = offset;
    offset += 4;
  }
  cur = 0;
  std::cout << std::endl;
}

// 访问 return
void Visit(const koopa_raw_return_t &ret) {
  if(ret.value->kind.tag == KOOPA_RVT_INTEGER) {
    std::cout << "  li    a0, " << ret.value->kind.data.integer.value << std::endl;
    std::cout << "  ret" << std::endl;
  } else {
    int os = value_offset[ret.value];
    dump_lw_sw("a0", "sp", os, "lw");
    // 函数的 epilogue
    if(alloc_size >= -2048 && alloc_size <= 2047) {
      std::cout << "  addi  sp, sp, " << alloc_size << std::endl;
    } else {
      std::cout << "  li    t" << cur << ", " << alloc_size << std::endl;
      std::cout << "  add   sp, sp, t" << cur << std::endl;
    }
    std::cout << "  ret";
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

void Visit(const koopa_raw_load_t &load) {
  std::string rs = "t" + std::to_string(cur);
  int lw_os = value_offset[load.src];
  int sw_os = offset;
  dump_lw_sw(rs, "sp", lw_os, "lw");
  dump_lw_sw(rs, "sp", sw_os, "sw");
}

void Visit(const koopa_raw_store_t &store) {
  std::string rs = "t" + std::to_string(cur);
  if(store.value->kind.tag == KOOPA_RVT_INTEGER) {
    std::cout << "  li    " << rs << ", " << store.value->kind.data.integer.value << std::endl;
  } else {
    int lw_os = value_offset[store.value];
    dump_lw_sw(rs, "sp", lw_os, "lw");
  }
  int sw_os = value_offset[store.dest];
  dump_lw_sw(rs, "sp", sw_os, "sw");
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &bin) {
  std::string rd, rs1, rs2;
  int os1, os2;
  if(bin.lhs->kind.tag == KOOPA_RVT_INTEGER) {
    Visit(bin.lhs);
    rs1 = int_res;
  } else {
    os1 = value_offset[bin.lhs];
    rs1 = "t" + std::to_string(cur);
    dump_lw_sw(rs1, "sp", os1, "lw");
  }
  cur ++;
  if(bin.rhs->kind.tag == KOOPA_RVT_INTEGER) {
    Visit(bin.rhs);
    rs2 = int_res;
  } else {
    os2 = value_offset[bin.rhs];
    rs2 = "t" + std::to_string(cur + 1);
    dump_lw_sw(rs2, "sp", os2, "lw");
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
  // 将返回值写入栈帧中
  dump_lw_sw(rd, "sp", offset, "sw");
}

int cal_alloc_size(const koopa_raw_slice_t &slice) {
  int res = 0;
  for(size_t i = 0; i < slice.len; i ++) {
    auto ptr = slice.buffer[i];
    switch(slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        res += cal_alloc_size(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        res += cal_alloc_size(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        res += cal_alloc_size(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        assert(false);
    }
  }
  return res;
}

int cal_alloc_size(const koopa_raw_function_t &func) {
  return cal_alloc_size(func->bbs);
}

int cal_alloc_size(const koopa_raw_basic_block_t &bb) {
  return cal_alloc_size(bb->insts);
}

int cal_alloc_size(const koopa_raw_value_t &value) {
  int res = 0;
  if(value->ty->tag != KOOPA_RTT_UNIT) res += 4;
  return res;
}

void dump_lw_sw(std::string rs1, std::string rs2, int offset, std::string type) {
  if(offset >= -2048 && offset <= 2047) {
    std::cout << "  " << type << "    " << rs1 << ", " << offset << "(" << rs2 << ")" << std::endl;
  } else {
    std::cout << "  " << type << "    " << rs1 << ", " << offset << std::endl;
    std::cout << "  " << type << "    " << rs1 << ", " << rs1 << "(" << rs2 << ")" << std::endl;
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