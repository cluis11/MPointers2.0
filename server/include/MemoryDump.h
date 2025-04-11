#include "MemoryList.h"
#include <chrono>
#include <iostream>

class MemoryDump {
	private:
		std::string dumpFolder;
		MemoryList& memList;

	public:
		MemoryDump(std::string folder, MemoryList& list);
		void CreateDump(std::string name);

};