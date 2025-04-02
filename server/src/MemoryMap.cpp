#include "MemoryMap.h"
#include <ostream>
#include <iostream>

MemoryMap::MemoryMap(int id, std::size_t size, std::string type, void* ptr)
	: id(id), size(size), type(type), refCount(0), ptr(ptr), isNew(true) { }

void MemoryMap::print() const {
    std::cout << "ID: " << id
        << ", Size: " << size
        << ", Type: " << type
        << ", RefCount: " << refCount
        << ", Ptr: " << ptr << std::endl;
}

Node::Node(int id, std::size_t size, std::string type, void* ptr) 
    : block(id, size, type, ptr), next(nullptr) { }