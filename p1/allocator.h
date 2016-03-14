#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

enum class AllocErrorType {
    InvalidFree,
    NoMemory,
};

class AllocError : std::runtime_error {
   private:
    AllocErrorType type;

   public:
    AllocError(AllocErrorType _type, std::string message)
        : runtime_error(message), type(_type) {}

    AllocErrorType getType() const { return type; }
};

class Allocator;

class Pointer {
    shared_ptr<void *> ptr;
    shared_ptr<size_t> size;

   public:
    Pointer();
    Pointer(void **ptr, size_t n);

    void *get() const { return *ptr; }
    void set(void *p) { *ptr = p; }

    size_t getSize() const { return *size; }
    void setSize(size_t s) { *size = s; }

    bool operator==(const Pointer &) const;
    bool operator<(const Pointer &p) const;
};

class Allocator {
    void *base_ptr;
    size_t size;
    std::vector<bool> is_occupied;
    std::vector<Pointer *> pointers;

   public:
    Allocator(void *base, size_t size);
    ~Allocator();

    Pointer alloc(size_t N);
    void realloc(Pointer &p, size_t N);
    void free(Pointer &p);

    void defrag();
    std::string dump() { return ""; }
};
