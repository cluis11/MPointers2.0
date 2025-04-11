#include "MemoryDump.h"
#include <iostream>
#include <fstream> 
#include <filesystem> 
#include <iomanip>
#include <chrono>
#include <sstream>


MemoryDump::MemoryDump(std::string folder, MemoryList& list) : dumpFolder(folder), memList(list) { }

void MemoryDump::CreateDump(std::string name) {
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

        std::string metodo;
        if (name == "Create") {
            metodo = "[Create]";
        }
        else if (name == "Set") {
            metodo = "[Set]";
        }
        else if (name == "Decrease") {
            metodo = "[DecreaseRefCount]";
        }
        else if (name == "Increase") {
            metodo = "[IncreaseRefCount]";
        }
        else if (name == "Get") {
            metodo = "[Get]";
        }
        else {
            metodo = "[Delete]";
        }
    
        archive << metodo << std::endl;

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
                const char* charData = static_cast<const char*>(current->block.ptr);
                if (charData) {
                    // 1. Encontrar la longitud real del string (hasta el primer null o el tamaño máximo)
                    size_t realLength = 0;
                    while (realLength < current->block.size && charData[realLength] != '\0') {
                        realLength++;
                    }
                    
                    // 2. Crear el string solo con los bytes válidos
                    std::string stringValue(charData, realLength);
                    
                    // 3. Mostrar el valor y metadatos útiles para debug
                    archive << ", Value: \"";
                    
                    // Escapar caracteres especiales para mejor visualización
                    for (char c : stringValue) {
                        if (std::isprint(static_cast<unsigned char>(c))) {
                            archive << c;
                        } else {
                            archive << "\\x" << std::hex << std::setw(2) << std::setfill('0') 
                                   << static_cast<int>(static_cast<unsigned char>(c));
                        }
                    }
                    
                } else {
                    archive << ", Value: [NULL_PTR]";
                }
            }
            else if (current->block.type == "char") {
                archive << ", Value: " << *static_cast<char*>(current->block.ptr);
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
