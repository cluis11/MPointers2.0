#pragma once 
#include "MemoryList.h"
#include "NodeStructure.h"
#include <string>

class MemoryBlock {
	private:
		void* memBlock;
		std::size_t memSize;
		std::size_t usedMem;
		std::size_t memAtEnd;
		MemoryList memList;
		int nextId = 0;

		//Métodos private
		void* GetLastFreeAddr();

	//Métodos
	public:
		MemoryBlock(std::size_t sizeMB);
		~MemoryBlock();
		int Create(std::size_t size, const std::string& type);
		void CompactMemory();
		template <typename T>
		void Set(int id, const T& value) {
			MemoryMap* block = memList.findById(id);
			if (!block) throw std::runtime_error("Bloque no encontrado");
			if (sizeof(T) > block->size) throw std::runtime_error("Tamaño excedido");
			*static_cast<T*>(block->ptr) = value;
			block->isNew = false;
		}

		template <typename T>
		T Get(int id) {
			MemoryMap* block = memList.findById(id);
			if (!block) throw std::runtime_error("Bloque no encontrado");
			return *static_cast<T*>(block->ptr);
		}

		// Especializaciones inline
		template <>
		inline void Set<NodeStructure>(int id, const NodeStructure& value) {
			Set<NodeStructure>(id, value); // Reutiliza la plantilla general
		}

		template <>
		inline NodeStructure Get<NodeStructure>(int id) {
			MemoryMap* block = memList.findById(id);
			if (!block) throw std::runtime_error("Bloque no encontrado");
			if (sizeof(NodeStructure) > block->size) throw std::runtime_error("Tamaño insuficiente");
			return *static_cast<NodeStructure*>(block->ptr);
		}
		void DecreaseRefCount(int id);
		void IncreaseRefCount(int id);
		MemoryMap* GetMemoryMapById(int id);
		void PrintMemoryList();
};