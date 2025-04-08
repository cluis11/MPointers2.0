#include "MemoryDump.h"
#include <iostream>
#include <fstream> 
#include <filesystem> 
#include <iomanip>
#include <chrono>
#include <sstream>


MemoryDump::MemoryDump(std::string folder, MemoryList& list) : dumpFolder(folder), memList(list) { }

void MemoryDump::CreateDump() {
    if (!std::filesystem::exists(dumpFolder)) {
        if (std::filesystem::create_directory(dumpFolder)) {
            std::cout << "Carpeta creada existosamente.\n";
        }
        else {
            std::cerr << "Error al crear la carpeta.\n";
        }
    } else {
        std::cout << "La carpeta ya existe.\n";
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d_%H-%M-%S-")
            << std::setw(3) << std::setfill('0') << ms.count();


        std::string datetime = oss.str();
        std::string fileName = dumpFolder + "/" + datetime + ".txt";

        std::fstream archive;
        archive.open(fileName, std::ios::out);

        if (!archive.is_open()) {
            std::cerr << "Error al abrir el arhcivo del dump: " << fileName << std::endl;
            return;
        }

        Node* current = memList.getHead(); // Obtener la cabeza de la lista

        while (current != nullptr) {
            archive << "ID: " << current->block.id
                << ", Size: " << current->block.size
                << ", Type: " << current->block.type
                << ", RefCount: " << current->block.refCount
                << ", Ptr: " << current->block.ptr;

            // Escribir el valor almacenado en la memoria si es un tipo primitivo
            if (current->block.type == "int") {
                archive << ", Value: " << *static_cast<int*>(current->block.ptr);
            }
            else if (current->block.type == "float") {
                archive << ", Value: " << *static_cast<float*>(current->block.ptr);
            }
            else if (current->block.type == "string") {
                archive << ", Value: " << *static_cast<std::string*>(current->block.ptr);
            }
            else {
                archive << ", Value: [No serializable]";
            }

            archive << std::endl;
            current = current->next;
        }

        archive.close();
        std::cout << "Dump creado: " << fileName << std::endl;

    }
}
