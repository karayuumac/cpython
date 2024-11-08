//
// Created by karayuu on 24/11/05.
//
#include "jit_record.h";
#include "jit_internal.h";

lir_op_t *initialize_lir_op()
{
    lir_op_t *new_lir_op = PyMem_Malloc(sizeof(lir_op_t));
    new_lir_op->register_index = -1;
    return new_lir_op;
}

void jit_record_LIR_NOPE()
{
    lir_op_t* lir = initialize_lir_op();
    lir->opcode = LIR_NOPE;
    lir->oparg.obj = NULL;
    jit_record_lir(lir);
}

void jit_record_LIR_ENV_LOAD_AND_PUSH(int oparg)
{
    lir_op_t *lir_env_load = initialize_lir_op();
    lir_env_load->opcode = LIR_ENV_LOAD;
    lir_env_load->oparg.arg = oparg;
    jit_record_lir(lir_env_load);

    lir_op_t *lir_push = initialize_lir_op();
    lir_push->opcode = LIR_PUSH;
    jit_record_lir(lir_push);
}

void jit_record_LIR_PUSH(PyObject* obj)
{
    lir_op_t *lir = initialize_lir_op();
    lir->opcode = LIR_PUSH;
    lir->oparg.obj = obj;
    jit_record_lir(lir);
}

void jit_record_LIR_LOAD_CONST_LL(long long value)
{
    lir_op_t *lir = initialize_lir_op();
    lir->opcode = LIR_LOAD_CONST_LL;
    lir->oparg.ll = value;
    jit_record_lir(lir);
}

void jit_record_LIR_POP_AND_ENV_STORE(int oparg)
{
    lir_op_t *lir_store = initialize_lir_op();
    lir_store->opcode = LIR_ENV_STORE;
    lir_store->oparg.arg = oparg;

    lir_op_t *lir_pop = initialize_lir_op();
    lir_pop->opcode = LIR_POP;

    jit_record_lir(lir_pop);
    jit_record_lir(lir_store);
}

void jit_record_LIR_LL_ADD_OVERFLOW()
{
    lir_op_t *lir_pop = initialize_lir_op();
    lir_pop->opcode = LIR_POP;
    jit_record_lir(lir_pop);

    lir_op_t *lir_pop_guard_ll = initialize_lir_op();
    lir_pop_guard_ll->opcode = LIR_GUARD_TYPE_LL;
    jit_record_lir(lir_pop_guard_ll);

    lir_op_t *lir_pop_two = initialize_lir_op();
    lir_pop_two->opcode = LIR_POP;
    jit_record_lir(lir_pop_two);

    lir_op_t *lir_pop_two_guard_ll = initialize_lir_op();
    lir_pop_two_guard_ll->opcode = LIR_GUARD_TYPE_LL;
    jit_record_lir(lir_pop_two_guard_ll);

    lir_op_t *lir_guard_add_overflow_ll = initialize_lir_op();
    lir_guard_add_overflow_ll->opcode = LIR_GUARD_ADD_OVERFLOW_LL;
    jit_record_lir(lir_guard_add_overflow_ll);

    lir_op_t *lir_add_ll = initialize_lir_op();
    lir_add_ll->opcode = LIR_ADD_LL;
    jit_record_lir(lir_add_ll);
}

void jit_record_LIR_GUARD_TYPE_BOOL(int bool)
{
    lir_op_t *lir_guard_type_bool = initialize_lir_op();
    if (bool)
    {
        lir_guard_type_bool->opcode = LIR_GUARD_TYPE_TRUE;
    }
    else
    {
        lir_guard_type_bool->opcode = LIR_GUARD_TYPE_FALSE;
    }
    jit_record_lir(lir_guard_type_bool);
}