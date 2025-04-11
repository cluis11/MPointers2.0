
#pragma once
#include "MemoryList.h"
#include "MemoryDump.h"
#include <string>
#include <memory>
#include <mutex>

// Declaración anticipada para evitar inclusión circular
class GarbageCollector;

class MemoryBlock {
private:
    void* memBlock;
    size_t memSize;
    size_t usedMem;
    size_t memAtEnd;
    MemoryList memList;
	MemoryDump dumps;
    int nextId = 0;
	std::unique_ptr<GarbageCollector> gc;
    std::mutex blockMutex;

    void* GetLastFreeAddr();
    void CleanMemorySpace(MemoryMap* block);

public:
	MemoryBlock(size_t sizeMB, const std::string& dumpFolder);
    ~MemoryBlock();

    int Create(size_t size, const std::string& type);
    void CompactMemory();
    void Set(int id, const std::string& serialized_data);
    std::string Get(int id);
    void DecreaseRefCount(int id);
    void IncreaseRefCount(int id);
    MemoryMap* GetMemoryMapById(int id);

	void PrintMemoryList();

    // Métodos para el GC
    void CleanMemoryBlock(int id);
    MemoryList& GetMemoryList() { return memList; }
    std::mutex& GetMutex() { return blockMutex; }
};