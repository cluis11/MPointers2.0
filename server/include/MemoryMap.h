#pragma once
#include <iostream>
#include <string>
#include <mutex>

struct MemoryMap {
	std::mutex blockMutex;
	int id;
	std::size_t size;
	std::string type;
	int refCount;
	void* ptr;
	bool isNew;

	//Métodos
	MemoryMap(int id, std::size_t size, std::string type, void* ptr);
	void print() const;
};

struct Node {
	MemoryMap block;
	Node* next;

	//Métodos
	Node(int id, std::size_t size, std::string type, void* ptr);
};
