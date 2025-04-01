#include "MPointer.h"
#include <iostream>

// Estructura del nodo para la lista enlazada
template<typename T>
struct Node {
    T data;
    MPointer<Node<T>> next;
    
    Node(const T& val) : data(val) {}
};

// Función para imprimir la lista
template<typename T>
void printList(const MPointer<Node<T>>& head) {
    MPointer<Node<T>> current = head;
    while (current) {
        std::cout << (*current).data;
        if ((*current).next) {
            std::cout << " -> ";
        }
        current = (*current).next;
    }
    std::cout << " -> nullptr" << std::endl;
}

// Función para agregar un nodo al final
template<typename T>
void append(MPointer<Node<T>>& head, const T& value) {
    MPointer<Node<T>> newNode = MPointer<Node<T>>::New();
    *newNode = Node<T>(value);
    
    if (!head) {
        head = newNode;
    } else {
        MPointer<Node<T>> current = head;
        while ((*current).next) {
            current = (*current).next;
        }
        (*current).next = newNode;
    }
}

// Función para eliminar un nodo por valor
template<typename T>
void remove(MPointer<Node<T>>& head, const T& value) {
    if (!head) return;
    
    // Caso especial: eliminar el primer nodo
    if ((*head).data == value) {
        head = (*head).next;
        return;
    }
    
    // Buscar el nodo a eliminar
    MPointer<Node<T>> current = head;
    while ((*current).next && (*(*current).next).data != value) {
        current = (*current).next;
    }
    
    if ((*current).next) {
        (*current).next = (*(*current).next).next;
    }
}

int main() {
    // 1. Inicializar MPointer
    MPointer<int>::Init("localhost:50051");
    MPointer<Node<int>>::Init("localhost:50051");
    
    // 2. Crear lista: 10 -> 20 -> 30
    MPointer<Node<int>> head = MPointer<Node<int>>::New();
    *head = Node<int>(10);
    
    append(head, 20);
    append(head, 30);
    
    std::cout << "Lista inicial: ";
    printList(head);
    
    // 3. Modificar un valor
    (*(*head).next).data = 25;  // Cambiar 20 -> 25
    std::cout << "Después de modificar: ";
    printList(head);
    
    // 4. Eliminar un nodo
    remove(head, 25);
    std::cout << "Después de eliminar 25: ";
    printList(head);
    
    // 5. Agregar otro nodo
    append(head, 40);
    std::cout << "Después de agregar 40: ";
    printList(head);
    
    return 0;
}