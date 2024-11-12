//
// Created by karayuu on 24/11/05.
//
#include "jit_record.h";
#include "jit_internal.h";

// Guard 失敗時には, 失敗した命令に対応するバイトコード命令から再開しなければならない
// 設計として, スタック状態を再現するような操作を行うようにする

// LOAD_CONST (定数をスタックに乗せる)
//   この命令が失敗するとしたら, 状態としてはスタックに何も載っていない状態にフォールバックしないといけない
// -> STORE_NAME -> LOAD_NAME -> LOAD_NAME ->

lir_op_t *initialize_lir_op()
{
    lir_op_t *new_lir_op = PyMem_Malloc(sizeof(lir_op_t));
    return new_lir_op;
}

/// 何もしない
void jit_record_LIR_NOPE()
{
    lir_op_t* lir = initialize_lir_op();
    lir->opcode = LIR_NOPE;
    lir->oparg.obj = NULL;
    jit_record_lir(lir);
}

/// 環境からロードしてスタックに乗せる
void jit_record_LIR_ENV_LOAD_AND_PUSH(int oparg)
{
    lir_op_t *lir_env_load = initialize_lir_op();
    lir_env_load->opcode = LIR_ENV_LOAD;
    lir_env_load->oparg.arg = oparg;
    jit_record_lir(lir_env_load);

    lir_op_t *lir_push = initialize_lir_op();
    lir_push->opcode = LIR_PUSH;
    jit_record_lir(lir_push);
}

/// 定数をスタックに乗せる
void jit_record_LIR_LOAD_CONST_LL(long long value)
{
    lir_op_t *lir = initialize_lir_op();
    lir->opcode = LIR_LOAD_CONST_LL;
    lir->oparg.ll = value;
    jit_record_lir(lir);

    lir_op_t *lir_push = initialize_lir_op();
    lir_push->opcode = LIR_PUSH;
    jit_record_lir(lir_push);
}

/// 定数をポップして環境に格納する
void jit_record_LIR_POP_AND_ENV_STORE(int oparg)
{
    lir_op_t *lir_pop = initialize_lir_op();
    lir_pop->opcode = LIR_POP;
    jit_record_lir(lir_pop);

    lir_op_t *lir_store = initialize_lir_op();
    lir_store->opcode = LIR_ENV_STORE;
    lir_store->oparg.arg = oparg;
    jit_record_lir(lir_store);
}

/// 2つの値をポップして LL 型かどうか検査し, オーバーフロー検査をして, 加算を行い, スタックに乗せる
void jit_record_LIR_LL_ADD_OVERFLOW()
{
    lir_op_t *lir_pop = initialize_lir_op();
    lir_pop->opcode = LIR_POP;
    jit_record_lir(lir_pop);

    lir_op_t *lir_pop_guard_ll = initialize_lir_op();
    lir_pop_guard_ll->opcode = LIR_GUARD_TYPE_LL;
    jit_record_lir(lir_pop_guard_ll);

    lir_op_t *lir_pop_two = initialize_lir_op();
    lir_pop_two->opcode = LIR_POP;
    jit_record_lir(lir_pop_two);

    lir_op_t *lir_pop_two_guard_ll = initialize_lir_op();
    lir_pop_two_guard_ll->opcode = LIR_GUARD_TYPE_LL;
    jit_record_lir(lir_pop_two_guard_ll);

    // TODO: ここで, スタックからポップした値をどこかに保存しておく必要あり？ (LIR_GUARD_ADD_OVERFLOW_LL)
    lir_op_t *lir_guard_add_overflow_ll = initialize_lir_op();
    lir_guard_add_overflow_ll->opcode = LIR_GUARD_ADD_OVERFLOW_LL;
    jit_record_lir(lir_guard_add_overflow_ll);

    lir_op_t *lir_add_ll = initialize_lir_op();
    lir_add_ll->opcode = LIR_ADD_LL;
    jit_record_lir(lir_add_ll);

    lir_op_t *lir_push = initialize_lir_op();
    lir_push->opcode = LIR_PUSH;
    jit_record_lir(lir_push);
}

/// 値をポップして, それが bool かどうかチェックした上で スタックに戻す
void jit_record_LIR_GUARD_TYPE_BOOL(int bool)
{
    lir_op_t *lir_pop = initialize_lir_op();
    lir_pop->opcode = LIR_POP;
    jit_record_lir(lir_pop);

    lir_op_t *lir_guard_type_bool = initialize_lir_op();
    if (bool)
    {
        lir_guard_type_bool->opcode = LIR_GUARD_TYPE_TRUE;
    }
    else
    {
        lir_guard_type_bool->opcode = LIR_GUARD_TYPE_FALSE;
    }
    jit_record_lir(lir_guard_type_bool);

    lir_op_t *lir_push = initialize_lir_op();
    lir_push->opcode = LIR_PUSH;
    jit_record_lir(lir_push);
}

/// 2つの値をポップして, 数値かどうかチェックを行い, 数値同士の比較を行い, 結果をスタックに乗せる
void jit_record_LIR_COMPARE_OP_NUM(int oparg)
{
    lir_op_t *lir_pop = initialize_lir_op();
    lir_pop->opcode = LIR_POP;
    jit_record_lir(lir_pop);

    lir_op_t *lir_pop_guard_num = initialize_lir_op();
    lir_pop_guard_num->opcode = LIR_GUARD_TYPE_NUM;
    jit_record_lir(lir_pop_guard_num);

    lir_op_t *lir_pop_two = initialize_lir_op();
    lir_pop_two->opcode = LIR_POP;
    jit_record_lir(lir_pop_two);

    lir_op_t *lir_pop_two_guard_num = initialize_lir_op();
    lir_pop_two_guard_num->opcode = LIR_GUARD_TYPE_NUM;
    jit_record_lir(lir_pop_two_guard_num);

    // #define Py_LT 0
    // #define Py_LE 1
    // #define Py_EQ 2
    // #define Py_NE 3
    // #define Py_GT 4
    // #define Py_GE 5
    lir_op_t *lir_compare_num = initialize_lir_op();
    switch (oparg)
    {
    case Py_LT:
        lir_compare_num->opcode = LIR_LT_NUM;
        break;

    case Py_LE:
        lir_compare_num->opcode = LIR_LE_NUM;
        break;

    case Py_EQ:
        lir_compare_num->opcode = LIR_EQ_NUM;
        break;

    case Py_NE:
        lir_compare_num->opcode = LIR_NE_NUM;
        break;

    case Py_GT:
        lir_compare_num->opcode = LIR_GT_NUM;
        break;

    case Py_GE:
        lir_compare_num->opcode = LIR_GE_NUM;
        break;
    }
    jit_record_lir(lir_compare_num);

    lir_op_t *lir_push = initialize_lir_op();
    lir_push->opcode = LIR_PUSH;
    jit_record_lir(lir_push);
}

void jit_record_LIR_EXIT()
{
    lir_op_t *lir_exit = initialize_lir_op();
    lir_exit->opcode = LIR_EXIT;
    jit_record_lir(lir_exit);
}

void jit_record_LIR_COMMIT()
{
    lir_op_t *lir_commit = initialize_lir_op();
    lir_commit->opcode = LIR_COMMIT;
    jit_record_lir(lir_commit);
}