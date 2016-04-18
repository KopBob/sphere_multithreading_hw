#include <stdexcept>
#include <string>
#include <vector>

enum class AllocErrorType {
    InvalidFree,
    NoMemory,
};

class AllocError : std::runtime_error {
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message) :
            runtime_error(message),
            type(_type) { }

    AllocErrorType getType() const { return type; }
};

class Allocator;

class Pointer {
    std::shared_ptr<void *> p;
    std::shared_ptr<size_t> p_size;
public:
    Pointer();

    Pointer(void **p, size_t *p_size);

    void *get() const { return *p; }
    void set(void *_p) {*p = _p;}

    size_t getSize() const { return *p_size; }
    void setSize(size_t size) { *p_size = size; }

    bool operator<(const Pointer &o) const {return *p < o.get();}
};

class Allocator {
    void *memory;
    std::vector<bool> ocupation;
    std::vector<Pointer *> pointers;

    int find_pointer(Pointer &p);

public:
    Allocator(void *base, size_t size);

    Pointer alloc(size_t N);

    void realloc(Pointer &p, size_t N);

    void free(Pointer &p);

    void defrag();

    std::string dump() { return ""; }
};

