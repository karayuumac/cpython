/*
 * Created by karayuu on 24/10/25.
 */
#include "Python.h"
#include "opcode.h"
#include "internal/pycore_pystate.h"
#include "jit_internal.h"

/// コンテキストの初期化
int jit_init_context(void)
{
    jit_context = (jit_context_t*)PyMem_Malloc(sizeof(jit_context_t));
    if (jit_context == NULL)
    {
        return -1;
    }

    jit_context->state = TRACE_INACTIVE;
    jit_context->current_trace = NULL;

    // トレースカウンタマップの初期化
    jit_context->trace_counter_map = PyDict_New();
    if (jit_context->trace_counter_map == NULL)
    {
        PyMem_Free(jit_context);
        jit_context = NULL;
        return -1;
    }

    // トレースキャッシュの初期化
    jit_context->trace_cache = PyDict_New();
    if (jit_context->trace_cache == NULL)
    {
        Py_DECREF(jit_context->trace_counter_map);
        PyMem_Free(jit_context);
        jit_context = NULL;
        return -1;
    }

    return 0;
}

/// コンテキストの解放
void jit_free_context(void)
{
    if (jit_context != NULL)
    {
        Py_XDECREF(jit_context->trace_counter_map);
        Py_XDECREF(jit_context->trace_cache);
        if (jit_context->current_trace != NULL)
        {
            jit_free_trace(jit_context->current_trace);
        }
        PyMem_Free(jit_context);
    }
}

/// トレースの作成
trace_t* jit_create_trace(PyFrameObject* frame)
{
    trace_t* trace = (trace_t*)PyMem_Malloc(sizeof(trace_t));
    if (trace == NULL)
    {
        return NULL;
    }

    // 基本情報の初期化
    trace->code = frame->f_code;
    Py_INCREF(trace->code);
    trace->start_offset = frame->f_lasti;
    trace->current_offset = frame->f_lasti;
    trace->parent = NULL;
    trace->counter = NULL;
    trace->compiled_code = NULL;
    trace->end_f_lasti = 0;

    // 命令バッファの初期化
    trace->buffer.capacity = JIT_INIT_BUFFER_SIZE;
    trace->buffer.length = 0;
    trace->buffer.instructions = (trace_instruction_t*)PyMem_Malloc(sizeof(trace_instruction_t) * JIT_INIT_BUFFER_SIZE);
    if (trace->buffer.instructions == NULL)
    {
        Py_DECREF(trace->code);
        PyMem_Free(trace);
        return NULL;
    }

    // 型情報バッファの初期化
    trace->type_info.capacity = JIT_INIT_BUFFER_SIZE;
    trace->type_info.length = 0;
    trace->type_info.types = (PyTypeObject**)PyMem_Malloc(sizeof(PyTypeObject*) * JIT_INIT_BUFFER_SIZE);
    if (trace->type_info.types == NULL)
    {
        PyMem_Free(trace->buffer.instructions);
        Py_DECREF(trace->code);
        PyMem_Free(trace);
        return NULL;
    }

    // ガード条件バッファの初期化
    trace->guard_conditions.capacity = JIT_INIT_BUFFER_SIZE;
    trace->guard_conditions.length = 0;
    trace->guard_conditions.guards = (trace_guard_t*)PyMem_Malloc(sizeof(trace_guard_t) * JIT_INIT_BUFFER_SIZE);
    if (trace->guard_conditions.guards == NULL)
    {
        PyMem_Free(trace->type_info.types);
        PyMem_Free(trace->buffer.instructions);
        Py_DECREF(trace->code);
        PyMem_Free(trace);
        return NULL;
    }

    // lir命令バッファの初期化
    trace->lir_buffer.capacity = JIT_INIT_BUFFER_SIZE;
    trace->lir_buffer.length = 0;
    trace->lir_buffer.lirs = (lir_op_t *) PyMem_Malloc(sizeof(lir_op_t) * JIT_INIT_BUFFER_SIZE);
    if (trace->lir_buffer.lirs == NULL)
    {
        PyMem_Free(trace->type_info.types);
        PyMem_Free(trace->buffer.instructions);
        PyMem_Free(trace->guard_conditions.guards);
        Py_DECREF(trace->code);
        PyMem_Free(trace);
        return NULL;
    }

    return trace;
}

