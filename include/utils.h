#pragma once

#include "array_container.h"
#include "binsearch_index.h"
#include "bitmap_container.h"
#include "prelude.h"
#include "rle_container.h"

#define CTYPE_PAIR(t1, t2) (static_cast<int>(t1) * 4 + static_cast<int>(t2))
