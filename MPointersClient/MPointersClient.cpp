// MPointersClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "MPointersLibrary.h"


int main() {
	using namespace MPointersLibrary;

	//Inicializar conexion (simulada)
	MPointers<int>::Init("1717");

	//Crear un nuevo MPointer
	MPointers<int> myPtr = MPointers<int>::New();

	//Asignar un valor al MPointer
	*myPtr = 5;
	std::cout << "Valor almacenado en myPtr: " << *myPtr << std::endl;

	//Crear otro MPointer
	MPointers <int> myPtr2 = MPointers<int>::New();
	myPtr2 = myPtr;

	std::cout << "Valor almacenado en myPtr2: " << *myPtr2 << std::endl;
	//std::cout << "Id almacenado en myPtr2: " << myPtr2 << std::endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
