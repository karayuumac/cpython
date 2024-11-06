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
    lir_push->oparg.ref_op = lir_env_load;
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