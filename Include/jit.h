/*
 * Created by karayuu on 24/10/25.
 */

#ifndef CPYTHON_JIT_H
#define CPYTHON_JIT_H

#include "Include/Python.h"
#include "Include/frameobject.h"
#include "Include/opcode.h"
#include "Include/structmember.h"

/// トレース命令の情報を保持する構造体
typedef struct trace_instruction {
  /// バイトコード命令
  int opcode;
  /// オペランド引数
  int oparg;
  /// スタックの深さ
  int stack_depth;
  /// スタック上のスナップショット
  PyObject **stack_values;
  /// スタック上の値の型情報
  PyTypeObject **stack_types;
  struct {
    /// co_names 参照時の値
    PyObject *names;
    /// co_consts 参照時の値
    PyObject *consts;
    /// ローカル変数参照時の値
    PyObject *locals;
  } refs;
} trace_instruction_t;

/// トレース情報を保持する構造体
typedef struct trace {
  /// トレース元のコードオブジェクト
  PyCodeObject *code;
  /// トレース開始位置のバイトコードオフセット
  int start_offset;
  /// 現在記録中の位置
  int current_offset;
  /// ホットコード判定用カウンタ
  int *counter;
  /// 親トレース (サイドエクジットから始まるトレース)
  struct trace *parent;

  /// トレース命令バッファ
  struct {
    /// バッファサイズ
    Py_ssize_t capacity;
    /// 現在の要素数
    Py_ssize_t length;
    /// 命令の配列
    trace_instruction_t *instructions;
  } buffer;

  /// トレース中の型情報バッファ
  struct {
    /// 型情報バッファサイズ
    Py_ssize_t capacity;
    /// 実際の型情報数
    Py_ssize_t length;
    /// 型情報の配列
    PyTypeObject **types;
  } type_info;

  /// トレースのガード条件バッファ
  struct {
    /// ガード条件バッファサイズ
    Py_ssize_t capacity;
    /// 実際のガード条件数
    Py_ssize_t length;
    /// ガード条件の配列
    struct trace_guard *guards;
  } guard_conditions;

  /// コンパイル済みのネイティブコード
  PyObject *compiled_code;
} trace_t;

typedef enum {
  /// 型チェック用ガード
  GUARD_TYPE,
  /// クラス一致チェック用ガード
  GUARD_CLASS,
  /*
  /// オブジェクト形状チェック用ガード
  GUARD_SHAPE,
   */
  /// 値一致チェック用ガード
  GUARD_VALUE,
} guard_kind_t;

/// トレースガード情報
typedef struct trace_guard {
  /// ガードの種類
  guard_kind_t kind;
  /// チェック対象のスタック位置
  int stack_index;
  union {
    /// 期待される型
    PyTypeObject *type;
    /// 期待される値
    PyObject *value;
    /// 期待されるオブジェクト形状
    PyObject *shape;
  } expected;
  /// ガード失敗時の分岐先トレース
  struct trace *side_exit;
} trace_guard_t;

/// トレース収集の状態
typedef enum trace_state {
  /// トレース収集していない
  TRACE_INACTIVE,
  /// トレース記録中
  TRACE_RECORDING,
  /// コンパイル中
  TRACE_COMPILING,
  /// コンパイル済み
  TRACE_COMPILED
} trace_state_t;

PyAPI_FUNC(int) PyJIT_Initialize(void);

PyAPI_FUNC(void) PyJIT_Finalize(void);

PyAPI_FUNC(int) PyJIT_CheckTraceHead(PyFrameObject *frame);

PyAPI_FUNC(int) PyJIT_RecordTrace(PyFrameObject *frame, int depth);

PyAPI_FUNC(trace_t *) PyJIT_GetCurrentTrace(void);

#endif //CPYTHON_JIT_H
