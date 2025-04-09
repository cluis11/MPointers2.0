#pragma once
#include "MPointer.h"
#include "Node.h"

// Debug macros para LinkedList (podés moverlas a un header común si ya están definidas)
#define MP_DEBUG_LOG(msg) std::cout << "[MPointer] " << __FUNCTION__ << "(): " << msg << std::endl
#define MP_DEBUG_LOG_VAR(var) std::cout << "[MPointer] " << __FUNCTION__ << "(): " << #var << " = " << var << std::endl
#define MP_ERROR_LOG(msg) std::cerr << "[MPointer-ERROR] " << __FUNCTION__ << "(): " << msg << std::endl
#define MP_MEMORY_LOG(msg) std::cout << "[MPointer-MEMORY] " << msg << std::endl

template <typename T>
class LinkedList {
private:
    MPointer<Node<T>> head_;

public:
    LinkedList() {
        Node<T> head_node{ T(), MPointerID(-1) };  // Construcción explícita
        head_ = MPointer<Node<T>>::NewNode(head_node);  // Pasar objeto ya construido
        MP_DEBUG_LOG("LinkedList creada con nodo cabeza vacío");
    }

    void Add(T value) {
        MP_DEBUG_LOG_VAR(value);
        Node<T> new_node{ value, MPointerID(-1) };
        MP_DEBUG_LOG("[LinkedList] Nodo creado con valor inicial");

        MP_DEBUG_LOG_VAR(new_node.value);
        MP_DEBUG_LOG_VAR(static_cast<int>(new_node.next));

        MPointer<Node<T>> newNode = MPointer<Node<T>>::NewNode(new_node);
        MP_DEBUG_LOG("MPointer<Node> creado para nuevo nodo");

        if (static_cast<int>(head_->next) == -1) {
            head_->next = MPointerID(newNode.GetId());
            MP_DEBUG_LOG("Nuevo nodo asignado como primer elemento después del head");
        }
        else {
            MPointer<Node<T>> current = head_;
            while (static_cast<int>(current->next) != -1) {
                current = MPointer<Node<T>>(static_cast<int>(current->next));
                MP_DEBUG_LOG("Avanzando al siguiente nodo en la lista");
            }
            current->next = MPointerID(newNode.GetId());
            MP_DEBUG_LOG("Nuevo nodo agregado al final de la lista");
        }
    }

    bool Remove(T value) {
        MP_DEBUG_LOG_VAR(value);
        if (static_cast<int>(head_->next) == -1) {
            MP_DEBUG_LOG("Lista vacía, no se puede remover");
            return false;
        }

        MPointer<Node<T>> prev = head_;
        MPointer<Node<T>> current = MPointer<Node<T>>(static_cast<int>(prev->next));

        while (static_cast<int>(current->next) != -1 && current->value != value) {
            prev = current;
            current = MPointer<Node<T>>(static_cast<int>(current->next));
            MP_DEBUG_LOG("Avanzando al siguiente nodo en búsqueda de valor");
        }

        if (current->value != value) {
            MP_DEBUG_LOG("Valor no encontrado en la lista");
            return false;
        }

        prev->next = MPointerID(static_cast<int>(current->next));
        MP_DEBUG_LOG("Nodo con valor eliminado de la lista");
        return true;
    }

    T Get(int index) {
        MP_DEBUG_LOG_VAR(index);
        MPointer<Node<T>> current = MPointer<Node<T>>(static_cast<int>(head_->next));
        int i = 0;

        while (static_cast<int>(current->next) != -1 && i < index) {
            current = MPointer<Node<T>>(static_cast<int>(current->next));
            i++;
            MP_DEBUG_LOG("Avanzando en la lista para llegar al índice");
        }

        if (i != index) {
            MP_ERROR_LOG("Índice fuera de rango");
            throw std::out_of_range("Índice inválido");
        }

        MP_DEBUG_LOG_VAR(current->value);
        return current->value;
    }

    int Size() {
        int count = 0;
        MPointer<Node<T>> current = head_;
        while (static_cast<int>(current->next) != -1) {
            current = MPointer<Node<T>>(static_cast<int>(current->next));
            count++;
        }
        MP_DEBUG_LOG_VAR(count);
        return count;
    }
};