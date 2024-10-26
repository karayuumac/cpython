/*
 * Created by karayuu on 24/10/26.
 */
#include "Python.h"
#include "jit_internal.h"

static PyObject *get_trace_info(PyObject *self, PyObject *args) {
  PyObject *func = NULL;
  PyObject *result_dict = NULL;

  if (!PyArg_ParseTuple(args, "O:get_trace_info", &func)) {
    return NULL;
  }

  if (!PyFunction_Check(func)) {
    PyErr_SetString(PyExc_TypeError, "Expected a function object");
    return NULL;
  }

  // 関数のコードオブジェクトを取得
  PyCodeObject *code = (PyCodeObject *) PyFunction_GET_CODE(func);

  // トレース情報を格納する辞書の作成
  result_dict = PyDict_New();
  if (result_dict == NULL) {
    return NULL;
  }

  // トレース情報の収集
  if (jit_context != NULL && jit_context->trace_cache != NULL) {
    PyObject *traces = PyList_New(0);
    if (traces == NULL) {
      Py_DECREF(result_dict);
      return NULL;
    }

    // コードオブジェクトに関連する全てのトレースを探す
    for (Py_ssize_t i = 0; i < PyBytes_GET_SIZE(code->co_code); i++) {
      trace_t *trace = jit_find_trace(code, i);
      if (trace != NULL) {
        // トレース情報を辞書に追加
        PyObject *trace_dict = PyDict_New();
        if (trace_dict == NULL) {
          Py_DECREF(traces);
          Py_DECREF(result_dict);
          return NULL;
        }

        // トレース開始位置
        PyDict_SetItemString(trace_dict, "start_offset", PyLong_FromLong(trace->start_offset));

        // 実行回数
        if (trace->counter != NULL) {
          PyDict_SetItemString(trace_dict, "execution_count", PyLong_FromLong(*trace->counter));
        }

        // 命令情報
        PyObject *instructions = PyList_New(0);
        for (Py_ssize_t j = 0; j < trace->buffer.length; j++) {
          trace_instruction_t *inst = &trace->buffer.instructions[j];
          PyObject *inst_dict = PyDict_New();

          PyDict_SetItemString(inst_dict, "opcode", PyLong_FromLong(inst->opcode));
          PyDict_SetItemString(inst_dict, "oparg", PyLong_FromLong(inst->oparg));
          PyDict_SetItemString(inst_dict, "stack_depth", PyLong_FromLong(inst->stack_depth));

          // スタック情報
          PyObject *stack_types = PyList_New(inst->stack_depth);
          for (int k = 0; k < inst->stack_depth; k++) {
            PyTypeObject *type = inst->stack_types[k];
            if (type != NULL) {
              PyList_SET_ITEM(stack_types, k, PyUnicode_FromString(type->tp_name));
            } else {
              PyList_SET_ITEM(stack_types, k, Py_None);
            }
          }
          PyDict_SetItemString(inst_dict, "stack_types", stack_types);
          Py_DECREF(stack_types);

          PyList_Append(instructions, inst_dict);
          Py_DECREF(inst_dict);
        }
        PyDict_SetItemString(trace_dict, "instructions", instructions);
        Py_DECREF(instructions);

        // ガード条件
        PyObject *guards = PyList_New(0);
        for (Py_ssize_t j = 0; j < trace->guard_conditions.length; j++) {
          trace_guard_t *guard = &trace->guard_conditions.guards[j];
          PyObject *guard_dict = PyDict_New();

          PyDict_SetItemString(guard_dict, "kind", PyLong_FromLong(guard->kind));
          PyDict_SetItemString(guard_dict, "stack_index", PyLong_FromLong(guard->stack_index));

          PyList_Append(guards, guard_dict);
          Py_DECREF(guard_dict);
        }
        PyDict_SetItemString(trace_dict, "guards", guards);
        Py_DECREF(guards);

        PyList_Append(traces, trace_dict);
        Py_DECREF(trace_dict);
      }
    }

    PyDict_SetItemString(result_dict, "traces", traces);
    Py_DECREF(traces);
  }
  return result_dict;
}

static PyMethodDef TraceMethods[] = {
  {"get_trace_info", get_trace_info, METH_VARARGS, "Get trace information for a given function"},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef tracemodule = {
  PyModuleDef_HEAD_INIT,
  "trace_debug",
  NULL,
  -1,
  TraceMethods
};

PyMODINIT_FUNC PyInit_trace_debug(void) {
  return PyModule_Create(&tracemodule);
}