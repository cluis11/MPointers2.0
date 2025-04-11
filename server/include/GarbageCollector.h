#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

class MemoryBlock;

class GarbageCollector {
private:
    MemoryBlock& memoryBlock;
    std::atomic<bool> running;
    std::mutex gcMutex;
    std::thread gcThread;
    std::condition_variable gcCV;
    int collectionIntervalMs;  // Configurable

    void CollectorThread();  // Función del hilo

public:
    GarbageCollector(MemoryBlock& memBlock, 
    const std::string& dumpFolder,
    int intervalMs = 1000);
    ~GarbageCollector();

    void Start();
    void Stop();
    void CollectNow();  // Forzar recolección manual
    void SetInterval(int intervalMs);  // Ajustar intervalo dinámico
};