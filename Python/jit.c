/*
 * Created by karayuu on 24/10/25.
 */

#include "Python.h"
#include "internal/pycore_pystate.h"
#include "opcode.h"
#include "jit_internal.h"

extern char *get_opcode_name(int opcode);

/// グローバルコンテキスト
jit_context_t *jit_context = NULL;

/// JITコンパイラの初期化
int PyJIT_Initialize(void) {
  if (jit_context != NULL) {
    return 0;
  }
  return jit_init_context();
}

/// JITコンパイラの終了処理
void PyJIT_Finalize(void) {
  if (jit_context != NULL) {
    jit_free_context();
    jit_context = NULL;
  }
}

/// 現在のトレースを取得
trace_t *PyJIT_GetCurrentTrace(void) {
  if (jit_context == NULL) {
    return NULL;
  }
  return jit_context->current_trace;
}

/// トレースヘッドのチェック
int PyJIT_CheckTraceHead(PyFrameObject *frame, PyObject **stack_pointer) {
  if (jit_context == NULL || jit_context->state != TRACE_INACTIVE) {
    return 0;
  }

  // バックワードジャンプをチェック
  int offset = frame->f_lasti;
  PyCodeObject *code = frame->f_code;

  // 既存のトレースを検索
  trace_t *existing_trace = jit_find_trace(code, offset);
  if (existing_trace != NULL)
  {
    // 既存のトレースのガード条件のチェック
    if (jit_check_all_guards(existing_trace, frame))
    {
      // 既存トレースを実行
      jit_execute_trace(PyThreadState_Get(), frame, existing_trace, stack_pointer);
      return 0;
    }
  }

  // バイトコードの取得
  _Py_CODEUNIT *instructions = (_Py_CODEUNIT *)PyBytes_AS_STRING(code->co_code);
  if (instructions == NULL) {
    return 0;
  }

  // 現在の命令を取得
  _Py_CODEUNIT instruction = instructions[offset];
  int opcode = _Py_OPCODE(instruction);
  int oparg = _Py_OPARG(instruction);

  // バックワードジャンプの判定
  int is_backward_jump = 0;
  switch (opcode) {
    case JUMP_ABSOLUTE:
    case POP_JUMP_IF_FALSE:
    case POP_JUMP_IF_TRUE:
    case JUMP_IF_FALSE_OR_POP:
    case JUMP_IF_TRUE_OR_POP:
      is_backward_jump = oparg < offset;
      break;

    default:
      return 0;
  }

  if (!is_backward_jump) {
    return 0;
  }

  // カウンタを取得・更新
  int *counter = jit_get_counter(code, offset);
  if (counter == NULL) {
    return 0;
  }
  (*counter)++;

  // ホットトレースの判定
  if (*counter > JIT_HOT_TRACE_THRESHOLD) {
    return jit_start_recording(frame);
  }

  return 0;
}

/// トレースの記録
int PyJIT_RecordTrace(PyFrameObject *frame, PyObject **stack_pointer, _Py_CODEUNIT end_f_lasti) {
  if (jit_context == NULL || jit_context->state != TRACE_RECORDING || jit_context->current_trace == NULL) {
    return 0;
  }

  // トレースの終了判定
  if (jit_should_stop_recording(frame)) {
    jit_stop_recoding(end_f_lasti);
    return 0;
  }

  // 命令を記録
  jit_record_instruction(frame, stack_pointer);
  return 0;
}