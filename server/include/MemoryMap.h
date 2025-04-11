#pragma once

#include <string>
#include <cstddef>
#include <cstring>

struct MemoryMap {
    int id;
    size_t size;
    std::string type;
    int refCount;
    void* ptr;
    bool isNew;

    MemoryMap(int id, size_t size, std::string type, void* ptr);
    void print() const;
};