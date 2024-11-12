/*
 * Created by karayuu on 24/10/26.
 */
#include "Python.h"
#include "internal/pycore_pystate.h"
#include "opcode.h"
#include "jit.h"

/// オペコード名の取得
char* get_opcode_name(int opcode)
{
    switch (opcode)
    {
    case POP_TOP: return "POP_TOP";
    case ROT_TWO: return "ROT_TWO";
    case ROT_THREE: return "ROT_THREE";
    case DUP_TOP: return "DUP_TOP";
    case DUP_TOP_TWO: return "DUP_TOP_TWO";
    case ROT_FOUR: return "ROT_FOUR";
    case NOP: return "NOP";
    case UNARY_POSITIVE: return "UNARY_POSITIVE";
    case UNARY_NEGATIVE: return "UNARY_NEGATIVE";
    case UNARY_NOT: return "UNARY_NOT";
    case UNARY_INVERT: return "UNARY_INVERT";
    case BINARY_MATRIX_MULTIPLY: return "BINARY_MATRIX_MULTIPLY";
    case INPLACE_MATRIX_MULTIPLY: return "INPLACE_MATRIX_MULTIPLY";
    case BINARY_POWER: return "BINARY_POWER";
    case BINARY_MULTIPLY: return "BINARY_MULTIPLY";
    case BINARY_MODULO: return "BINARY_MODULO";
    case BINARY_ADD: return "BINARY_ADD";
    case BINARY_SUBTRACT: return "BINARY_SUBTRACT";
    case BINARY_SUBSCR: return "BINARY_SUBSCR";
    case BINARY_FLOOR_DIVIDE: return "BINARY_FLOOR_DIVIDE";
    case BINARY_TRUE_DIVIDE: return "BINARY_TRUE_DIVIDE";
    case INPLACE_FLOOR_DIVIDE: return "INPLACE_FLOOR_DIVIDE";
    case INPLACE_TRUE_DIVIDE: return "INPLACE_TRUE_DIVIDE";
    case GET_LEN: return "GET_LEN";
    case MATCH_MAPPING: return "MATCH_MAPPING";
    case MATCH_SEQUENCE: return "MATCH_SEQUENCE";
    case MATCH_KEYS: return "MATCH_KEYS";
    case COPY_DICT_WITHOUT_KEYS: return "COPY_DICT_WITHOUT_KEYS";
    case WITH_EXCEPT_START: return "WITH_EXCEPT_START";
    case GET_AITER: return "GET_AITER";
    case GET_ANEXT: return "GET_ANEXT";
    case BEFORE_ASYNC_WITH: return "BEFORE_ASYNC_WITH";
    case END_ASYNC_FOR: return "END_ASYNC_FOR";
    case INPLACE_ADD: return "INPLACE_ADD";
    case INPLACE_SUBTRACT: return "INPLACE_SUBTRACT";
    case INPLACE_MULTIPLY: return "INPLACE_MULTIPLY";
    case INPLACE_MODULO: return "INPLACE_MODULO";
    case STORE_SUBSCR: return "STORE_SUBSCR";
    case DELETE_SUBSCR: return "DELETE_SUBSCR";
    case BINARY_LSHIFT: return "BINARY_LSHIFT";
    case BINARY_RSHIFT: return "BINARY_RSHIFT";
    case BINARY_AND: return "BINARY_AND";
    case BINARY_XOR: return "BINARY_XOR";
    case BINARY_OR: return "BINARY_OR";
    case INPLACE_POWER: return "INPLACE_POWER";
    case GET_ITER: return "GET_ITER";
    case GET_YIELD_FROM_ITER: return "GET_YIELD_FROM_ITER";
    case PRINT_EXPR: return "PRINT_EXPR";
    case LOAD_BUILD_CLASS: return "LOAD_BUILD_CLASS";
    case YIELD_FROM: return "YIELD_FROM";
    case GET_AWAITABLE: return "GET_AWAITABLE";
    case LOAD_ASSERTION_ERROR: return "LOAD_ASSERTION_ERROR";
    case INPLACE_LSHIFT: return "INPLACE_LSHIFT";
    case INPLACE_RSHIFT: return "INPLACE_RSHIFT";
    case INPLACE_AND: return "INPLACE_AND";
    case INPLACE_XOR: return "INPLACE_XOR";
    case INPLACE_OR: return "INPLACE_OR";
    case LIST_TO_TUPLE: return "LIST_TO_TUPLE";
    case RETURN_VALUE: return "RETURN_VALUE";
    case IMPORT_STAR: return "IMPORT_STAR";
    case SETUP_ANNOTATIONS: return "SETUP_ANNOTATIONS";
    case YIELD_VALUE: return "YIELD_VALUE";
    case POP_BLOCK: return "POP_BLOCK";
    case POP_EXCEPT: return "POP_EXCEPT";
    case STORE_NAME: return "STORE_NAME";
    case DELETE_NAME: return "DELETE_NAME";
    case UNPACK_SEQUENCE: return "UNPACK_SEQUENCE";
    case FOR_ITER: return "FOR_ITER";
    case UNPACK_EX: return "UNPACK_EX";
    case STORE_ATTR: return "STORE_ATTR";
    case DELETE_ATTR: return "DELETE_ATTR";
    case STORE_GLOBAL: return "STORE_GLOBAL";
    case DELETE_GLOBAL: return "DELETE_GLOBAL";
    case ROT_N: return "ROT_N";
    case LOAD_CONST: return "LOAD_CONST";
    case LOAD_NAME: return "LOAD_NAME";
    case BUILD_TUPLE: return "BUILD_TUPLE";
    case BUILD_LIST: return "BUILD_LIST";
    case BUILD_SET: return "BUILD_SET";
    case BUILD_MAP: return "BUILD_MAP";
    case LOAD_ATTR: return "LOAD_ATTR";
    case COMPARE_OP: return "COMPARE_OP";
    case IMPORT_NAME: return "IMPORT_NAME";
    case IMPORT_FROM: return "IMPORT_FROM";
    case JUMP_FORWARD: return "JUMP_FORWARD";
    case JUMP_IF_FALSE_OR_POP: return "JUMP_IF_FALSE_OR_POP";
    case JUMP_IF_TRUE_OR_POP: return "JUMP_IF_TRUE_OR_POP";
    case JUMP_ABSOLUTE: return "JUMP_ABSOLUTE";
    case POP_JUMP_IF_FALSE: return "POP_JUMP_IF_FALSE";
    case POP_JUMP_IF_TRUE: return "POP_JUMP_IF_TRUE";
    case LOAD_GLOBAL: return "LOAD_GLOBAL";
    case IS_OP: return "IS_OP";
    case CONTAINS_OP: return "CONTAINS_OP";
    case RERAISE: return "RERAISE";
    case JUMP_IF_NOT_EXC_MATCH: return "JUMP_IF_NOT_EXC_MATCH";
    case SETUP_FINALLY: return "SETUP_FINALLY";
    case LOAD_FAST: return "LOAD_FAST";
    case STORE_FAST: return "STORE_FAST";
    case DELETE_FAST: return "DELETE_FAST";
    case GEN_START: return "GEN_START";
    case RAISE_VARARGS: return "RAISE_VARARGS";
    case CALL_FUNCTION: return "CALL_FUNCTION";
    case MAKE_FUNCTION: return "MAKE_FUNCTION";
    case BUILD_SLICE: return "BUILD_SLICE";
    case LOAD_CLOSURE: return "LOAD_CLOSURE";
    case LOAD_DEREF: return "LOAD_DEREF";
    case STORE_DEREF: return "STORE_DEREF";
    case DELETE_DEREF: return "DELETE_DEREF";
    case CALL_FUNCTION_KW: return "CALL_FUNCTION_KW";
    case CALL_FUNCTION_EX: return "CALL_FUNCTION_EX";
    case SETUP_WITH: return "SETUP_WITH";
    case EXTENDED_ARG: return "EXTENDED_ARG";
    case LIST_APPEND: return "LIST_APPEND";
    case SET_ADD: return "SET_ADD";
    case MAP_ADD: return "MAP_ADD";
    case LOAD_CLASSDEREF: return "LOAD_CLASSDEREF";
    case MATCH_CLASS: return "MATCH_CLASS";
    case SETUP_ASYNC_WITH: return "SETUP_ASYNC_WITH";
    case FORMAT_VALUE: return "FORMAT_VALUE";
    case BUILD_CONST_KEY_MAP: return "BUILD_CONST_KEY_MAP";
    case BUILD_STRING: return "BUILD_STRING";
    case LOAD_METHOD: return "LOAD_METHOD";
    case CALL_METHOD: return "CALL_METHOD";
    case LIST_EXTEND: return "LIST_EXTEND";
    case SET_UPDATE: return "SET_UPDATE";
    case DICT_MERGE: return "DICT_MERGE";
    case DICT_UPDATE: return "DICT_UPDATE";
    default: return "UNKNOWN";
    }
}

