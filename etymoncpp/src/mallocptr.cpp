#include <stdlib.h>

#include "../include/mallocptr.h"

namespace etymon {

malloc_ptr::malloc_ptr(void* ptr)
{
    this->ptr = ptr;
}

malloc_ptr::~malloc_ptr()
{
    free(ptr);
}

}

