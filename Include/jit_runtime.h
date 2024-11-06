//
// Created by karayuu on 24/10/28.
//

#ifndef JIT_RUNTIME_H
#define JIT_RUNTIME_H

#include "Python.h"

// 共通のランタイム関数
static PyObject* jit_binary_add(PyObject* a, PyObject* b) {
  PyObject* result = PyNumber_Add(a, b);
  if (result == NULL) return NULL;
  return result;
}

static PyObject* jit_binary_multiply(PyObject* a, PyObject* b) {
  PyObject* result = PyNumber_Multiply(a, b);
  if (result == NULL) return NULL;
  return result;
}

static int jit_check_type(PyObject* obj, PyTypeObject* type) {
  return PyObject_TypeCheck(obj, type);
}

static int jit_check_overflow_add(PyObject* a, PyObject* b) {
  long va = PyLong_AsLong(a);
  long vb = PyLong_AsLong(b);
  return (va > 0 && vb > 0 && va > LONG_MAX - vb);
}

#endif //JIT_RUNTIME_H
