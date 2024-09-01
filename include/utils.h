#pragma once

#include "array_container.h"
#include "binsearch_index.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"

#define CAST_TO_BITMAP(p) static_cast<BitmapContainer<WordType, DataBits>*>(p)
#define CAST_TO_ARRAY(p) static_cast<ArrayContainer<WordType, DataBits>*>(p)
#define CAST_TO_RLE(p) static_cast<RLEContainer<WordType, DataBits>*>(p)
#define CAST_TO_BITMAP_CONST(p) static_cast<const BitmapContainer<WordType, DataBits>*>(p)
#define CAST_TO_ARRAY_CONST(p) static_cast<const ArrayContainer<WordType, DataBits>*>(p)
#define CAST_TO_RLE_CONST(p) static_cast<const RLEContainer<WordType, DataBits>*>(p)
#define CTYPE_PAIR(t1, t2) (static_cast<int>(t1) * 4 + static_cast<int>(t2))
