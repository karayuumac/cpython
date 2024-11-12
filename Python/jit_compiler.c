//
// Created by karayuu on 24/10/28.
//
#include "jit.h"
#include "opcode.h"
#include <dlfcn.h>

/// LIR のレジスタ割当て
void allocate_lir_register(trace_t* trace)
{
  /*
  int reg_num = 0;
  for (int i = 0; i < trace->lir_buffer.length; i++)
  {
    switch (trace->lir_buffer.lirs[i].opcode)
    {
    case LIR_PUSH:
      trace->lir_buffer.lirs[i].reg.register_index = reg_num;
      reg_num++;
      break;

    default:
      continue;
    }

  }
  */

  /*
  lir_code_t* lir = lir_create();
  if (!lir) return NULL;

  // トレース命令の変換
  for (int i = 0; i < trace->buffer.length; i++)
  {
    trace_instruction_t* inst = &trace->buffer.instructions[i];

    // 現在の命令に関連するガード条件のチェック
    for (int j = 0; j < trace->guard_conditions.length; j++)
    {
      trace_guard_t* guard = &trace->guard_conditions.guards[j];
      if (guard->stack_index == i)
      {
        // ガード命令の生成
        lir_inst_t* guard_inst = NULL;

        switch (guard->kind)
        {
        case GUARD_TYPE:
          {
            guard_inst = lir_new_inst(LIR_GUARD_TYPE);
            guard_inst->src1 = lir_stack_operand(guard->stack_index);
            guard_inst->src2 = lir_type_operand(guard->expected.type);
            break;
          }

        case GUARD_CLASS:
          {
            guard_inst = lir_new_inst(LIR_GUARD_TYPE);
            guard_inst->src1 = lir_stack_operand(guard->stack_index);
            guard_inst->src2 = lir_type_operand((PyTypeObject*)guard->expected.value);
            break;
          }

        case GUARD_VALUE:
          {
            guard_inst = lir_new_inst(LIR_GUARD_TYPE);
            guard_inst->src1 = lir_stack_operand(guard->stack_index);
            int const_idx = lir_add_constant(lir, guard->expected.value);
            guard_inst->src2 = lir_heap_operand(const_idx);
          }
        }

        if (guard_inst)
        {
          guard_inst->guard_exit = guard->side_exit;
          lir_append_inst(lir->current, guard_inst);
        }
      }
    }

    // 命令の変換
    switch (inst->opcode)
    {
    case LOAD_CONST:
      lir_inst_t* load_const = lir_new_inst(LIR_CONST);
      load_const->dest = lir_reg_operand(lir->reg_count++);
      load_const->src1 = lir_heap_operand(inst->oparg);
      lir_append_inst(lir->current, load_const);
      break;

    case STORE_NAME:
      // スタックから値をロードする
      lir_inst_t* load_value = lir_new_inst(LIR_LOAD_STACK);
      load_value->dest = lir_reg_operand(lir->reg_count++);
      load_value->src1 = lir_stack_operand(inst->stack_depth - 1); // スタックトップ
      lir_append_inst(lir->current, load_value);

      // co_names から名前をロードする
      lir_inst_t *load_name = lir_new_inst(LIR_LOAD_NAME);
      load_name->dest = lir_reg_operand(lir->reg_count++);
      load_name->src1 = lir_heap_operand(inst->oparg);
      lir_append_inst(lir->current, load_name);

      // 値の格納
      lir_inst_t *store = lir_new_inst(LIR_STORE_NAME);
      store->src1 = load_name->dest,
      store->src2 = load_value->dest,
      lir_append_inst(lir->current, store);
      break;

    case LOAD_FAST:
      {
        lir_inst_t* load = lir_new_inst(LIR_LOAD);
        load->dest = lir_reg_operand(lir->reg_count++);
        load->src1 = lir_stack_operand(inst->oparg);
        lir_append_inst(lir->current, load);
        break;
      }

    case STORE_FAST:
      {
        lir_inst_t* store = lir_new_inst(LIR_STORE);
        store->src1 = lir_reg_operand(lir->reg_count - 1);
        store->dest = lir_stack_operand(inst->oparg);
        lir_append_inst(lir->current, store);
        break;
      }

    case BINARY_ADD:
      {
        // 型チェックガード
        lir_inst_t *guard_type_1 = lir_new_inst(LIR_GUARD_TYPE_LL);
        guard_type_1->src1 = lir_reg_operand(lir->reg_count - 2);
        lir_append_inst(lir->current, guard_type_1);

        lir_inst_t *guard_type_2 = lir_new_inst(LIR_GUARD_TYPE_LL);
        guard_type_2->src1 = lir_reg_operand(lir->reg_count - 1);
        lir_append_inst(lir->current, guard_type_2);

        // オーバーフローチェック
        lir_inst_t* guard_overflow = lir_new_inst(LIR_GUARD_ADD_LL_OVERFLOW);
        guard_overflow->src1 = lir_reg_operand(lir->reg_count - 2);
        guard_overflow->src2 = lir_reg_operand(lir->reg_count - 1);
        lir_append_inst(lir->current, guard_overflow);

        // 加算
        lir_inst_t* add = lir_new_inst(LIR_ADD);
        add->dest = lir_reg_operand(lir->reg_count++);
        add->src1 = guard_overflow->src1;
        add->src2 = guard_overflow->src2;
        lir_append_inst(lir->current, add);
        break;
      }

    case BINARY_SUBTRACT:
      {
        // オーバーフローチェック
        // lir_
      }

    case BINARY_MULTIPLY:
      {
        // オーバーフローチェック
        lir_inst_t* guard = lir_new_inst(LIR_GUARD_MUL_LL_OVERFLOW);
        guard->src1 = lir_reg_operand(lir->reg_count - 2);
        guard->src2 = lir_reg_operand(lir->reg_count - 1);
        lir_append_inst(lir->current, guard);

        // 乗算
        lir_inst_t* mul = lir_new_inst(LIR_MUL);
        mul->dest = lir_reg_operand(lir->reg_count++);
        mul->src1 = guard->src1;
        mul->src2 = guard->src2;
        lir_append_inst(lir->current, mul);
        break;
      }

    case COMPARE_OP:
      {
        lir_inst_t* cmp = NULL;
        switch (inst->oparg)
        {
        case Py_LT: cmp = lir_new_inst(LIR_LT);
          break;
        case Py_LE: cmp = lir_new_inst(LIR_LE);
          break;
        case Py_EQ: cmp = lir_new_inst(LIR_EQ);
          break;
        case Py_NE: cmp = lir_new_inst(LIR_NE);
          break;
        case Py_GT: cmp = lir_new_inst(LIR_GT);
          break;
        case Py_GE: cmp = lir_new_inst(LIR_GE);
          break;
        }
        if (cmp)
        {
          cmp->dest = lir_reg_operand(lir->reg_count++);
          cmp->src1 = lir_reg_operand(lir->reg_count - 2);
          cmp->src2 = lir_reg_operand(lir->reg_count - 1);
          lir_append_inst(lir->current, cmp);
        }
        break;
      }

    case LOAD_METHOD:
      {
        // メソッドのロード
        lir_operand_t src1 = lir_reg_operand(lir->reg_count - 1);

        lir_inst_t* load = lir_new_inst(LIR_CALL);
        load->dest = lir_reg_operand(lir->reg_count++);
        load->src1 = src1;
        load->src2 = lir_method_operand(inst->refs.names);
        lir_append_inst(lir->current, load);
        break;
      }

    case JUMP_ABSOLUTE:
      {
        lir_inst_t* jump = lir_new_inst(LIR_JUMP);
        jump->dest = lir_label_operand(inst->oparg);
        lir_append_inst(lir->current, jump);

        // 新しいブロックを作成
        lir_block_t* next = lir_new_block(lir);
        if (!next) {
          lir_free(lir);
          return NULL;
        }
        lir->current->next = next;
        lir->current = next;
        break;
      }

    case POP_JUMP_IF_FALSE:
    case POP_JUMP_IF_TRUE:
      {
        lir_inst_t* branch = lir_new_inst(LIR_BRANCH);
        branch->src1 = lir_reg_operand(lir->reg_count - 1);
        branch->dest = lir_label_operand(inst->oparg);
        lir_append_inst(lir->current, branch);

        // 新しいブロックを作成
        lir_block_t* next = lir_new_block(lir);
        if (!next) {
          lir_free(lir);
          return NULL;
        }
        lir->current->next = next;
        lir->current = next;
        break;
      }

    default:
      lir_inst_t *exit = lir_new_inst(LIR_EXIT);
      exit->src1 = lir_label_operand(inst->f_lasti);
      lir_append_inst(lir->current, exit);
      return lir;

    }
  }
  return lir;
  */
}
