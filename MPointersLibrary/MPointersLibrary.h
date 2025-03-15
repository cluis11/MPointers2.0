#pragma once
#include <iostream>
#include <string>
#include <unordered_map>

namespace MPointersLibrary {
	template<class T>
	class MPointers {
	private:
		static int nextId;
		static std::unordered_map<int, T> memorySimulation;
		int id;

	public:
		/*
		* Inicializa la conexion con el servidor Memory Manager
		* @param port, Puerto donde escucha el servidor GRPC
		*/
		static void Init(const std::string& port) {
			std::cout << "Conexion inicializada en el puerto " << port << std::endl;
		}

		/*
		* Crea un nuevo MPointer y lo simula en memoria.
		* @return Instancia de MPointers con ID �nico asignado
		*/
		static MPointers<T> New() {
			MPointers<T> ptr;
			ptr.id = nextId++;
			memorySimulation[ptr.id] = T{};
			std::cout << "Se creo el MPointer con ID " << ptr.id << std::endl;
			return ptr;
		}

		/*
		* Operador de desreferenciacion para obtener el valor almacenado
		* @return Referencia al valor almacenado en memoria simulada.
		*/
		T& operator*() {
			std::cout << "Obteniendo valor de MPointer con ID " << id << std::endl;
			return memorySimulation[id];
		}

		/*
		* Operador de asignacion para establecer un nuevo valor
		* @param newValue Nuev valor a almacenar
		*/
		void operator=(const T& newValue) {
			std::cout << "Asignando nuevo valor a MPointer con ID " << id << std::endl;
			memorySimulation[id] = newValue;
		}

		/*
		* Operador de asignaci�n para compartir la referencia de otro MPointer.
		* @param other MPointer del cual se copiara el ID
		*/
		void operator=(const MPointers<T>& other) {
			if (this->id != other.id) {
				std::cout << "Copiando referencia de MPointer con ID " << other.id << "a este Mpointer." << std::endl;
				this->id = other.id;
			}

		}
	};

	template <class T>
	int MPointers<T>::nextId = 1;

	template <class T>
	std::unordered_map<int, T> MPointers<T>::memorySimulation;

}



