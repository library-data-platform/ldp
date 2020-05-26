#ifndef ETYMON_MALLOCPTR_H
#define ETYMON_MALLOCPTR_H

namespace etymon {

class malloc_ptr {
public:
    void* ptr;
    malloc_ptr(void* ptr);
    ~malloc_ptr();
};

}

#endif
