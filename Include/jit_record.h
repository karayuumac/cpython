//
// Created by karayuu on 24/11/05.
//

#ifndef JIT_RECORD_H
#define JIT_RECORD_H

#include "Python.h"

void jit_record_LIR_NOPE();
void jit_record_LIR_ENV_LOAD_AND_PUSH(int oparg);
void jit_record_LIR_LOAD_CONST_LL(long long value);
void jit_record_LIR_POP_AND_ENV_STORE(int oparg);
void jit_record_LIR_LL_ADD_OVERFLOW();
void jit_record_LIR_GUARD_TYPE_BOOL(int bool);
void jit_record_LIR_COMPARE_OP_NUM(int oparg);
void jit_record_LIR_EXIT();
void jit_record_LIR_COMMIT();

#endif //JIT_RECORD_H