/// トレースの解放
void jit_free_trace(trace_t* trace)
{
    if (trace == NULL)
    {
        return;
    }

    // 命令バッファの解放
    if (trace->buffer.instructions != NULL)
    {
        for (Py_ssize_t i = 0; i < trace->buffer.length; i++)
        {
            trace_instruction_t* inst = &trace->buffer.instructions[i];

            // スタック情報の解放
            if (inst->stack_values != NULL)
            {
                // スタック上の値の参照カウントを減らす
                for (int j = 0; j < inst->stack_depth; j++)
                {
                    Py_XDECREF(inst->stack_values[j]);
                }
                PyMem_Free(inst->stack_values);
            }

            if (inst->stack_types != NULL)
            {
                PyMem_Free(inst->stack_types);
            }

            // 参照情報の解放
            Py_XDECREF(inst->refs.names);
            Py_XDECREF(inst->refs.consts);
            Py_XDECREF(inst->refs.locals);
        }
        PyMem_Free(trace->buffer.instructions);
        trace->buffer.instructions = NULL;
    }

    // 型情報バッファの解放
    if (trace->type_info.types != NULL)
    {
        PyMem_Free(trace->type_info.types);
        trace->type_info.types = NULL;
    }

    // ガード条件の解放
    if (trace->guard_conditions.guards != NULL)
    {
        for (Py_ssize_t i = 0; i < trace->guard_conditions.length; i++)
        {
            trace_guard_t* guard = &trace->guard_conditions.guards[i];

            switch (guard->kind)
            {
            case GUARD_TYPE:
                Py_XDECREF(guard->expected.type);
                break;

            case GUARD_CLASS:
            case GUARD_VALUE:
                Py_XDECREF(guard->expected.value);
                break;
            }

            guard->side_exit = NULL;
        }
        PyMem_Free(trace->guard_conditions.guards);
        trace->guard_conditions.guards = NULL;
    }

    // コードオブジェクトの参照を解放
    Py_XDECREF(trace->code);

    // コンパイル済みコードの解放
    if (trace->compiled_code != NULL)
    {
        trace->compiled_code = NULL;
    }

    trace->counter = NULL;
    trace->parent = NULL;
    PyMem_Free(trace);
}

/// トレースカウンタの取得・作成
int* jit_get_counter(PyCodeObject* code, int offset)
{
    if (jit_context == NULL || jit_context->trace_counter_map == NULL)
    {
        return NULL;
    }

    // カウンタのキーを作成
    PyObject* key = PyTuple_Pack(2, (PyObject*)code, PyLong_FromLong(offset));
    if (key == NULL)
    {
        return NULL;
    }

    // 既存のカウンタの検索
    PyObject* counter_obj = PyDict_GetItem(jit_context->trace_counter_map, key);
    if (counter_obj == NULL)
    {
        // 新しいカウンタを作成する
        int* counter = (int*)PyMem_Malloc(sizeof(int));
        if (counter == NULL)
        {
            Py_DECREF(key);
            return NULL;
        }
        *counter = 1;

        // カウンタをPyCapsuleでラップ
        counter_obj = PyCapsule_New(counter, "trace_counter", NULL);
        if (counter_obj == NULL)
        {
            PyMem_Free(counter);
            Py_DECREF(key);
            return NULL;
        }

        // カウンタを登録
        if (PyDict_SetItem(jit_context->trace_counter_map, key, counter_obj) < 0)
        {
            Py_DECREF(counter_obj);
            Py_DECREF(key);
            return NULL;
        }
        Py_DECREF(counter_obj);
    }
    Py_DECREF(key);

    return (int*)PyCapsule_GetPointer(counter_obj, "trace_counter");
}

/// トレースの記録開始
int jit_start_recording(PyFrameObject* frame)
{
    if (jit_context == NULL || jit_context->state != TRACE_INACTIVE)
    {
        return 0;
    }

    // 新しいトレースを作成
    trace_t* trace = jit_create_trace(frame);
    if (trace == NULL)
    {
        return -1;
    }

    // カウンタを関連づける
    trace->counter = jit_get_counter(frame->f_code, frame->f_lasti);
    if (trace->counter == NULL)
    {
        jit_free_trace(trace);
        return -1;
    }

    jit_context->current_trace = trace;
    jit_context->state = TRACE_RECORDING;
    printf("start recording!\n");
    return 0;
}

