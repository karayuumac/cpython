//
// Created by root on 24/10/28.
//

#include "jit_internal.h"
#include "jit_runtime.h"
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#ifndef JIT_INCLUDE_PATHS
#error "JIT_INCLUDE_PATHS is not defined. Check your Makefile."
#endif

/*
char* generate_operand_code(lir_operand_t *operand)
{
  static char code[65536];
  char* p = code;

  switch (operand->kind)
  {
  case OPERAND_NONE:
    p += sprintf(p, "");
    break;

  case OPERAND_CONST:
    p += sprintf(p, "ctx->frame->f_code->const[%d]", operand->u.const_index);
    break;

  case OPERAND_REG:
    p += sprintf(p, "v%d", operand->u.reg_num);

  default:
    p += sprintf(p, "");
    break;
  }

  return p;
}
*/

#define MAX_ELEMENT 100000

typedef struct Reg_
{
  int reg_index;
  // reg_type_t type;
} Reg;

typedef struct Stack_
{
  Reg *stack[MAX_ELEMENT];
  int sp;
  int peek_sp;
} Stack;

void Stack_Construct(Stack* stack)
{
  stack->sp = 0;
  stack->peek_sp = 0;
}

int Stack_Push(Stack *stack, Reg *elem)
{
  if (stack->sp == MAX_ELEMENT)
  {
    return 0;
  }
  stack->stack[stack->sp] = elem;
  stack->sp++;
  stack->peek_sp++;
  return 1;
}

Reg *Stack_Pop(Stack *stack)
{
  if (stack->sp == 0)
  {
    return NULL;
  }
  stack->sp--;
  stack->peek_sp--;
  return stack->stack[stack->sp];
}

Reg *Stack_Peek_And_Move(Stack *stack)
{
  if (stack->peek_sp == 0)
  {
    return NULL;
  }
  stack->peek_sp--;
  return stack->stack[stack->peek_sp];
}

void Stack_Peek_Restore(Stack *stack)
{
  stack->peek_sp = stack->sp;
}

Reg *Stack_Peek(Stack *stack)
{
  if (stack->sp == 0)
  {
    return NULL;
  }
  return stack->stack[stack->sp - 1];
}

typedef struct Queue_
{
  Reg *queue[MAX_ELEMENT];
  int queue_head;
  int s_queue_head;
  int inplace_head;
  int length;
} Queue;

void Queue_Construct(Queue *queue)
{
  queue->length = 0;
  queue->queue_head = 0;
  queue->inplace_head = 0;
  queue->s_queue_head = 0;
}

void Queue_Enqueue(Queue *queue, Reg *elem)
{
  queue->queue[queue->inplace_head % MAX_ELEMENT] = elem;
  queue->inplace_head++;
  queue->length++;
}

Reg *Queue_Dequeue(Queue *queue)
{
  if (queue->length > 0)
  {
    const int queue_head = queue->queue_head;
    queue->queue_head = (queue->queue_head + 1) % MAX_ELEMENT;
    queue->s_queue_head = (queue->s_queue_head + 1) % MAX_ELEMENT;
    queue->length--;
    return queue->queue[queue_head];
  }
  return NULL;
}

Reg *Queue_Peek_And_Move(Queue *queue)
{
  if (queue->s_queue_head < queue->inplace_head)
  {
    int s_queue_head = queue->s_queue_head;
    queue->s_queue_head = (queue->s_queue_head + 1) % MAX_ELEMENT;
    return queue->queue[s_queue_head];
  }
  return NULL;
}

void Queue_Peek_Restore(Queue *queue)
{
  queue->s_queue_head = queue->queue_head;
}

Reg *Queue_Peek(Queue *queue)
{
  return queue->queue[queue->queue_head];
}

typedef struct env_
{
  /// レジスタ
  Reg *reg;
  /// oparg で与えられる環境上のインデックス
  int f_env_index;
} env_t;

typedef struct env_list_
{
  env_t *envs;
  Py_ssize_t length;
  Py_ssize_t capacity;
  // ここまでは反映して良い index
  Py_ssize_t last_commited_index;
  Py_ssize_t commited_index;
} env_list_t;

env_list_t *initialize()
{
  env_list_t *env_list = PyMem_Malloc(sizeof(env_list_t));
  env_list->capacity = JIT_INIT_BUFFER_SIZE;
  env_list->length = 0;
  env_list->last_commited_index = -1;
  env_list->commited_index = -1;
  env_list->envs = PyMem_Malloc(sizeof(env_t) * JIT_INIT_BUFFER_SIZE);
  return env_list;
}

