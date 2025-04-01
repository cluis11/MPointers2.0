#include "MPointer.h"
#include <iostream>

// Estructura Node definida directamente en el cliente
struct Node {
    int data;
    int next_id;  // 0 indica nullptr
    
    Node(int d = 0, int next = 0) : data(d), next_id(next) {}
};

void printList(const MPointer<Node>& head) {
    auto current = head;
    while (current != nullptr) {
        std::cout << current->data;
        if (current->next_id != 0) {
            std::cout << " -> ";
            current = MPointer<Node>(current->next_id);
        } else {
            std::cout << " -> nullptr";
            break;
        }
    }
    std::cout << std::endl;
}

int main() {
    // 1. Inicializar
    MPointer<int>::Init("localhost:50051");
    MPointer<Node>::Init("localhost:50051");

    // 2. Prueba con tipos básicos
    {
        MPointer<int> num = MPointer<int>::New(10);
        *num = 15;
        *num = *num + 5;
        std::cout << "Valor int: " << *num << std::endl;
    }

    // 3. Prueba con lista enlazada
    {
        auto n1 = MPointer<Node>::New(Node{10});
        auto n2 = MPointer<Node>::New(Node{20});
        auto n3 = MPointer<Node>::New(Node{30});
        
        n1->next_id = n2.id();
        n2->next_id = n3.id();
        
        std::cout << "Lista original: ";
        printList(n1);
        
        // Modificar valores
        n1->data = 100;
        *n2 = Node{200, n2->next_id};
        
        std::cout << "Lista modificada: ";
        printList(n1);
    }

    std::cout << "=== Pruebas completadas exitosamente ===" << std::endl;
    return 0;
}