/// LIR命令の文字列化
char *get_lir_opcode_name(const lir_op_t *op)
{
    switch (op->opcode)
    {
    case LIR_NOPE: return "LIR_NOPE";
    case LIR_ENV_LOAD: return "LIR_ENV_LOAD";
    case LIR_PUSH: return "LIR_PUSH";
    case LIR_POP: return "LIR_POP";
    case LIR_ENV_STORE: return "LIR_ENV_STORE";
    case LIR_LOAD_CONST_LL: return "LIR_LOAD_CONST_LL";
    case LIR_GUARD_TYPE_LL: return "LIR_GUARD_TYPE_LL";
    case LIR_GUARD_ADD_OVERFLOW_LL: return "LIR_GUARD_ADD_OVERFLOW_LL";
    case LIR_ADD_LL: return "LIR_ADD_LL";
    case LIR_GUARD_TYPE_TRUE: return "LIR_GUARD_TYPE_TRUE";
    case LIR_GUARD_TYPE_FALSE: return "LIR_GUARD_TYPE_FALSE";
    case LIR_GUARD_TYPE_NUM: return "LIR_GUARD_TYPE_NUM";
    case LIR_EQ_NUM: return "LIR_EQ_NUM";
    case LIR_GE_NUM: return "LIR_GE_NUM";
    case LIR_GT_NUM: return "LIR_GT_NUM";
    case LIR_LE_NUM: return "LIR_LE_NUM";
    case LIR_LT_NUM: return "LIR_LT_NUM";
    case LIR_NE_NUM: return "LIR_NE_NUM";
    case LIR_EXIT: return "LIR_EXIT";
    case LIR_COMMIT: return "LIR_COMMIT";
    default: return "UNKNOWN";
    }
}

