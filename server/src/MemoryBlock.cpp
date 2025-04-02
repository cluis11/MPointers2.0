#include "MemoryBlock.h"
#include "NodeStructure.h"
#include <iostream>

void* MemoryBlock::GetLastFreeAddr() {
    Node* lastNode = memList.getLast();
    if (!lastNode) {
        memAtEnd = memSize;
        return memBlock;
    }

    // Cálculo seguro (cambios aquí)
    char* lastBlockEnd = static_cast<char*>(lastNode->block.ptr) + lastNode->block.size;
    char* totalEnd = static_cast<char*>(memBlock) + memSize;
    memAtEnd = (lastBlockEnd >= totalEnd) ? 0 : totalEnd - lastBlockEnd;

    return (memAtEnd > 0) ? lastBlockEnd : nullptr;
}

MemoryBlock::MemoryBlock(size_t sizeMB) {
    memSize = 256;
    memBlock = malloc(256);
    if (!memBlock) {
        std::cout << "Error al reservar la memoria";
    }
    usedMem = 0;
    memAtEnd = memSize;
}

MemoryBlock::~MemoryBlock() {
    free(memBlock);
}

int MemoryBlock::Create(std::size_t size, const std::string& type) {
    std::cout << "[MemoryBlock] Creando bloque - Tipo: " << type << ", Tamaño: " << size << std::endl;

    // Validación de memoria total
    if (usedMem + size > memSize) {
        std::cout << "[MemoryBlock] ERROR: No hay espacio suficiente. Usado: "
            << usedMem << "/" << memSize << std::endl;
        throw std::runtime_error("No hay espacio suficiente");
    }

    std::cout << "[MemoryBlock] Verificando espacio contiguo al final. memAtEnd: "
        << memAtEnd << std::endl;

    // Verificar espacio contiguo al final
    if (size <= memAtEnd) {
        std::cout << "[MemoryBlock] Asignando espacio contiguo al final" << std::endl;
        void* addr = GetLastFreeAddr();
        std::cout << "[MemoryBlock] Dirección asignada: " << addr << std::endl;

        memList.insert(nextId, size, type, addr);
        usedMem += size;
        memAtEnd -= size;

        std::cout << "[MemoryBlock] Bloque creado. ID: " << nextId
            << ", Memoria usada: " << usedMem
            << ", Memoria libre al final: " << memAtEnd << std::endl;
        return nextId++;
    }

    size_t freeBlockSize = (memSize - usedMem) - memAtEnd;
    std::cout << "[MemoryBlock] Espacio libre entre bloques: " << freeBlockSize << std::endl;

    // Intentar reutilizar bloques libres
    if (freeBlockSize >= size) {
        std::cout << "[MemoryBlock] Intentando reutilizar bloque libre existente" << std::endl;
        int reusedId = memList.reuseFreeBlock(size, type, nextId);
        if (reusedId > -1) {
            nextId++;
            std::cout << "[MemoryBlock] Bloque reutilizado. ID: " << reusedId
                << ", Memoria usada: " << usedMem << std::endl;
            return reusedId;
        }
    }

    // Intentar compactación si no hay espacio suficiente
    if (freeBlockSize + memAtEnd >= size) {
        std::cout << "[MemoryBlock] Intentando compactar memoria para hacer espacio" << std::endl;
        CompactMemory();
        if (memAtEnd > memSize - usedMem) {  // Línea añadida
            memAtEnd = memSize - usedMem;    // Línea añadida (corrección forzada)
        }
        void* addr = GetLastFreeAddr();
        std::cout << "[MemoryBlock] Dirección asignada post-compactación: " << addr << std::endl;

        memList.insert(nextId, size, type, addr);
        usedMem += size;
        memAtEnd -= size;

        std::cout << "[MemoryBlock] Bloque creado después de compactación. ID: " << nextId
            << ", Memoria usada: " << usedMem
            << ", Memoria libre al final: " << memAtEnd << std::endl;
        return nextId++;
    }

    std::cout << "[MemoryBlock] ERROR: No se pudo asignar memoria después de todos los intentos" << std::endl;
    throw std::runtime_error("No se pudo asignar memoria");
}

