#include "../include/GarbageCollector.h"
#include "../include/MemoryBlock.h"
#include "../include/MemoryList.h"
#include <chrono>
#include <iostream>

GarbageCollector::GarbageCollector(MemoryBlock& memBlock, 
    const std::string& dumpFolder,
    int intervalMs)
    : memoryBlock(memBlock),
    running(false),
    collectionIntervalMs(intervalMs) {
}

GarbageCollector::~GarbageCollector() {
    Stop();
}

void GarbageCollector::Start() {
    if (!running) {
        running = true;
        gcThread = std::thread(&GarbageCollector::CollectorThread, this);
    }
}

void GarbageCollector::Stop() {
    if (running) {
        {
            std::lock_guard<std::mutex> lock(gcMutex);
            running = false;
        }
        gcCV.notify_all();
        if (gcThread.joinable()) {
            gcThread.join();
        }
    }
}

void GarbageCollector::CollectorThread() {
    std::unique_lock<std::mutex> lock(gcMutex);
    
    while (running) {

        gcCV.wait_for(lock, 
                     std::chrono::milliseconds(collectionIntervalMs),
                     [this] { return !running; });
        
        if (!running) break;
        CollectNow();
    }
}

void GarbageCollector::CollectNow() {
    std::lock_guard<std::mutex> blockLock(memoryBlock.GetMutex());
    
    auto& memList = memoryBlock.GetMemoryList();
    Node* current = memList.getHead();
    Node* previous = nullptr;

    while (current != nullptr) {

        if (current->block.refCount == 0 && !current->block.isNew) {
            Node* toDelete = current;
            current = current->next;

            std::cout << "[GC] Eliminando bloque con ID " << toDelete->block.id << '\n';
            memoryBlock.CleanMemoryBlock(toDelete->block.id);
        } else {
            previous = current;
            current = current->next;
        }
    }

}

void GarbageCollector::SetInterval(int intervalMs) {
    std::lock_guard<std::mutex> lock(gcMutex);
    collectionIntervalMs = intervalMs;
    gcCV.notify_one();
}
