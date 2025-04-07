#include <iostream>
#include "LinkedList.h"

int main() {
    try {
        LinkedList<int> list;

        // Agregar elementos
        list.Add(10);
        list.Add(20);
        list.Add(30);

        // Recorrer lista
        std::cout << "Elemento en posición 1: " << list.Get(1) << std::endl;  // 20

        // Eliminar
        list.Remove(20);
        std::cout << "Tamaño después de eliminar: " << list.Size() << std::endl;  // 2

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}