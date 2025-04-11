#include "../include/MemoryMap.h"

MemoryMap::MemoryMap(int id, size_t size, std::string type, void* ptr)
    : id(id), size(size), type(type), refCount(0), ptr(ptr), isNew(true) {}

void MemoryMap::print() const {}