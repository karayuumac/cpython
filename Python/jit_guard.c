/*
 * Created by karayuu on 24/10/26.
 */
#include "Python.h"
#include "internal/pycore_pystate.h"
#include "jit_internal.h"

/// ガード条件の追加
int jit_add_guard(trace_t *trace, guard_kind_t kind, int stack_index, PyObject *expected) {
  if (trace == NULL || expected == NULL) {
    return -1;
  }

  // バッファのリサイズが必要かどうかを確認する
  if (trace->guard_conditions.length >= trace->guard_conditions.capacity) {
    Py_ssize_t new_capacty = trace->guard_conditions.capacity * 2;
    trace_guard_t *new_guards = (trace_guard_t *) PyMem_Realloc(trace->guard_conditions.guards, sizeof(trace_guard_t) * new_capacty);
    if (new_guards == NULL) {
      return -1;
    }
    trace->guard_conditions.guards = new_guards;
    trace->guard_conditions.capacity = new_capacty;
  }

  // 新しいガード条件を追加
  trace_guard_t *guard = &trace->guard_conditions.guards[trace->guard_conditions.length];
  guard->kind = kind;
  guard->stack_index = stack_index;
  guard->side_exit = NULL;

  // 期待する値の設定
  switch (kind) {
    case GUARD_TYPE:
      if (!PyType_Check(expected)) {
        PyErr_SetString(PyExc_TypeError, "Expected type object for type guard");
        return -1;
      }
      guard->expected.type = (PyTypeObject *) expected;
      Py_INCREF(expected);
      break;

    case GUARD_CLASS:
      if (!PyType_Check(expected)) {
        PyErr_SetString(PyExc_TypeError, "Expected type object for class guard");
        return -1;
      }
      guard->expected.value = expected;

    case GUARD_VALUE:
      guard->expected.value = expected;
      Py_INCREF(expected);
      break;
  }

  trace->guard_conditions.length++;
  return 0;
}

/// 実行時のガード条件のチェック
int jit_check_guard(trace_guard_t *guard, PyObject *actual) {
  if (guard == NULL || actual == NULL) {
    return 0;
  }

  switch (guard->kind) {
    case GUARD_TYPE:
      return Py_TYPE(actual) == guard->expected.type;

    case GUARD_CLASS:
      return PyObject_IsInstance(actual, guard->expected.value);

    case GUARD_VALUE:
      return PyObject_RichCompareBool(actual, guard->expected.value, Py_EQ);
  }

  return 0;
}

/// サイドエクジットトレースの設定
void jit_set_side_exit(trace_guard_t*guard, trace_t *side_exit) {
  if (guard != NULL) {
    guard->side_exit = side_exit;
  }
}

/// トレースのガード条件全体のチェック
int jit_check_all_guards(trace_t *trace, PyFrameObject *frame) {
  if (trace == NULL || frame == NULL) {
    return 0;
  }

  for (Py_ssize_t i = 0; i < trace->guard_conditions.length; i++) {
    trace_guard_t *guard = &trace->guard_conditions.guards[i];

    // スタック上の値を取得
    if (guard->stack_index >= frame->f_stackdepth) {
      return 0;
    }
    PyObject *actual = frame->f_valuestack[guard->stack_index];

    if (!jit_check_guard(guard, actual)) {
      // ガード失敗時の処理
      if (guard->side_exit != NULL) {
        jit_context->current_trace = guard->side_exit;
        return 0;
      }
      return 0;
    }
  }

  return 1;
}