/// トレースの記録を中止する
void jit_stop_recoding(_Py_CODEUNIT end_f_lasti)
{
    if (jit_context == NULL || jit_context->state != TRACE_RECORDING)
    {
        return;
    }

    trace_t* trace = jit_context->current_trace;
    trace->end_f_lasti = end_f_lasti;
    jit_dump_trace(trace);

    // トレースが有効な場合はキャッシュに追加する
    if (trace->buffer.length > 0)
    {
        // トレースの最適化
        jit_optimize_trace(trace);

        // コンパイルする
        if (jit_compile_trace(trace))
        {
            // キャッシュに登録する
            jit_cache_trace(trace);
        }
    }
    else
    {
        jit_free_trace(trace);
    }

    jit_context->current_trace = NULL;
    jit_context->state = TRACE_INACTIVE;
    printf("stop recording\n");
}

/// 命令の記録
void jit_record_instruction(PyFrameObject* frame, PyObject **stack_pointer)
{
    trace_t* trace = jit_context->current_trace;
    if (trace == NULL)
    {
        return;
    }

    // もしバッファが一杯なら, リサイズする
    if (trace->buffer.length >= trace->buffer.capacity)
    {
        Py_ssize_t new_capacity = trace->buffer.capacity * 2;
        trace_instruction_t* new_instructions = (trace_instruction_t*)PyMem_Realloc(
            trace->buffer.instructions, sizeof(trace_instruction_t) * new_capacity);
        if (new_instructions == NULL)
        {
            return;
        }
        trace->buffer.instructions = new_instructions;
        trace->buffer.capacity = new_capacity;
    }

    // 新しい命令を追加する
    trace_instruction_t* inst = &trace->buffer.instructions[trace->buffer.length];
    int offset = frame->f_lasti;
    _Py_CODEUNIT* instructions = (_Py_CODEUNIT*)PyBytes_AS_STRING(frame->f_code->co_code);
    _Py_CODEUNIT instruction = instructions[offset];
    int opcode = _Py_OPCODE(instruction);
    int oparg = _Py_OPARG(instruction);

    // 基本情報の記録
    inst->opcode = opcode;
    inst->oparg = oparg;
    inst->f_lasti = offset;
    inst->stack_depth = abs(stack_pointer - frame->f_valuestack);

    // スタック情報のコピー
    inst->stack_values = PyMem_Malloc(sizeof(PyObject*) * inst->stack_depth);
    inst->stack_types = PyMem_Malloc(sizeof(PyTypeObject*) * inst->stack_depth);
    if (inst->stack_values == NULL || inst->stack_types == NULL)
    {
        PyMem_Free(inst->stack_values);
        PyMem_Free(inst->stack_types);
        return;
    }

    // スタック上の値と型情報を記録する
    for (int i = 1; i <= inst->stack_depth; i++)
    {
        PyObject* obj = stack_pointer[-i];
        Py_XINCREF(obj);
        inst->stack_values[i - 1] = obj;
        inst->stack_types[i - 1] = obj ? Py_TYPE(obj) : NULL;
    }

    // 参照情報の初期化
    inst->refs.names = NULL;
    inst->refs.consts = NULL;
    inst->refs.locals = NULL;

    // 命令に応じた追加情報の記録
    switch (inst->opcode)
    {
    case LOAD_CONST:
        {
            PyObject* const_val = PyTuple_GET_ITEM(frame->f_code->co_consts, inst->oparg);
            if (const_val != NULL)
            {
                Py_INCREF(const_val);
                inst->refs.consts = const_val;
            }
            break;
        }

    case LOAD_NAME:
    case LOAD_GLOBAL:
        {
            PyObject* name = PyTuple_GET_ITEM(frame->f_code->co_names, inst->oparg);
            if (name != NULL)
            {
                Py_INCREF(name);
                inst->refs.names = name;
            }
            break;
        }

    case LOAD_FAST:
        {
            PyObject* local = frame->f_localsplus[inst->oparg];
            if (local != NULL)
            {
                Py_INCREF(local);
                inst->refs.locals = local;
            }
            break;
        }

    default:
        break;
    }

    trace->buffer.length++;
    trace->current_offset = offset;
}