void check_capacity(env_list_t *env_list)
{
  if (env_list->length >= env_list->capacity)
  {
    Py_ssize_t new_capacity = env_list->capacity * 2;
    env_t *new_env_list = PyMem_Realloc(env_list->envs, sizeof(env_t) * new_capacity);
    env_list->capacity = new_capacity;
    env_list->envs = new_env_list;
  }
}

void append(env_list_t *env_list, env_t *element)
{
  check_capacity(env_list);
  env_list->envs[env_list->length] = *element;
  env_list->length++;
}

void free_env(env_list_t *env_list)
{
  PyMem_Free(env_list->envs);
  PyMem_Free(env_list);
}

void commit(env_list_t *env_list)
{
  env_list->commited_index = env_list->length - 1;
}

char* reg_to_py_object(int reg_index, reg_type_t type)
{
  char *c = PyMem_Malloc(sizeof(char) * 256);
  switch (type)
  {
  case LL:
    sprintf(c, "Py_BuildValue(\"L\", v%d)", reg_index);
    break;
  }
  return c;
}

char* commit_f_locals(env_list_t *f_locals)
{
  char *c = PyMem_Malloc(sizeof(char) * 65536);
  char *tp = c;

  if (f_locals->last_commited_index + 1 > f_locals->commited_index)
  {
    return tp;
  }

  c += sprintf(c,
    "    if (PyDict_CheckExact(locals)) {\n"
    );

  for (Py_ssize_t i = f_locals->last_commited_index + 1; i <= f_locals->commited_index; i++)
  {
    env_t env = f_locals->envs[i];
    c += sprintf(c,
      "        PyDict_SetItem(locals, GETITEM(names, %d), v%d);\n",
      env.f_env_index, env.reg->reg_index);
  }

  c += sprintf(c,
    "    } else {\n"
    );

  for (Py_ssize_t i = f_locals->last_commited_index + 1; i <= f_locals->commited_index; i++)
  {
    env_t env = f_locals->envs[i];
    c += sprintf(c,
      "        PyDict_SetItem(locals, GETITEM(names, %d), v%d);\n",
      env.f_env_index, env.reg->reg_index);
  }

  c += sprintf(c,
    "    }\n");

  f_locals->last_commited_index = f_locals->commited_index;

  return tp;
}

