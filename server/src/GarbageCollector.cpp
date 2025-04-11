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
    std::cout << "[GC] Constructor: Intervalo de recolección = " << intervalMs << " ms\n";
}

GarbageCollector::~GarbageCollector() {
    std::cout << "[GC] Destructor llamado, deteniendo GC si está activo...\n";
    Stop();
}

void GarbageCollector::Start() {
    if (!running) {
        std::cout << "[GC] Iniciando hilo del Garbage Collector...\n";
        running = true;
        gcThread = std::thread(&GarbageCollector::CollectorThread, this);
    }
}

void GarbageCollector::Stop() {
    if (running) {
        std::cout << "[GC] Deteniendo Garbage Collector...\n";
        {
            std::lock_guard<std::mutex> lock(gcMutex);
            running = false;
        }
        gcCV.notify_all();
        if (gcThread.joinable()) {
            gcThread.join();
        }
        std::cout << "[GC] Garbage Collector detenido.\n";
    }
}

void GarbageCollector::CollectorThread() {
    std::unique_lock<std::mutex> lock(gcMutex);
    std::cout << "[GC] Hilo de recolección iniciado.\n";
    
    while (running) {
        std::cout << "[GC] Esperando " << collectionIntervalMs << " ms o notificación...\n";

        gcCV.wait_for(lock, 
                     std::chrono::milliseconds(collectionIntervalMs),
                     [this] { return !running; });
        
        if (!running) break;

        std::cout << "[GC] Ejecutando recolección manual...\n";
        CollectNow();
    }

    std::cout << "[GC] Saliendo del hilo de recolección.\n";
}

void GarbageCollector::CollectNow() {
    std::lock_guard<std::mutex> blockLock(memoryBlock.GetMutex());
    std::cout << "[GC] Iniciando recorrido de memoria...\n";
    
    auto& memList = memoryBlock.GetMemoryList();
    Node* current = memList.getHead();
    Node* previous = nullptr;

    while (current != nullptr) {
        std::cout << "[GC] Visitando nodo con ID " << current->block.id 
                  << ", refCount = " << current->block.refCount 
                  << ", isNew = " << current->block.isNew << '\n';

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

    std::cout << "[GC] Recolección completada.\n";
}

void GarbageCollector::SetInterval(int intervalMs) {
    std::lock_guard<std::mutex> lock(gcMutex);
    std::cout << "[GC] Cambiando intervalo de recolección a " << intervalMs << " ms\n";
    collectionIntervalMs = intervalMs;
    gcCV.notify_one();
}