void MemoryBlock::CompactMemory() {
    std::cout << "\n[MemoryBlock] INICIANDO COMPACTACIÓN DE MEMORIA" << std::endl;
    std::cout << "[MemoryBlock] Estado antes de compactar:" << std::endl;
    std::cout << " - Memoria total: " << memSize << std::endl;
    std::cout << " - Memoria usada: " << usedMem << std::endl;
    std::cout << " - Memoria libre al final: " << memAtEnd << std::endl;
    memList.printList();

    Node* current = memList.getHead();
    Node* prev = nullptr;
    char* currentPos = static_cast<char*>(memBlock);
    size_t freeSpace = 0;
    size_t movedBlocks = 0;
    size_t freedBlocks = 0;

    std::cout << "[MemoryBlock] Posición inicial: " << (void*)currentPos << std::endl;

    while (current) {
        if (current->block.type == "") {
            // Bloque libre encontrado
            std::cout << "[MemoryBlock] Eliminando bloque libre - ID: "
                << current->block.id << ", Tamaño: " << current->block.size << std::endl;

            Node* toDelete = current;
            freeSpace += current->block.size;
            freedBlocks++;

            if (prev) {
                prev->next = current->next;
            }
            else {
                memList.updateHead(current->next);
            }

            current = current->next;
            delete toDelete;
        }
        else {
            // Bloque ocupado
            if (freeSpace > 0) {
                void* oldPos = current->block.ptr;
                void* newPos = currentPos;

                std::cout << "[MemoryBlock] Moviendo bloque - ID: " << current->block.id
                    << "\n   De: " << oldPos << " a " << newPos
                    << "\n   Tamaño: " << current->block.size << std::endl;

                memmove(newPos, oldPos, current->block.size);
                current->block.ptr = newPos;
                movedBlocks++;
            }

            currentPos = static_cast<char*>(current->block.ptr) + current->block.size;
            prev = current;
            current = current->next;
        }
    }

    memAtEnd = memSize - (currentPos - static_cast<char*>(memBlock));

    std::cout << "\n[MemoryBlock] COMPACTACIÓN COMPLETADA" << std::endl;
    std::cout << " - Bloques movidos: " << movedBlocks << std::endl;
    std::cout << " - Bloques liberados: " << freedBlocks << std::endl;
    std::cout << " - Espacio libre recuperado: " << freeSpace << std::endl;
    std::cout << " - Nueva memoria libre al final: " << memAtEnd << std::endl;
    std::cout << "Estado después de compactar:" << std::endl;
    memList.printList();
}

void MemoryBlock::DecreaseRefCount(int id) {
    std::cout << "\n[MemoryBlock] Reduciendo conteo de referencias para bloque ID: " << id << std::endl;

    // Buscar el bloque de memoria
    MemoryMap* block = memList.findById(id);

    if (!block) {
        std::cerr << "[MemoryBlock] ERROR: Bloque ID " << id << " no encontrado" << std::endl;
        throw std::runtime_error("Bloque de memoria no encontrado");
    }

    std::cout << "[MemoryBlock] Bloque encontrado:" << std::endl;
    block->print();

    // Decrementar el contador
    --block->refCount;
    std::cout << "[MemoryBlock] Nuevo conteo de referencias: " << block->refCount << std::endl;

    // Eliminar el bloque si no hay más referencias
    if (block->refCount == 0) {
        std::cout << "[MemoryBlock] Liberando bloque..." << std::endl;

        // Guardar información antes de liberar
        size_t freedSize = block->size;
        void* freedPtr = block->ptr;
        bool wasLastBlock = (memList.getLast() && memList.getLast()->block.id == id); // Nueva línea

        // Liberar el bloque
        if (memList.removeById(id)) {
            std::cout << "[MemoryBlock] Bloque removido de la lista" << std::endl;

            // Actualizar contadores de memoria (versión mejorada)
            usedMem -= freedSize;

            if (wasLastBlock) {  // Nueva condición
                memAtEnd += freedSize;
                std::cout << "[MemoryBlock] Se liberó el último bloque. MemAtEnd actualizado a: "
                    << memAtEnd << std::endl;
            }

            // Sugerir compactación si hay fragmentación
            if (memAtEnd < freedSize) {
                std::cout << "[MemoryBlock] SUGERENCIA: Ejecutar CompactMemory() "
                    << "para reducir fragmentación" << std::endl;
            }
        }
        else {
            std::cerr << "[MemoryBlock] ERROR: No se pudo remover el bloque" << std::endl;
        }
    }
}

void MemoryBlock::IncreaseRefCount(int id) {
    std::cout << "\n[MemoryBlock] Aumentando conteo de referencias para bloque ID: " << id << std::endl;

    // Buscar el bloque de memoria
    MemoryMap* block = memList.findById(id);

    if (!block) {
        std::cerr << "[MemoryBlock] ERROR: Bloque ID " << id << " no encontrado" << std::endl;
        throw std::runtime_error("Bloque de memoria no encontrado");
    }

    std::cout << "[MemoryBlock] Bloque encontrado - Detalles:" << std::endl;
    std::cout << " - Tipo: " << block->type << std::endl;
    std::cout << " - Tamaño: " << block->size << std::endl;
    std::cout << " - Referencias antes: " << block->refCount << std::endl;

    // Incrementar el contador
    block->refCount++;

    std::cout << "[MemoryBlock] Referencias después: " << block->refCount << std::endl;

    // Mostrar estado actual del bloque
    std::cout << "[MemoryBlock] Estado actual del bloque:" << std::endl;
    block->print();
}

MemoryMap* MemoryBlock::GetMemoryMapById(int id) {
    return memList.findById(id);
}

void MemoryBlock::PrintMemoryList() {
    std::cout << "Contenido de la lista de MemoryBlock:" << std::endl;
    memList.printList();
}