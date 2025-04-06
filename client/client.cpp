#include <iostream>
#include "MPointer.h"

int main() {
     try {
        // Ejemplo con int
        MPointer<int> entero = MPointer<int>::New();
        *entero = 42;
        std::cout << "Valor entero: " << *entero << std::endl;

        // Ejemplo con float
        MPointer<float> flotante = MPointer<float>::New();
        *flotante = 3.1416f;
        std::cout << "Valor flotante: " << *flotante << std::endl;

        // Ejemplo con operaciones matemáticas
        *entero = *entero * 2;
        *flotante = *flotante / 2;

        std::cout << "Entero modificado: " << *entero << std::endl;
        std::cout << "Flotante modificado: " << *flotante << std::endl;

        // Ejemplo con Node<int>
       /* MPointer<Node<int>> nodo = MPointer<Node<int>>::New();
        *nodo = Node<int>{10, -1};
        std::cout << "Nodo valor: " << (*nodo).value 
                  << ", next: " << (*nodo).next << std::endl;*/

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}