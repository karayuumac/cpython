//
// Created by root on 24/10/28.
//

#include "jit_internal.h"
#include "jit_runtime.h"
#include <dlfcn.h>
#include <opcode.h>

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

char* generate_c_code(lir_code_t* lir, trace_t *trace)
{
  static char code[65536];
  char* p = code;

  // ヘッダーとインクルード
  p += sprintf(p,
               "#include \"jit_internal.h\"\n"
               "#include \"jit_runtime.h\"\n"
               "\n"
               "PyObject* trace_func(jit_execution_context_t* ctx) {\n"
               "    PyObject* v0 = NULL;\n"
               );

  // レジスタ変数の宣言
  for (int i = 1; i < lir->reg_count; i++)
  {
    p += sprintf(p, "    PyObject* v%d = NULL;\n", i);
  }
  p += sprintf(p, "\n");

  // LIR命令をランタイム関数呼び出しに変換
  lir_block_t* block = lir->entry;
  while (block)
  {
    p += sprintf(p, "L%d:\n", block->label);

    lir_inst_t* inst = block->first;
    while (inst)
    {
      switch (inst->opcode)
      {
      case LIR_CONST:
        p += sprintf(p,
          "    v%d = PyTuple_GET_ITEM((PyTupleObject *) ctx->frame->f_code->co_consts, %d);\n",
          inst->dest.u.reg_num,
          inst->src1.u.heap_index
        );
        break;

      case LIR_LOAD_NAME:
        p += sprintf(p,
          "    v%d = PyTuple_GET_ITEM((PyTupleObject *) ctx->frame->f_code->co_names, %d);\n",
          inst->dest.u.reg_num,
          inst->src1.u.heap_index
        );
        break;


      case LIR_STORE_NAME:
        p += sprintf(p,
          "    {\n"
          "        PyObject *name = v%d;\n"
          "        PyObject *value = v%d;\n"
          "        PyObject *ns = ctx->frame->f_locals;\n"
          "        int err;\n"
          "        if (ns == NULL) {\n"
          "            goto error;\n"
          "        }\n"
          "        if (PyDict_CheckExact(ns))\n"
          "            err = PyDict_SetItem(ns, name, value);\n"
          "        else\n"
          "            err = PyObject_SetItem(ns, name, value);\n"
          "        Py_DECREF(value);\n"
          "        if (err != 0)\n"
          "            goto error;\n"
          "    }\n",
          inst->src1.u.reg_num,
          inst->src2.u.reg_num
        );

      case LIR_LOAD_STACK:
        assert(insn->src1.kind == OPERAND_STACK);
        p += sprintf(p,
          "    {\n"
          "        v%d = ctx->frame->f_valuestack[%d];\n"
          "        Py_INCREF(v%d);\n"
          "    }\n",
          inst->dest.u.reg_num,
          inst->src1.u.stack_pos,
          inst->dest.u.reg_num
          );

      case LIR_EXIT:
        p += sprintf(p,
          "    ctx->frame->f_lasti = %d;\n"
          "    goto error;\n",
          inst->src1.u.label);

      default:
        break;

        /*

      case LIR_ADD:
        p += sprintf(p,
                     "    v%d = jit_binary_add(v%d, v%d);\n"
                     "    if (v%d == NULL) goto error;\n",
                     insn->dest.u.reg_num,
                     insn->src1.u.reg_num,
                     insn->src2.u.reg_num,
                     insn->dest.u.reg_num);
        break;

      case LIR_GUARD_TYPE:
        p += sprintf(p,
                     "    if (!jit_check_type(v%d, %s)) {\n"
                     "        ctx->error = GUARD_ERROR_TYPE;\n"
                     "        ctx->exit_trace = %p;\n"
                     "        goto side_exit;\n"
                     "    }\n",
                     insn->src1.u.reg_num,
                     insn->src2.u.type->tp_name,
                     insn->guard_exit);
        break;
        */

      // TODO: その他命令に対する命令の処理を追加
      }
      inst = inst->next;
    }
    block = block->next;
  }

  int last_reg = 0;
  if (lir->reg_count != 0)
  {
    last_reg = lir->reg_count - 1;
  }

  p += sprintf(p,
               "    return v%d;\n"
               "error:\n"
               "    ctx->error = 1;\n"
               "    return NULL;\n"
               "side_exit:\n"
               "    return NULL;\n"
               "}\n",
               last_reg);

  return code;
}

int jit_compile_trace(trace_t* trace)
{
  // LIRの生成
  lir_code_t* lir = generate_lir(trace);
  if (!lir) return -1;

  // Cコードの生成
  char* trace_code = generate_c_code(lir, trace);
  if (!trace_code)
  {
    lir_free(lir);
    return -1;
  }

  printf("generated code:\n");
  printf(trace_code);

  // ソースコードを一時ファイルに書き出し
  FILE* f = fopen("/tmp/trace.c", "w");
  if (!f)
  {
    lir_free(lir);
    return -1;
  }
  fputs(trace_code, f);
  fclose(f);

  // コンパイルコマンドの生成
  char compile_cmd[1024];
  snprintf(compile_cmd, sizeof(compile_cmd),
        "cc -O2 -fPIC -shared %s "
        "/tmp/trace.c -o /tmp/trace.so", JIT_INCLUDE_PATHS);

  // コンパイル実行
  int result = system(compile_cmd);
  if (result != 0)
  {
    printf("Compilation failed: %s\n", compile_cmd); // デバッグ用
    lir_free(lir);
    return -1;
  }

  // 動的ライブラリをロード
  void* handle = dlopen("/tmp/trace.so", RTLD_NOW);
  if (!handle)
  {
    printf("dlopen error: %s\n", dlerror()); // デバッグ用
    lir_free(lir);
    return -1;
  }

  // コンパイル済み関数を取得
  jit_compiled_code_t func = dlsym(handle, "trace_func");
  if (!func)
  {
    dlclose(handle);
    lir_free(lir);
    return -1;
  }

  // トレースにコンパイル済みコードを設定
  trace->compiled_code = func;
  lir_free(lir);
  return 1;
}
