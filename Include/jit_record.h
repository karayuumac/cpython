//
// Created by karayuu on 24/11/05.
//

#ifndef JIT_RECORD_H
#define JIT_RECORD_H

#include "Python.h"

void jit_record_LIR_NOPE();
void jit_record_LIR_ENV_LOAD_AND_PUSH(int oparg);
void jit_record_LIR_PUSH(PyObject* obj);
void jit_record_LIR_LOAD_CONST_LL(long long value);

#endif //JIT_RECORD_H