/// トレース終了条件のチェック
int jit_should_stop_recording(PyFrameObject* frame)
{
    trace_t* trace = jit_context->current_trace;
    if (trace == NULL)
    {
        return 1;
    }

    // トレース長の制限
    if (trace->buffer.length >= JIT_DEFAULT_TRACE_LIMIT)
    {
        return 1;
    }

    // ループの検出 (トレースの開始位置に戻ってきた)
    if (frame->f_lasti == trace->start_offset && trace->buffer.length > 0)
    {
        return 1;
    }

    // 既存トレースの開始位置に到達
    trace_t *existing_trace = jit_find_trace(frame->f_code, frame->f_lasti);
    if (existing_trace != NULL)
    {
        return 1;
    }

    // 例外が発生した場合
    if (PyErr_Occurred())
    {
        return 1;
    }

    return 0;
}

/// サポートする命令のチェック
int jit_is_support_opcode(int opcode)
{
    switch (opcode)
    {
    // case BINARY_AND:
    // case BINARY_SUBTRACT:
    // case BINARY_MULTIPLY:
    // case BINARY_TRUE_DIVIDE:
    // case BINARY_FLOOR_DIVIDE:
    // case BINARY_MODULO:
    // case BINARY_POWER:
    // case COMPARE_OP:
    // case JUMP_ABSOLUTE:
    // case POP_JUMP_IF_TRUE:
    // case POP_JUMP_IF_FALSE:
    // case LOAD_FAST:
    // case STORE_FAST:
    // case LOAD_METHOD:
    // case CALL_METHOD:
    case FOR_ITER:
    case STORE_NAME:
    case LOAD_CONST:
    case JUMP_ABSOLUTE:
        return 1;
    default:
        return 0;
    }
}

/// トレースの最適化を行う
int jit_optimize_trace(trace_t* trace)
{
    return 0;
}

/// トレースの実行を行う
_Py_CODEUNIT jit_execute_trace(PyThreadState* tstate, PyFrameObject* frame, trace_t* trace, PyObject **stack_pointer)
{
    jit_execution_context_t ctx = {
        .tstate = tstate,
        .frame = frame,
        .trace = trace,
        .stack_pointer = stack_pointer,
        .error = 0,
        .exit_on = -1,
        // exit 時に備えて, 初期状態の環境で初期化しておく.
        // COMMIT 命令でこの値を現在のフレームの環境に書き換える.
        // exit 時には, この値でロールバックする.
        .f_globals_on_exit = *frame->f_globals,
        .f_locals_on_exit = *frame->f_locals,
    };

    // トレースの実行
    _Py_CODEUNIT side_exit_f_lasti = trace->compiled_code(&ctx);

    if (ctx.error)
    {
        // printf("exit_on: %d\n    side_exit: %u\n", ctx.exit_on, side_exit_f_lasti);
        if (PyErr_Occurred())
        {
            return side_exit_f_lasti;
        }

        // サイドエグジットの場合
        // 新しいトレースの記録を開始する
        // PyJIT_CheckTraceHead(frame, stack_pointer);

        return side_exit_f_lasti;
    }

    // 正常終了時の処理
    // トレースの最後の命令位置に更新
    return trace->end_f_lasti;
}

void jit_record_lir(lir_op_t* lir_op)
{
    trace_t* trace = jit_context->current_trace;
    if (trace == NULL)
    {
        return;
    }

    // もしバッファが一杯なら, リサイズする
    if (trace->lir_buffer.length >= trace->lir_buffer.capacity)
    {
        Py_ssize_t new_capacity = trace->lir_buffer.capacity * 2;
        lir_op_t *new_lirs = PyMem_Realloc(trace->lir_buffer.lirs, sizeof(lir_op_t) * new_capacity);
        if (new_lirs == NULL)
        {
            return;
        }
        trace->lir_buffer.lirs = new_lirs;
        trace->lir_buffer.capacity = new_capacity;
    }

    trace->lir_buffer.lirs[trace->lir_buffer.length] = *lir_op;
    trace->lir_buffer.length++;
}