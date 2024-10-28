//
// Created by karayuu on 24/10/28.
//
#include "jit_lir.h"

/// LIRコードの作成
lir_code_t *lir_create(void)
{
  lir_code_t *lir = (lir_code_t *) PyMem_Malloc(sizeof(lir_code_t));
  if (!lir) return NULL;

  lir->entry = NULL;
  lir->current = NULL;
  lir->reg_count = 0;
  lir->label_count = 0;
  lir->constant = NULL;
  lir->const_count = 0;

  // 入口ブロックを作成
  lir->entry = lir_new_block(lir);
  if (!lir->entry)
  {
    PyMem_Free(lir);
    return NULL;
  }
  lir->current = lir->entry;

  return lir;
}
