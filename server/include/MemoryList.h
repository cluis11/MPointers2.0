#pragma once
#include "MemoryMap.h"

struct Node {
    MemoryMap block;
    Node* next;
    Node(int id, size_t size, std::string type, void* ptr);
};

class MemoryList {
private:
    Node* head;
    Node* last;

public:
    MemoryList();
    ~MemoryList();

    Node* getHead() const;
    void updateHead(Node* newHead);
    Node* getLast() const;

    void insert(int id, size_t size, const std::string& type, void* ptr);
    bool insertNextTo(int id, int newId, size_t size, const std::string& type, void* ptr);
    MemoryMap* findById(int id);
    bool removeById(int id);
    int reuseFreeBlock(size_t size, const std::string& newType, int newId);
};