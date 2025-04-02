#pragma once
#include <string>

struct MemoryMap {
	int id;
	std::size_t size;
	std::string type;
	int refCount;
	void* ptr;
	bool isNew;

	//M�todos
	MemoryMap(int id, std::size_t size, std::string type, void* ptr);
	void print() const;
};

struct Node {
	MemoryMap block;
	Node* next;

	//M�todos
	Node(int id, std::size_t size, std::string type, void* ptr);
};