char *get_lir_oparg_info(const lir_op_t *op)
{
    static char _buf[10000];
    char *buf = _buf;

    switch (op->opcode)
    {
    case LIR_NOPE:
        sprintf(buf, "");
        break;
    case LIR_ENV_LOAD:
        sprintf(buf, "arg = %d", op->oparg.arg);
        break;
    case LIR_ENV_STORE:
        sprintf(buf, "arg = %d", op->oparg.arg);
        break;
    case LIR_LOAD_CONST_LL:
        sprintf(buf, "ll = %ld", op->oparg.ll);
        break;
    default:
        sprintf(buf, "");
        break;
    }
    return buf;
}

/// オブジェクトの文字列表現
static const char* object_str(PyObject* obj)
{
    if (obj == NULL)
    {
        return "NULL";
    }

    PyObject* str = PyObject_Str(obj);
    if (str == NULL)
    {
        PyErr_Clear();
        return "<error>";
    }

    const char* result = PyUnicode_AsUTF8(str);
    if (result == NULL)
    {
        PyErr_Clear();
        Py_DECREF(str);
        return "<error>";
    }

    Py_DECREF(str);
    return result;
}

/// トレース情報のダンプ
void jit_dump_trace(trace_t* trace)
{
    if (trace == NULL)
    {
        return;
    }

    printf("=== Trace Dump ===\n");
    printf("Start offset: %d\n", trace->start_offset);
    printf("Current offset: %d\n", trace->current_offset);
    printf("Counter value: %d\n", trace->counter ? *trace->counter : -1);
    printf("Instructions (%zd):\n", trace->buffer.length);

    for (Py_ssize_t i = 0; i < trace->buffer.length; i++)
    {
        trace_instruction_t* inst = &trace->buffer.instructions[i];

        printf("%4zd: %s (oparg=%d, stack_depth=%d)\n",
               i,
               get_opcode_name(inst->opcode),
               inst->oparg,
               inst->stack_depth);

        // スタック情報の表示
        printf("    Stack values: [");
        for (int j = 0; j < inst->stack_depth; j++)
        {
            if (j > 0) printf(", ");
            printf("%s", object_str(inst->stack_values[j]));
        }
        printf("]\n");

        printf("    Stack types: [");
        for (int j = 0; j < inst->stack_depth; j++)
        {
            if (j > 0) printf(", ");
            PyTypeObject* type = inst->stack_types[j];
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
    for (Py_ssize_t i = 0; i < trace->guard_conditions.length; i++)
    {
        trace_guard_t* guard = &trace->guard_conditions.guards[i];
        printf("%4zd: ", i);

        switch (guard->kind)
        {
        case GUARD_TYPE:
            printf("Type guard: stack[%d] == %s\n",
                   guard->stack_index,
                   guard->expected.type->tp_name);
            break;

        case GUARD_CLASS:
            printf("Class guard: stack[%d] class == %s\n",
                   guard->stack_index,
                   ((PyTypeObject*)guard->expected.value)->tp_name);
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

    // lir 命令の表示
    printf("LIR instructions (%zd):\n", trace->lir_buffer.length);
    for (Py_ssize_t i = 0; i < trace->lir_buffer.length; i++)
    {
        lir_op_t *lir_op = &trace->lir_buffer.lirs[i];
        printf("%4zd: %s\n", i, get_lir_opcode_name(lir_op));
        printf("    %s\n", get_lir_oparg_info(lir_op));
    }
    printf("==========\n");
}
