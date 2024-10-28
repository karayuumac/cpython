//
// Created by karayuu on 24/10/28.
//
#include "jit_lir.h"

/// LIRコードの作成
lir_code_t *lir_create(void)
{
  lir_code_t *lir = (lir_code_t *) PyMem_Malloc(sizeof(lir_code_t));
  if (!lir) return NULL;

  lir->entry = NULL;
  lir->current = NULL;
  lir->reg_count = 0;
  lir->label_count = 0;
  lir->constants = NULL;
  lir->const_count = 0;

  // 入口ブロックを作成
  lir->entry = lir_new_block(lir);
  if (!lir->entry)
  {
    PyMem_Free(lir);
    return NULL;
  }
  lir->current = lir->entry;

  return lir;
}

/// LIRコードの解放
void lir_free(lir_code_t *lir)
{
  if (!lir)
  {
    return;
  }

  // ブロックの解放
  lir_block_t *block = lir->entry;
  while (block)
  {
    // 命令の解放
    lir_inst_t *inst = block->first;
    while (inst)
    {
      lir_inst_t *next = inst->next;
      PyMem_Free(inst);
      inst = next;
    }

    lir_block_t *next = block->next;
    PyMem_Free(block);
    block = next;
  }

  // 定数プールの解放
  if (lir->constants)
  {
    for (int i = 0; i < lir->const_count; i++)
    {
      Py_XDECREF(lir->constants[i]);
    }
    PyMem_Free(lir->constants);
  }

  PyMem_Free(lir);
}

/// 新規ブロックの作成
lir_block_t *lir_new_block(lir_code_t *lir)
{
  lir_block_t *block = (lir_block_t *) PyMem_Malloc(sizeof(lir_block_t));
  if (!block)
  {
    return NULL;
  }

  block->next = NULL;
  block->first = NULL;
  block->last = NULL;
  block->label = lir->label_count++;

  return block;
}

/// 新規命令の作成
lir_inst_t *lir_new_inst(lir_optcode_t opcode)
{
  lir_inst_t *inst = (lir_inst_t *) PyMem_Malloc(sizeof(lir_inst_t));
  if (!inst)
  {
    return NULL;
  }

  inst->opcode = opcode;
  inst->dest = lir_none_operand();
  inst->src1 = lir_none_operand();
  inst->src2 = lir_none_operand();
  inst->next = NULL;
  inst->guard_exit = NULL;

  return inst;
}

/// ブロックへの命令追加
void lir_append_inst(lir_block_t *block, lir_inst_t *inst)
{
  if (!block->first)
  {
    block->first = inst;
  }
  else
  {
    block->last->next = inst;
  }
  block->last = inst;
}

/// 定数の追加
int lir_add_constant(lir_code_t *lir, PyObject *const_val)
{
  // 配列の拡張が必要な場合
  if (lir->const_count == 0)
  {
    lir->constants = PyMem_Malloc(sizeof(PyObject*) * 8);
    if (!lir->constants) return -1;
  }
  else if ((lir->const_count & (lir->const_count - 1)) == 0)
  {
    PyObject **new_constants = PyMem_Realloc(
      lir->constants,
      sizeof(PyObject*) * lir->const_count * 2);
    if (!new_constants) return -1;
    lir->constants = new_constants;
  }

  Py_INCREF(const_val);
  lir->constants[lir->const_count] = const_val;
  return lir->const_count++;
}

// オペランド生成関数
lir_operand_t lir_none_operand(void)
{
  lir_operand_t op = { OPERAND_NONE };
  return op;
}

lir_operand_t lir_const_operand(int const_index)
{
  lir_operand_t op = { OPERAND_CONST };
  op.u.reg_num = const_index;
  return op;
}

lir_operand_t lir_reg_operand(int reg_num)
{
  lir_operand_t op = { OPERAND_REG };
  op.u.reg_num = reg_num;
  return op;
}

lir_operand_t lir_stack_operand(int stack_pos)
{
  lir_operand_t op = { OPERAND_STACK };
  op.u.stack_pos = stack_pos;
  return op;
}

lir_operand_t lir_exit_operand(struct trace *exit)
{
  lir_operand_t op = { OPERAND_EXIT };
  op.u.exit = exit;
  return op;
}

lir_operand_t lir_type_operand(PyTypeObject *type)
{
  lir_operand_t op = { OPERAND_TYPE };
  op.u.type = type;
  return op;
}

lir_operand_t lir_method_operand(PyObject *method)
{
  lir_operand_t op = { OPERAND_METHOD };
  op.u.method = method;
  return op;
}

lir_operand_t lir_label_operand(int label)
{
  lir_operand_t op = { OPERAND_LABEL };
  op.u.label = label;
  return op;
}