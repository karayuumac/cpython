/*
 * Created by karayuu on 24/10/26.
 */
#include "Python.h"
#include "internal/pycore_pystate.h"
#include "jit_internal.h"

/// トレースキャッシュのキーの生成
static PyObject *make_trace_key(PyCodeObject *code, int offset) {
  Py_INCREF(code);
  PyObject *offset_obj = PyLong_FromLong(offset);
  if (offset_obj == NULL) {
    Py_DECREF(code);
    return NULL;
  }

  PyObject *key = PyTuple_Pack(2, code, offset_obj);
  Py_DECREF(offset_obj);
  Py_DECREF(code);

  return key;
}

/// トレスのキャッシュへの登録
int jit_cache_trace(trace_t *trace) {
  if (jit_context == NULL || jit_context->trace_cache == NULL) {
    return -1;
  }

  PyObject *key = make_trace_key(trace->code, trace->start_offset);
  if (key == NULL) {
    return -1;
  }

  printf("Cache trace - key: %p, trace: %p\n",
           (void*)key, (void*)trace);  // デバッグ出力

  // トレースをPyCapsuleでラップ
  PyObject *trace_capsule = PyCapsule_New(trace, "trace", NULL);
  if (trace_capsule == NULL) {
    Py_DECREF(key);
    return -1;
  }

  int result = PyDict_SetItem(jit_context->trace_cache, key, trace_capsule);
  Py_DECREF(trace_capsule);
  Py_DECREF(key);

  return result;
}

/// キャッシュからトレースを検索する
trace_t *jit_find_trace(PyCodeObject *code, int offset) {
  if (jit_context == NULL || jit_context->trace_cache == NULL) {
    return NULL;
  }

  PyObject *key = make_trace_key(code, offset);
  if (key == NULL) {
    return NULL;
  }

  PyObject *trace_capsule = PyDict_GetItem(jit_context->trace_cache, key);
  Py_DECREF(key);

  if (trace_capsule == NULL) {
    return NULL;
  }

  trace_t *trace = PyCapsule_GetPointer(trace_capsule, "trace");
  printf("Found trace object: %p\n", (void*)trace);  // デバッグ出力

  return trace;
}

/// トレースキャッシュからの削除
int jit_remove_trace(PyCodeObject *code, int offset) {
  if (jit_context == NULL || jit_context->trace_cache == NULL) {
    return -1;
  }

  PyObject *key = make_trace_key(code, offset);
  if (key == NULL) {
    return -1;
  }

  int result = PyDict_DelItem(jit_context->trace_cache, key);
  Py_DECREF(key);

  return result;
}

/// キャッシュの全クリア
void jit_clear_trace_cache(void) {
  if (jit_context == NULL || jit_context->trace_cache == NULL) {
    return;
  }

  PyDict_Clear(jit_context->trace_cache);
}