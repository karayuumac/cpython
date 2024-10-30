/*
 * Created by karayuu on 24/10/26.
 */
#include "Python.h"
#include "internal/pycore_pystate.h"
#include "opcode.h"
#include "jit_internal.h"

/// オペコード名の取得
char *get_opcode_name(int opcode) {
  switch (opcode) {
    case BINARY_ADD: return "BINARY_ADD";
    case BINARY_SUBTRACT: return "BINARY_SUBTRACT";
    case BINARY_MULTIPLY: return "BINARY_MULTIPLY";
    case BINARY_TRUE_DIVIDE: return "BINARY_TRUE_DIVIDE";
    case BINARY_FLOOR_DIVIDE: return "BINARY_FLOOR_DIVIDE";
    case BINARY_MODULO: return "BINARY_MODULO";
    case BINARY_POWER: return "BINARY_POWER";
    case UNARY_NEGATIVE: return "UNARY_NEGATIVE";
    case UNARY_POSITIVE: return "UNARY_POSITIVE";
    case UNARY_NOT: return "UNARY_NOT";
    case JUMP_ABSOLUTE: return "JUMP_ABSOLUTE";
    case JUMP_IF_TRUE_OR_POP: return "JUMP_IF_TRUE_OR_POP";
    case JUMP_IF_FALSE_OR_POP: return "JUMP_IF_FALSE_OR_POP";
    case POP_JUMP_IF_TRUE: return "POP_JUMP_IF_TRUE";
    case POP_JUMP_IF_FALSE: return "POP_JUMP_IF_FALSE";
    case LOAD_FAST: return "LOAD_FAST";
    case STORE_FAST: return "STORE_FAST";
    case LOAD_CONST: return "LOAD_CONST";
    case LOAD_GLOBAL: return "LOAD_GLOBAL";
    case COMPARE_OP: return "COMPARE_OP";
    default: return "UNKNOWN";
  }
}

/// オブジェクトの文字列表現
static const char *object_str(PyObject *obj) {
  if (obj == NULL) {
    return "NULL";
  }

  PyObject *str = PyObject_Str(obj);
  if (str == NULL) {
    PyErr_Clear();
    return "<error>";
  }

  const char *result = PyUnicode_AsUTF8(str);
  if (result == NULL) {
    PyErr_Clear();
    Py_DECREF(str);
    return "<error>";
  }

  Py_DECREF(str);
  return result;
}

/// トレース情報のダンプ
void jit_dump_trace(trace_t *trace) {
  if (trace == NULL) {
    return;
  }

  printf("=== Trace Dump ===\n");
  printf("Start offset: %d\n", trace->start_offset);
  printf("Current offset: %d\n", trace->current_offset);
  printf("Counter value: %d\n", trace->counter ? *trace->counter : -1);
  printf("Instructions (%zd):\n", trace->buffer.length);

  for (Py_ssize_t i = 0; i < trace->buffer.length; i++) {
    trace_instruction_t *inst = &trace->buffer.instructions[i];

    printf("%4zd: %s (oparg=%d, stack_depth=%d)\n",
           i,
           get_opcode_name(inst->opcode),
           inst->oparg,
           inst->stack_depth);

    // スタック情報の表示
    printf("    Stack values: [");
    for (int j = 0; j < inst->stack_depth; j++) {
      if (j > 0) printf(", ");
      printf("%s", object_str(inst->stack_values[j]));
    }
    printf("]\n");

    printf("    Stack types: [");
    for (int j = 0; j < inst->stack_depth; j++) {
      if (j > 0) printf(", ");
      PyTypeObject *type = inst->stack_types[j];
      printf("%s", type ? type->tp_name : "NULL");
    }
    printf("]\n");

    // 参照情報の表示
    if (inst->refs.names)
      printf("    Name ref: %s\n", object_str(inst->refs.names));
    if (inst->refs.consts)
      printf("    Const ref: %s\n", object_str(inst->refs.consts));
    if (inst->refs.locals)
      printf("    Local ref: %s\n", object_str(inst->refs.locals));
  }

  // ガード条件の表示
  printf("Guard conditions (%zd):\n", trace->guard_conditions.length);
  for (Py_ssize_t i = 0; i < trace->guard_conditions.length; i++) {
    trace_guard_t *guard = &trace->guard_conditions.guards[i];
    printf("%4zd: ", i);

    switch (guard->kind) {
      case GUARD_TYPE:
        printf("Type guard: stack[%d] == %s\n",
               guard->stack_index,
               guard->expected.type->tp_name);
        break;

      case GUARD_CLASS:
        printf("Class guard: stack[%d] class == %s\n",
               guard->stack_index,
               ((PyTypeObject *)guard->expected.value)->tp_name);
        break;

        /*
      case GUARD_SHAPE:
        printf("Shape guard: stack[%d] shape == %s\n",
               guard->stack_index,
               object_str(guard->expected.shape));
        break;
         */

      case GUARD_VALUE:
        printf("Value guard: stack[%d] == %s\n",
               guard->stack_index,
               object_str(guard->expected.value));
        break;
    }
  }
  printf("==========\n");
}