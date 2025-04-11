#include "../include/MemoryBlock.h"
#include "../include/GarbageCollector.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iostream>


MemoryBlock::MemoryBlock(size_t sizeMB, const std::string& dumpFolder)
    : memSize(sizeMB * 1024 * 1024), 
      dumps(dumpFolder, memList) {
    memBlock = malloc(memSize);
    if (!memBlock) {
        throw std::runtime_error("Failed to allocate memory block");
    }
    usedMem = 0;
    memAtEnd = memSize;
    
    gc = std::make_unique<GarbageCollector>(*this, dumpFolder);
    gc->Start();
}

MemoryBlock::~MemoryBlock() {
    gc->Stop();
    free(memBlock);
}

void* MemoryBlock::GetLastFreeAddr() {
    Node* lastNode = memList.getLast();
    if (!lastNode) {
        memAtEnd = memSize;
        return memBlock;
    }
    
    char* lastBlockEnd = static_cast<char*>(lastNode->block.ptr) + lastNode->block.size;
    char* totalEnd = static_cast<char*>(memBlock) + memSize;
    memAtEnd = (lastBlockEnd >= totalEnd) ? 0 : totalEnd - lastBlockEnd;
    
    return (memAtEnd > 0) ? lastBlockEnd : nullptr;
}

int MemoryBlock::Create(size_t size, const std::string& type) {
    std::string strCreate = "Create";
    if (usedMem + size > memSize) {
        throw std::runtime_error("Not enough space");
    }
    
    if (size <= memAtEnd) {
        void* addr = GetLastFreeAddr();
        memList.insert(nextId, size, type, addr);
        usedMem += size;
        memAtEnd -= size;
        dumps.CreateDump(strCreate);
        return nextId++;
    }
    
    size_t freeBlockSize = (memSize - usedMem) - memAtEnd;
    
    if (freeBlockSize >= size) {
        int reusedId = memList.reuseFreeBlock(size, type, nextId);
        if (reusedId > -1) {
            nextId++;
            dumps.CreateDump(strCreate);
            return reusedId;
        }
    }
    
    if (freeBlockSize + memAtEnd >= size) {
        CompactMemory();
        if (memAtEnd > memSize - usedMem) {
            memAtEnd = memSize - usedMem;
        }
        void* addr = GetLastFreeAddr();
        memList.insert(nextId, size, type, addr);
        usedMem += size;
        memAtEnd -= size;
        dumps.CreateDump(strCreate);
        return nextId++;
    }
    
    throw std::runtime_error("Failed to allocate memory");
}

void MemoryBlock::CompactMemory() {
    Node* current = memList.getHead();
    Node* prev = nullptr;
    char* currentPos = static_cast<char*>(memBlock);
    
    while (current) {
        if (current->block.type == "") {
            Node* toDelete = current;
            
            if (prev) {
                prev->next = current->next;
            } else {
                memList.updateHead(current->next);
            }
            
            current = current->next;
            delete toDelete;
        } else {
            if (currentPos != current->block.ptr) {
                void* oldPos = current->block.ptr;
                void* newPos = currentPos;
                
                memmove(newPos, oldPos, current->block.size);
                current->block.ptr = newPos;
            }
            
            currentPos = static_cast<char*>(current->block.ptr) + current->block.size;
            prev = current;
            current = current->next;
        }
    }
    
    memAtEnd = memSize - (currentPos - static_cast<char*>(memBlock));
}

void MemoryBlock::Set(int id, const std::string& serialized_data) {
    std::string strSet = "Set";
    MemoryMap* block = memList.findById(id);
    if (!block) {
        throw std::runtime_error("Memory block not found");
    }
    if (serialized_data.size() > block->size) {
        throw std::runtime_error("Data exceeds block size");
    }
    std::memcpy(block->ptr, serialized_data.data(), block->size);
    block->isNew = false;
    dumps.CreateDump(strSet);
}

std::string MemoryBlock::Get(int id) {
    std::string strGet = "Get";
    MemoryMap* block = memList.findById(id);
    if (!block) {
        throw std::runtime_error("Memory block not found");
    }
    dumps.CreateDump(strGet);
    return std::string(static_cast<char*>(block->ptr), block->size);
}

void MemoryBlock::CleanMemoryBlock(int id) {
    std::lock_guard<std::mutex> lock(blockMutex);
    if (auto* block = memList.findById(id)) {
        CleanMemorySpace(block);
    }
}

void MemoryBlock::CleanMemorySpace(MemoryMap* block) {
    if (block->refCount == 0) {
        size_t freedSize = block->size;
        bool wasLastBlock = (memList.getLast() && memList.getLast()->block.id == block->id);  
        
        if (memList.removeById(block->id)) {
            usedMem -= freedSize;
            if (wasLastBlock) { 
                memAtEnd += freedSize;
            }
        }
    }
}



void MemoryBlock::DecreaseRefCount(int id) {
    std::string strRef = "Decrease";
    MemoryMap* block = memList.findById(id);
    if (!block) {
        throw std::runtime_error("Memory block not found");
    }    
    --block->refCount;
    dumps.CreateDump(strRef);
    if (block->refCount == 0) {
        CleanMemorySpace(block);
        dumps.CreateDump(strRef);
    }
}

void MemoryBlock::IncreaseRefCount(int id) {
    std::string strRef = "Increase";
    MemoryMap* block = memList.findById(id);
    if (!block) {
        throw std::runtime_error("Memory block not found");
    }
    ++block->refCount;
    dumps.CreateDump(strRef);
}

MemoryMap* MemoryBlock::GetMemoryMapById(int id) {
    return memList.findById(id);
}