char* generate_c_code(trace_t *trace)
{
  static char code[6553600];
  static char exit[3000000];
  char* p = code;
  char* exit_p = exit;

  Stack *value_stack = PyMem_Malloc(sizeof(Stack));
  Stack_Construct(value_stack);

  // TODO: Queue にしないとダメ
  Queue *popped_reg_queue = PyMem_Malloc(sizeof(Queue));
  Queue_Construct(popped_reg_queue);

  env_list_t *f_locals = initialize();

  // ヘッダーとインクルード
  p += sprintf(p,
               "#include \"jit_internal.h\"\n"
               "#include \"jit_runtime.h\"\n"
               "#include <stdio.h>\n"
               "\n"
               "#define GETITEM(v, i) PyTuple_GET_ITEM((v), (i))\n" // TODO: 速度向上のため, name をストアすることを検討する
               "PyObject* trace_func(jit_execution_context_t* ctx) {\n"
               "    PyObject *locals = ctx->frame->f_locals;\n"
               "    PyObject *names = ctx->frame->f_code->co_names;\n"
               "    PyObject *v0 = NULL;\n"
               );

  int reg_index = 1;
  int prev_reg = -1;
  int label_index = 0;
  for (int i = 1; i < trace->lir_buffer.length; i++)
  {
    lir_op_t lir_op = trace->lir_buffer.lirs[i];

    switch (lir_op.opcode)
    {
    case LIR_LOAD_CONST_LL:
      {
        p += sprintf(p,
          "    PyObject *v%d = Py_BuildValue(\"L\", %lld);\n",
          reg_index, lir_op.oparg.ll);
        prev_reg = reg_index;
        reg_index++;
        break;
      }

    case LIR_PUSH:
      {
        assert(prev_reg != -1);

        Reg *reg = PyMem_Malloc(sizeof(Reg));
        reg->reg_index = prev_reg;
        // reg->type = lir_op.reg.type;

        Stack_Push(value_stack, reg);
        break;
      }

    case LIR_POP:
      {
        Reg* elem = Stack_Pop(value_stack);
        Queue_Enqueue(popped_reg_queue, elem);
        // printf("popped! reg: %d\n", popped_reg->reg_index);
        break;
      }

    case LIR_COMMIT:
      {
        // 反映を確定
        commit(f_locals);

        printf("%d : %d\n", f_locals->last_commited_index, f_locals->commited_index);

        // TODO: f_locals_on_exitの書き換えが必要？
        p += sprintf(p, "%s", commit_f_locals(f_locals));

        break;
      }

    case LIR_ENV_STORE:
      {
        Reg* popped_reg = Queue_Dequeue(popped_reg_queue);
        assert(popped_reg != NULL);

        // 環境への格納
        env_t *env = PyMem_Malloc(sizeof(env_t));
        env->reg = popped_reg;
        env->f_env_index = lir_op.oparg.arg;
        append(f_locals, env);

        // TODO: ENV_STORE時に環境を書き換える？ -> いらない気がする

        break;
      }

    case LIR_ENV_LOAD:
      {
        // TODO: f_globals, f_builtins の取り扱いは？
        p += sprintf(p,
          "    PyObject *v%d;\n"
          "    if (PyDict_CheckExact(locals)) {\n"
          "        v%d = PyDict_GetItemWithError(locals, GETITEM(names, %d));\n"
          "    } else {\n"
          "        v%d = PyObject_GetItem(locals, GETITEM(names, %d));\n"
          "    }\n",
          reg_index, reg_index, lir_op.oparg.arg, reg_index, lir_op.oparg.arg);

        prev_reg = reg_index;
        reg_index++;
        break;
      }

    case LIR_GUARD_TYPE_LL:
      {
        Reg* peeked_reg = Queue_Peek(popped_reg_queue);
        p += sprintf(p,
          "    if (!PyLong_Check(v%d)) {\n"
          "        goto L%d;\n"
          "    }\n",
          peeked_reg->reg_index, label_index);
        exit_p += sprintf(exit_p,
          "L%d:\n"
          "    ctx->error = 1;\n"
          "    ctx->exit_on = %d;\n"
          "    *ctx->frame->f_globals = ctx->f_globals_on_exit;\n"
          "    *ctx->frame->f_locals = ctx->f_locals_on_exit;\n"
          ,label_index, label_index);
        label_index++;

        Reg* temp_reg;
        while ((temp_reg = Stack_Peek_And_Move(value_stack)) != NULL)
        {
          exit_p += sprintf(exit_p,
            "    *ctx->stack_pointer++ = v%d;\n",
            temp_reg->reg_index);
        }
        Stack_Peek_Restore(value_stack);

        exit_p += sprintf(exit_p,
            "    return NULL;\n");
        break;
      }

    case LIR_GUARD_TYPE_TRUE:
      {
        Reg* peeked_reg = Queue_Peek(popped_reg_queue);
        p += sprintf(p,
          "    if (!PyBool_Check(v%d) || v%d != Py_True) {\n"
          "        goto L%d;\n"
          "    }\n",
          peeked_reg->reg_index, peeked_reg->reg_index, label_index);
        exit_p += sprintf(exit_p,
          "L%d:\n"
          "    ctx->error = 1;\n"
          "    ctx->exit_on = %d;\n"
          "    *ctx->frame->f_globals = ctx->f_globals_on_exit;\n"
          "    *ctx->frame->f_locals = ctx->f_locals_on_exit;\n"
          , label_index, label_index);
        label_index++;

        Reg* temp_reg;
        while ((temp_reg = Stack_Peek_And_Move(value_stack)) != NULL)
        {
          exit_p += sprintf(exit_p,
            "    *ctx->stack_pointer++ = v%d;\n",
            temp_reg->reg_index);
        }
        Stack_Peek_Restore(value_stack);

        exit_p += sprintf(exit_p,
            "    return NULL;\n");
        break;
      }

    case LIR_GUARD_ADD_OVERFLOW_LL:
      {
        Reg *r2 = Queue_Peek_And_Move(popped_reg_queue);
        Reg *r1 = Queue_Peek_And_Move(popped_reg_queue);

        p += sprintf(p,
          "    if (!jit_check_overflow_add(v%d, v%d)) {\n"
          "        goto L%d;\n"
          "    }\n",
          r1->reg_index, r2->reg_index, label_index);
        Queue_Peek_Restore(popped_reg_queue);

        exit_p += sprintf(exit_p,
          "L%d:\n"
          "    ctx->error = 1;\n"
          "    ctx->exit_on = %d;\n"
          "    *ctx->frame->f_globals = ctx->f_globals_on_exit;\n"
          "    *ctx->frame->f_locals = ctx->f_locals_on_exit;\n"
          , label_index, label_index);
        label_index++;

        Reg* temp_reg;
        while ((temp_reg = Stack_Peek_And_Move(value_stack)) != NULL)
        {
          exit_p += sprintf(exit_p,
            "    *ctx->stack_pointer++ = v%d;\n",
            temp_reg->reg_index);
        }
        Stack_Peek_Restore(value_stack);

        exit_p += sprintf(exit_p,
            "    return NULL;\n");
        break;
      }

    case LIR_ADD_LL:
      {
        Reg *r2 = Queue_Dequeue(popped_reg_queue);
        Reg *r1 = Queue_Dequeue(popped_reg_queue);
        p += sprintf(p,
          "    PyObject *v%d = Py_BuildValue(\"L\", PyLong_AsLongLong(v%d) + PyLong_AsLongLong(v%d));\n",
          reg_index, r1->reg_index, r2->reg_index);
        prev_reg = reg_index;
        reg_index++;
        break;
      }

    case LIR_LT_LL:
      {
        Reg *r2 = Queue_Dequeue(popped_reg_queue);
        Reg *r1 = Queue_Dequeue(popped_reg_queue);
        p += sprintf(p,
          "    PyObject *v%d = Py_BuildValue(\"Oi\", PyLong_AsLongLong(v%d) < PyLong_AsLongLong(v%d) ? Py_True : Py_False);\n",
          reg_index, r1->reg_index, r2->reg_index);
        prev_reg = reg_index;
        reg_index++;
        break;
      }

    case LIR_EXIT:
      {
        p += sprintf(p,
          "    goto L%d;\n"
          , label_index);
        exit_p += sprintf(exit_p,
          "L%d:\n"
          "    ctx->error = 1;\n"
          "    ctx->exit_on = %d;\n"
          "    *ctx->frame->f_globals = ctx->f_globals_on_exit;\n"
          "    *ctx->frame->f_locals = ctx->f_locals_on_exit;\n"
          , label_index, label_index);
        label_index++;

        Reg* temp_reg;
        while ((temp_reg = Stack_Peek_And_Move(value_stack)) != NULL)
        {
          exit_p += sprintf(exit_p,
            "    *ctx->stack_pointer++ = v%d;\n",
            temp_reg->reg_index);
        }
        Stack_Peek_Restore(value_stack);

        exit_p += sprintf(exit_p,
            "    return NULL;\n");

        break;
      }
    }
  }

  p += sprintf(p, "%s", commit_f_locals(f_locals));
  p += sprintf(p,
               "    return v%d;\n",
               prev_reg);

  p += sprintf(p, "%s", exit);
  p += sprintf(p, "}\n");

  free_env(f_locals);

  return code;
}

