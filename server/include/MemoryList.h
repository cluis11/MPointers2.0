#pragma once
#include "MemoryMap.h"
#include <iostream>
#include <string>
#include <mutex>


class MemoryList {
	private:
		Node* head;
		Node* last;
		std::mutex mtx;

	//Métodos
	public:
		MemoryList();
		~MemoryList();
		Node* getHead() const;
		void updateHead(Node* newHead);
		Node* getLast() const;
		void insert(int id, std::size_t size, const std::string& type, void* ptr);
		bool insertNextTo(int id, int newId, std::size_t size, const std::string& type, void* ptr);
		MemoryMap* findById(int id);
		bool removeById(int id);
		int reuseFreeBlock(std::size_t size, const std::string& newType, int newId);
		void printList();
};