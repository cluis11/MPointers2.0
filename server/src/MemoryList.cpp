#include "MemoryList.h"
#include <iostream>

MemoryList::MemoryList() : head(nullptr), last(nullptr) { }

MemoryList::~MemoryList() {
    Node* current = head;
    while (current != nullptr) {
        Node* nextNode = current->next;
        delete static_cast<char*>(current->block.ptr);
        delete current;
        current = nextNode;
    }
}

Node* MemoryList::getHead() const { return head; }
void MemoryList::updateHead(Node* newHead) { head = newHead; }
Node* MemoryList::getLast() const { return last; }

void MemoryList::insert(int id, std::size_t size, const std::string& type, void* ptr) {
    Node* newNode = new Node(id, size, type, ptr);
    if (!head) {
        head = last = newNode;
    }
    else {
        last->next = newNode;
        last = newNode;
    }
}

bool MemoryList::insertNextTo(int id, int newId, std::size_t size, const std::string& type, void* ptr) {
    Node* current = head;
    while (current) {
        if (current->block.id == id) {
            // Crear nuevo nodo con el espacio sobrante
            Node* newNode = new Node(newId, size, type, ptr);
            newNode->next = current->next;
            current->next = newNode;
            return true;
        }
        current = current->next;
    }
    return false; // No se encontró el id
}

MemoryMap* MemoryList::findById(int id) {
    for (Node* current = head; current != nullptr; current = current->next) {
        if (current->block.id == id) {
            return &current->block;
        }
    }
    return nullptr;
}

bool MemoryList::removeById(int id) {
    Node* current = head;
    Node* prev = nullptr;
    while (current != nullptr) {
        if (current->block.id == id) {
            if (current == last) {          // Línea añadida
                last = prev;                // Línea añadida
            }
            std::memset(current->block.ptr, 0, current->block.size);
            current->block.isNew = true;
            current->block.type = "";
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

int MemoryList::reuseFreeBlock(size_t size, const std::string& newType, int newId) {
    std::cout << "[MemoryList] Buscando bloque libre para reutilizar. Tamaño requerido: "
        << size << ", Tipo: " << newType << std::endl;

    Node* current = head;
    while (current) {
        if (current->block.type == "" && current->block.size >= size) {
            std::cout << "[MemoryList] Bloque libre encontrado - ID: " << current->block.id
                << ", Tamaño: " << current->block.size << std::endl;

            // Tamaño exacto
            if (current->block.size == size) {
                std::cout << "[MemoryList] Reutilizando bloque completo - ID: "
                    << current->block.id << std::endl;
                current->block.type = newType;
                return current->block.id;
            }
            // Bloque más grande (dividirlo)
            else {
                std::cout << "[MemoryList] Dividiendo bloque - ID original: "
                    << current->block.id << ", Tamaño original: " << current->block.size << std::endl;

                void* blockAddr = current->block.ptr;
                size_t remainingSize = current->block.size - size;
                void* remainingAddr = static_cast<char*>(blockAddr) + size;

                // Actualizar bloque actual
                current->block.size = size;
                current->block.type = newType;

                std::cout << "[MemoryList] Bloque actualizado - Nuevo tamaño: "
                    << current->block.size << ", Nuevo tipo: " << current->block.type << std::endl;
                std::cout << "[MemoryList] Creando nuevo bloque libre - Tamaño: "
                    << remainingSize << ", Dirección: " << remainingAddr << std::endl;

                // Insertar espacio sobrante como nuevo nodo libre
                insertNextTo(current->block.id, newId, remainingSize, "", remainingAddr);
                return current->block.id;
            }
        }
        current = current->next;
    }

    std::cout << "[MemoryList] No se encontraron bloques libres adecuados" << std::endl;
    return -1;
}

void MemoryList::printList() {
    Node* current = head;
    while (current != nullptr) {
        current->block.print();
        current = current->next;
    }
}