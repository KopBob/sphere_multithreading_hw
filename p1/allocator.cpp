#include "allocator.h"


//////////////////////////////
//    Pointer Class
//////////////////////////////

Pointer::Pointer() {
    ptr = shared_ptr<void *>(new void *(nullptr));
    size = shared_ptr<size_t>(new size_t(0));
}

Pointer::Pointer(void **p, size_t n) {
    ptr = shared_ptr<void *>(p);
    size = shared_ptr<size_t>(new size_t(n));
}

bool Pointer::operator==(const Pointer &p) const {
    return (ptr == p.ptr && size == p.size) ? true : false;
}

bool Pointer::operator<(const Pointer &p) const {
    return *ptr < p.get();
}


//////////////////////////////
//    Allocator Class
//////////////////////////////

Allocator::Allocator(void *base, size_t size) : base_ptr(base), size(size) {
    is_occupied = std::vector<bool>(size, 0);
    pointers = std::vector<Pointer *>();
}

Pointer Allocator::alloc(size_t N) {
    int block_begin = 0;
    int free_blocks = 0;

    bool is_enough_mem = false;

    for (int i = 0; i < size; ++i) {
        if (is_occupied[i]) {
            block_begin = i + 1;
            free_blocks = 0;
        } else {
            ++free_blocks;
        }

        if (free_blocks == N) {
            is_enough_mem = true;
            break;
        }
    }

    if (!is_enough_mem)
        throw AllocError(AllocErrorType::NoMemory, "No memory\n");

    for (int i = 0; i < N; ++i) is_occupied[block_begin + i] = true;

    void **new_ptr = new void *((char *)base_ptr + block_begin);

    Pointer *new_pointer = new Pointer(new_ptr, N);
    pointers.push_back(new_pointer);

    return *new_pointer;
}

void Allocator::realloc(Pointer &p, size_t N) {
    // if empty
    if (p.getSize() == 0) {
        p = alloc(N);
        return;
    }

    // shorter
    int offset = (char *)p.get() - (char *)base_ptr;
    if (N < p.getSize()) {
        for (int i = N; i < p.getSize(); ++i) is_occupied[offset + i] = false;
        p.setSize(N);
        return;
    }

    // bigger
    //// ckeck for shrinkage
    bool is_shrinkable = true;
    for (int i = p.getSize(); i < N; ++i)
        if (is_occupied[offset + i]) {
            is_shrinkable = false;
            break;
        }

    if (is_shrinkable) {
        for (int i = p.getSize(); i < N; ++i) is_occupied[offset + i] = true;
        p.setSize(N);
    } else {
        Pointer new_ptr = alloc(N);
        memcpy(new_ptr.get(), p.get(), p.getSize());
        free(p);
        p = new_ptr;
    }
}

void Allocator::free(Pointer &p) {
    int idx = -1;

    for (int i = 0; i < pointers.size(); ++i)
        if (*pointers[i] == p) idx = i;

    if (idx == -1)
        throw AllocError(AllocErrorType::InvalidFree, "Invalid Free\n");

    int offset = (char *)p.get() - (char *)base_ptr;
    for (int i = 0; i < p.getSize(); ++i) is_occupied[offset + i] = false;

    delete pointers[idx];
    pointers.erase(pointers.begin() + idx);
    p = Pointer(new void *(nullptr), 0);
}

void Allocator::defrag() {
    sort(pointers.begin(), pointers.end());

    char *curr_base = (char *)base_ptr;

    int total_len = 0;
    for (int i = 0; i < pointers.size(); ++i) {
        int curr_size = pointers[i]->getSize();
        
        memmove(curr_base, pointers[i]->get(), curr_size);
        pointers[i]->set(curr_base);

        total_len += curr_size;
        curr_base += curr_size;
    }

    for (int i = 0; i < total_len; ++i) is_occupied[i] = true;
    for (int i = total_len; i < size; ++i) is_occupied[i] = false;
}

Allocator::~Allocator() {
    for (int i = 0; i < pointers.size(); ++i) delete pointers[i];
}