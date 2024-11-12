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
  LIR_GUARD_TYPE_NUM, // 数値型 (long long, float) かどうか
  LIR_GUARD_TYPE_TRUE, // True かどうか
  LIR_GUARD_TYPE_FALSE, // False かどうか
  LIR_GUARD_ADD_OVERFLOW_LL, // 足し算の結果が long long に収まるかどうか

  /// スタックプッシュ
  LIR_PUSH,
  /// スタックポップ
  LIR_POP,
  /// 環境からのロード
  LIR_ENV_LOAD,
  /// 環境へのストア
  LIR_ENV_STORE,
  /// 定数のロード
  LIR_LOAD_CONST_LL,

  LIR_NOPE,

  /// 整数加算
  LIR_ADD_LL,

  /// 等しい
  LIR_EQ_NUM,
  /// 等しくない
  LIR_NE_NUM,
  /// より大きい
  LIR_LT_NUM,
  /// 以下
  LIR_LE_NUM,
  /// より大きい
  LIR_GT_NUM,
  /// 以上
  LIR_GE_NUM,

  /// サイドエグジット
  LIR_EXIT,

  /// 状態の確定
  LIR_COMMIT,
} lir_opcode_t;

/// レジスタタイプ
typedef enum
{
  LL, FLOAT,
} reg_type_t;

/// LIR 命令
typedef struct lir_op lir_op_t;
struct lir_op
{
  /// LIR 命令の種類
  lir_opcode_t opcode;
  /// LIR 命令に付随する引数
  union
  {
    PyObject *obj;
    int arg;
    lir_op_t *ref_op;
    long long ll;
    struct
    {
      int lhs;
      int rhs;
    } operand;
  } oparg;

  struct
  {
    /// 生成されたCコードにおけるレジスタ番号
    int register_index;
    /// レジスタタイプ
    reg_type_t type;
  } reg;
};

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
  lir_opcode_t opcode;
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
lir_inst_t *lir_new_inst(lir_opcode_t opcode);
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
