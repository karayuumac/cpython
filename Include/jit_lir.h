//
// Created by karayuu on 24/10/27.
//

#ifndef CPYTHON_JIT_LIR_H
#define CPYTHON_JIT_LIR_H

#include "Python.h"

#include "jit.h"

/// LIR命令の種類
typedef enum
{
  /// 型チェックガード
  LIR_GUARD_TYPE_LL, // long long 型かどうか
  /// 定数のロード
  LIR_LOAD_CONST_LL,


  LIR_NOPE,
  /// 定数ロード
  LIR_CONST,
  /// 名前ロード
  LIR_LOAD_NAME,
  /// 変数ロード
  LIR_LOAD_STACK,
  /// 変数ストア
  LIR_STORE_NAME,

  /// 整数加算
  LIR_ADD,
  /// 整数減算
  LIR_SUB,
  /// 整数乗算
  LIR_MUL,
  /// 整数除算
  LIR_DIV,
  /// 剰余
  LIR_MOD,
  /// ビット論理積
  LIR_AND,
  /// ビット論理和
  LIR_OR,
  /// ビット排他的論理和
  LIR_XOR,
  /// 左シフト
  LIR_SHL,
  /// 右シフト
  LIR_SHR,

  /// 浮動小数点加算
  LIR_FADD,
  /// 浮動小数点減算
  LIR_FSUB,
  /// 浮動小数点乗算
  LIR_FMUL,
  /// 浮動小数点除算
  LIR_FDIV,

  /// 等しい
  LIR_EQ,
  /// 等しくない
  LIR_NE,
  /// より大きい
  LIR_LT,
  /// 以下
  LIR_LE,
  /// より大きい
  LIR_GT,
  /// 以上
  LIR_GE,

  /// 型チェックガード
  LIR_GUARD_TYPE,
  /// オーバーフローガード
  LIR_GUARD_ADD_LL_OVERFLOW,
  LIR_GUARD_SUB_LL_OVERFLOW,
  LIR_GUARD_MUL_LL_OVERFLOW,
  /// 値の一致チェックガード
  LIR_GUARD_VALUE,
  /// クラスチェックガード
  LIR_GUARD_CLASS,

  /// 無条件ジャンプ
  LIR_JUMP,
  /// 条件分岐
  LIR_BRANCH,
  /// リターン
  LIR_RETURN,
  /// 例外
  LIR_THROW,

  /// 属性取得
  LIR_GETATTR,
  /// 属性設定
  LIR_SETATTR,
  /// 要素取得
  LIR_GETITEM,
  /// 要素設定
  LIR_SETITEM,
  /// グローバル変数取得
  LIR_GETGLOBAL,
  /// グローバル変数設定
  LIR_SETGLOBAL,
  /// メソッド呼び出し
  LIR_CALL,

  /// スタックプッシュ
  LIR_PUSH,
  /// スタックポップ
  LIR_POP,

  /// サイドエグジット
  LIR_EXIT,
} lir_optcode_t;

/// ガードエラーの種類
typedef enum
{
  GUARD_ERROR_NONE = 0,
  /// 型不一致
  GUARD_ERROR_TYPE,
  /// オーバーフロー
  GUARD_ERROR_METHOD,
  /// 値不一致
  GUARD_ERROR_VALUE,
  /// クラス不一致
  GUARD_ERROR_CLASS,
} guard_error_t;

/// LIR命令のオペランド型
typedef struct
{
  enum
  {
    OPERAND_NONE,
    /// ヒープ領域位置
    OPERAND_HEAP,
    /// レジスタ変数位置
    OPERAND_REG,
    /// スタック位置
    OPERAND_STACK,
    /// サイドエグジット位置
    OPERAND_EXIT,
    /// 型情報
    OPERAND_TYPE,
    /// メソッド情報
    OPERAND_METHOD,
    /// ジャンプラベル
    OPERAND_LABEL,
  } kind;

  union
  {
    /// 定数配列のインデックス
    int heap_index;
    /// レジスタ番号
    int reg_num;
    /// スタック位置
    int stack_pos;
    /// サイドエグジット
    struct trace* exit;
    /// 型情報
    PyTypeObject* type;
    /// メソッド情報
    PyObject* method;
    /// ラベル番号
    int label;
  } u;
} lir_operand_t;

/// LIR命令
typedef struct lir_inst
{
  /// 命令種類
  lir_optcode_t opcode;
  /// 結果格納先
  lir_operand_t dest;
  /// ソースオペランド
  lir_operand_t src1;
  lir_operand_t src2;
  /// 次の命令へのリンク
  struct lir_inst *next;
  /// ガード失敗時の分岐先
  struct trace *guard_exit;
} lir_inst_t;

/// LIRコードブロック
typedef struct lir_block
{
  /// 次のブロックへのリンク
  struct lir_block *next;
  /// ブロックの先頭命令
  lir_inst_t *first;
  /// ブロックの末尾命令
  lir_inst_t *last;
  /// ブロックのラベル
  int label;
} lir_block_t;

/// LIRコード
typedef struct
{
  /// 入口ブロック
  lir_block_t *entry;
  /// 現在ブロック
  lir_block_t *current;
  /// 使用レジスタ数
  int reg_count;
  /// ラベル数
  int label_count;
  /// 定数プール
  PyObject **constants;
  /// 定数の数
  int const_count;
} lir_code_t;

// LIR関連の関数宣言
lir_code_t *lir_create(void);
void lir_free(lir_code_t *lir);
lir_block_t* lir_new_block(lir_code_t *lir);
lir_inst_t *lir_new_inst(lir_optcode_t opcode);
void lir_append_inst(lir_block_t *block, lir_inst_t *inst);
int lir_add_constant(lir_code_t *lir, PyObject *const_var);

// オペランド生成関数
lir_operand_t lir_none_operand(void);
lir_operand_t lir_heap_operand(int heap_index);
lir_operand_t lir_reg_operand(int reg_num);
lir_operand_t lir_stack_operand(int stack_pos);
lir_operand_t lir_exit_operand(struct trace *exit);
lir_operand_t lir_type_operand(PyTypeObject *type);
lir_operand_t lir_method_operand(PyObject *method);
lir_operand_t lir_label_operand(int label);

// char* generate_operand_code(lir_operand_t *operand);
#endif //CPYTHON_JIT_LIR_H
