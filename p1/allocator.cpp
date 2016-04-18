#include <cstdlib>
#include <iostream>
#include "allocator.h"


Pointer::Pointer() {
    p = std::shared_ptr<void *>(new void *(nullptr));
    p_size = std::shared_ptr<size_t>(new size_t(0));
}


Pointer::Pointer(void **_p, size_t *_p_size) {
    p = std::shared_ptr<void *>(_p);
    p_size = std::shared_ptr<size_t>(_p_size);
}


Allocator::Allocator(void *base, size_t size) {
    memory = base;
    ocupation = std::vector<bool>(size, false);
}


int Allocator::find_pointer(Pointer &p) {
    int i;

    for (i = 0; i < pointers.size(); ++i)
        if (p.get() == (*pointers[i]).get() and p.getSize() == p.getSize())
            break;
    if (i == pointers.size())
        i = -1;

    return i;
}


Pointer Allocator::alloc(size_t N) {
    int p_begin = 0;
    size_t k = N;

    for (int i = 0; i < ocupation.size(); ++i) {
        if (ocupation[i] == false) {
            p_begin = i;
            k = N;
            for (; i < ocupation.size(); ++i) {
                if (ocupation[i] == false) k--;
                if (ocupation[i] == true or !k) break;
            }
            if (!k) break;
        }
    }
    if (k > 0)
        throw AllocError(AllocErrorType::NoMemory, "No memory\n");

    std::fill(ocupation.begin() + p_begin,
              ocupation.begin() + p_begin + N, true);

    void **p = new (void *)((char *) memory + p_begin);

    Pointer *pointer = new Pointer(p, new size_t(N));
    pointers.push_back(pointer);

    return *pointer;
}


void Allocator::free(Pointer &p) {
    int idx = find_pointer(p);
    if (idx == -1)
        throw AllocError(AllocErrorType::InvalidFree, "Invalid Free\n");

    size_t offset = (char *) p.get() - (char *) memory;

    std::fill(ocupation.begin() + offset,
              ocupation.begin() + offset + p.getSize(), false);

    delete pointers[idx];
    pointers.erase(pointers.begin() + idx);
    p = Pointer(new void *(nullptr), 0);
}


void Allocator::realloc(Pointer &p, size_t N) {
    int idx;
    size_t start;
    int required;

    idx = find_pointer(p);
    if (idx == -1) {
        p = alloc(N);
        return;
    }

    if (idx != -1 and N == p.getSize()) return;

    start = (char *) p.get() - (char *) memory + p.getSize();
    required = (int) (N - p.getSize());

    if (required < 0) {
        std::fill(ocupation.begin() + start + required,
                  ocupation.begin() + start, false);
        p.setSize(N);
        return;
    }

    for (size_t i = start; i < ocupation.size(); ++i) {
        if (ocupation[i] == false) required--;
        if (ocupation[i] == true or !required) break;
    }

    if (!required) {
        std::fill(ocupation.begin() + start,
                  ocupation.begin() + start + N - p.getSize(), true);
        p.setSize(N);
    } else {
        Pointer new_p = alloc(N);
        std::memcpy(new_p.get(), p.get(), p.getSize());
        Allocator::free(p);
        p = new_p;
    }
}


void Allocator::defrag() {
    std::sort(pointers.begin(), pointers.end());

    int curr_pos = 0;
    for (int i = 0; i < pointers.size(); ++i) {
        std::memcpy((char *)memory + curr_pos, pointers[i]->get(), pointers[i]->getSize());
        pointers[i]->set((void *) ((char *) memory + curr_pos));
        curr_pos += pointers[i]->getSize();
    }

    std::fill(ocupation.begin(),
              ocupation.begin() + curr_pos, true);
    std::fill(ocupation.begin() + curr_pos,
              ocupation.end(), false);
}

//int main() {
//    size_t mem_size = 256;
//    int *mem = (int *) malloc(mem_size * sizeof(int));
//
//    Allocator *allocator = new Allocator(mem, mem_size);
//
//    Pointer p1 = allocator->alloc(1);
//    Pointer p2 = allocator->alloc(4);
//    Pointer p3 = allocator->alloc(3);
//    Pointer p4 = allocator->alloc(2);
//    allocator->free(p1);
//    allocator->free(p3);
//    allocator->realloc(p2, 5);
//
//    allocator->defrag();
//
//
//    delete allocator;
//    free(mem);
//
//    return 1;
//}
