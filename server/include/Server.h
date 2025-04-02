#pragma once
#include "MemoryManager.h"
#include "MemoryBlock.h"
#include <string>

class MemoryServer {
	public:
		static void Run(const std::string& listenPort, std::size_t memSize);
};