int jit_compile_trace(trace_t* trace)
{
  // レジスタ割当て
  allocate_lir_register(trace);

  // Cコードの生成
  char* trace_code = generate_c_code(trace);

  printf("generated code:\n");
  printf(trace_code);

  // ソースコードを一時ファイルに書き出し
  FILE* f = fopen("/tmp/trace.c", "w");
  fputs(trace_code, f);
  fclose(f);

  // コンパイルコマンドの生成
  char compile_cmd[1024];
  snprintf(compile_cmd, sizeof(compile_cmd),
        "cc -O2 -fPIC /tmp/trace.c -shared %s "
        "-o /tmp/trace.so", JIT_INCLUDE_PATHS);

  // コンパイル実行
  int result = system(compile_cmd);
  if (result != 0)
  {
    printf("Compilation failed: %s\n", compile_cmd); // デバッグ用
    return -1;
  }

  // 動的ライブラリをロード
  void* handle = dlopen("/tmp/trace.so", RTLD_NOW);
  if (!handle)
  {
    printf("dlopen error: %s\n", dlerror()); // デバッグ用
    return -1;
  }

  // コンパイル済み関数を取得
  jit_compiled_code_t func = dlsym(handle, "trace_func");
  if (!func)
  {
    dlclose(handle);
    return -1;
  }

  // トレースにコンパイル済みコードを設定
  trace->compiled_code = func;
  return 1;
}
