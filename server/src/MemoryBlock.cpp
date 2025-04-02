#include "MemoryBlock.h"
#include "NodeStructure.h"
#include <iostream>

void* MemoryBlock::GetLastFreeAddr() {
    Node* lastNode = memList.getLast();
    if (!lastNode) {
        memAtEnd = memSize;
        return memBlock;
    }

    // C�lculo seguro (cambios aqu�)
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
    std::cout << "[MemoryBlock] Creando bloque - Tipo: " << type << ", Tama�o: " << size << std::endl;

    // Validaci�n de memoria total
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
        std::cout << "[MemoryBlock] Direcci�n asignada: " << addr << std::endl;

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

    // Intentar compactaci�n si no hay espacio suficiente
    if (freeBlockSize + memAtEnd >= size) {
        std::cout << "[MemoryBlock] Intentando compactar memoria para hacer espacio" << std::endl;
        CompactMemory();
        if (memAtEnd > memSize - usedMem) {  // L�nea a�adida
            memAtEnd = memSize - usedMem;    // L�nea a�adida (correcci�n forzada)
        }
        void* addr = GetLastFreeAddr();
        std::cout << "[MemoryBlock] Direcci�n asignada post-compactaci�n: " << addr << std::endl;

        memList.insert(nextId, size, type, addr);
        usedMem += size;
        memAtEnd -= size;

        std::cout << "[MemoryBlock] Bloque creado despu�s de compactaci�n. ID: " << nextId
            << ", Memoria usada: " << usedMem
            << ", Memoria libre al final: " << memAtEnd << std::endl;
        return nextId++;
    }

    std::cout << "[MemoryBlock] ERROR: No se pudo asignar memoria despu�s de todos los intentos" << std::endl;
    throw std::runtime_error("No se pudo asignar memoria");
}

void MemoryBlock::CompactMemory() {
    std::cout << "\n[MemoryBlock] INICIANDO COMPACTACI�N DE MEMORIA" << std::endl;
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

    std::cout << "[MemoryBlock] Posici�n inicial: " << (void*)currentPos << std::endl;

    while (current) {
        if (current->block.type == "") {
            // Bloque libre encontrado
            std::cout << "[MemoryBlock] Eliminando bloque libre - ID: "
                << current->block.id << ", Tama�o: " << current->block.size << std::endl;

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
                    << "\n   Tama�o: " << current->block.size << std::endl;

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

    std::cout << "\n[MemoryBlock] COMPACTACI�N COMPLETADA" << std::endl;
    std::cout << " - Bloques movidos: " << movedBlocks << std::endl;
    std::cout << " - Bloques liberados: " << freedBlocks << std::endl;
    std::cout << " - Espacio libre recuperado: " << freeSpace << std::endl;
    std::cout << " - Nueva memoria libre al final: " << memAtEnd << std::endl;
    std::cout << "Estado despu�s de compactar:" << std::endl;
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

    // Eliminar el bloque si no hay m�s referencias
    if (block->refCount == 0) {
        std::cout << "[MemoryBlock] Liberando bloque..." << std::endl;

        // Guardar informaci�n antes de liberar
        size_t freedSize = block->size;
        void* freedPtr = block->ptr;
        bool wasLastBlock = (memList.getLast() && memList.getLast()->block.id == id); // Nueva l�nea

        // Liberar el bloque
        if (memList.removeById(id)) {
            std::cout << "[MemoryBlock] Bloque removido de la lista" << std::endl;

            // Actualizar contadores de memoria (versi�n mejorada)
            usedMem -= freedSize;

            if (wasLastBlock) {  // Nueva condici�n
                memAtEnd += freedSize;
                std::cout << "[MemoryBlock] Se liber� el �ltimo bloque. MemAtEnd actualizado a: "
                    << memAtEnd << std::endl;
            }

            // Sugerir compactaci�n si hay fragmentaci�n
            if (memAtEnd < freedSize) {
                std::cout << "[MemoryBlock] SUGERENCIA: Ejecutar CompactMemory() "
                    << "para reducir fragmentaci�n" << std::endl;
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
    std::cout << " - Tama�o: " << block->size << std::endl;
    std::cout << " - Referencias antes: " << block->refCount << std::endl;

    // Incrementar el contador
    block->refCount++;

    std::cout << "[MemoryBlock] Referencias despu�s: " << block->refCount << std::endl;

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