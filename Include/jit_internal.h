/*
 * Created by karayuu on 24/10/25.
 */

#ifndef CPYTHON_JIT_INTERNAL_H
#define CPYTHON_JIT_INTERNAL_H

#include "Include/Python.h"

#include "jit.h"
#include "jit_lir.h"

// トレース情報の設定
/// 最大トレース長
#define JIT_DEFAULT_TRACE_LIMIT 1000
/// ホットトレース判定閾値
#define JIT_HOT_TRACE_THRESHOLD 1000
/// バッファ初期サイズ
#define JIT_INIT_BUFFER_SIZE 128

/// トレース管理
typedef struct jit_context {
  /// 現在のトレース状態
  trace_state_t state;
  /// 記録中のトレース
  trace_t *current_trace;
  /// トレースヘッドのカウンタ管理 (dict)
  PyObject *trace_counter_map;
  /// コンパイル済みトレースのキャッシュ (dict)
  PyObject *trace_cache;
} jit_context_t;

/// グローバルコンテキスト
extern jit_context_t *jit_context;

/// トレース実行用のコンテキスト
typedef struct jit_execution_context {
  PyThreadState *tstate;
  PyFrameObject *frame;
  trace_t *trace;
  PyObject **stack_pointer;
  int error;
  int exit_on;
  PyObject f_globals_on_exit;
  PyObject f_locals_on_exit;
} jit_execution_context_t;

// 内部関数
int jit_init_context(void);
void jit_free_context(void);
int jit_start_recording(PyFrameObject *frame);
void jit_stop_recoding(_Py_CODEUNIT end_f_lasti);
int jit_should_stop_recording(PyFrameObject *frame);
void jit_record_instruction(PyFrameObject *frame, PyObject **stack_pointer);
int jit_is_support_opcode(int opcode);
void jit_dump_trace(trace_t *trace);

trace_t *jit_create_trace(PyFrameObject *frame);
void jit_free_trace(trace_t *trace);
int jit_add_instruction(trace_t *trace, PyFrameObject *frame);
int jit_add_guard(trace_t *trace, guard_kind_t kind, int stack_index, PyObject *expected);
int *jit_get_counter(PyCodeObject *code, int offset);

int jit_cache_trace(trace_t *trace);
trace_t *jit_find_trace(PyCodeObject *code, int offset);
int jit_remove_trace(PyCodeObject *code, int offset);
void jit_clear_trace_cache(void);

int jit_optimize_trace(trace_t *trace);
int jit_compile_trace(trace_t *trace);
_Py_CODEUNIT jit_execute_trace(PyThreadState *tstate, PyFrameObject *frame, trace_t *trace, PyObject **stack_pointer);
int jit_check_all_guards(trace_t *trace, PyFrameObject *frame);
int jit_check_guard(trace_guard_t *guard, PyObject *actual);

void jit_record_lir(lir_op_t *lir_op) ;

/// トレースからLIRへの変換
void allocate_lir_register(trace_t* trace);
/// コード生成
char* generate_c_code(trace_t* trace);

#endif //CPYTHON_JIT_INTERNAL_H
