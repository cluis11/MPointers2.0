#include "../include/MemoryList.h"
#include <cstring>

Node::Node(int id, size_t size, std::string type, void* ptr)
    : block(id, size, type, ptr), next(nullptr) {}

MemoryList::MemoryList() : head(nullptr), last(nullptr) {}

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

void MemoryList::insert(int id, size_t size, const std::string& type, void* ptr) {
    Node* newNode = new Node(id, size, type, ptr);
    if (!head) {
        head = last = newNode;
    } else {
        last->next = newNode;
        last = newNode;
    }
}

bool MemoryList::insertNextTo(int id, int newId, size_t size, const std::string& type, void* ptr) {
    Node* current = head;
    while (current) {
        if (current->block.id == id) {
            Node* newNode = new Node(newId, size, type, ptr);
            newNode->next = current->next;
            current->next = newNode;
            return true;
        }
        current = current->next;
    }
    return false;
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
            if (current == last) {
                last = prev;
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
    Node* current = head;
    while (current) {
        if (current->block.type == "" && current->block.size >= size) {
            if (current->block.size == size) {
                current->block.type = newType;
                return current->block.id;
            } else {
                void* blockAddr = current->block.ptr;
                size_t remainingSize = current->block.size - size;
                void* remainingAddr = static_cast<char*>(blockAddr) + size;
                
                current->block.size = size;
                current->block.type = newType;
                
                insertNextTo(current->block.id, newId, remainingSize, "", remainingAddr);
                return current->block.id;
            }
        }
        current = current->next;
    }
    return -1;
}