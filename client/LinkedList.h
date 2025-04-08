#pragma once
#include "MPointer.h"
#include "Node.h"

template <typename T>
class LinkedList {
private:
    MPointer<Node<T>> head_;

public:
    LinkedList() {
        Node<T> head_node{T(), MPointerID(-1)};  // Construcción explícita
        head_ = MPointer<Node<T>>::NewNode(head_node);  // Pasar objeto ya construido
    }

    void Add(T value) {
        Node<T> new_node{value, MPointerID(-1)};  // Construcción explícita
        MPointer<Node<T>> newNode = MPointer<Node<T>>::NewNode(new_node);

        if (static_cast<int>(head_->next) == -1) {
            head_->next = MPointerID(newNode.GetId());  // Cambiado
        } else {
            MPointer<Node<T>> current = head_;
            while (static_cast<int>(current->next) != -1) {
                current = MPointer<Node<T>>(static_cast<int>(current->next));  // Cambiado
            }
            current->next = MPointerID(newNode.GetId());  // Cambiado
        }
    }

    bool Remove(T value) {
        if (static_cast<int>(head_->next) == -1) return false;

        MPointer<Node<T>> prev = head_;
        MPointer<Node<T>> current = MPointer<Node<T>>(static_cast<int>(prev->next));  // Cambiado

        while (static_cast<int>(current->next) != -1 && current->value != value) {
            prev = current;
            current = MPointer<Node<T>>(static_cast<int>(current->next));  // Cambiado
        }

        if (current->value != value) return false;

        prev->next = MPointerID(static_cast<int>(current->next));  // Cambiado
        return true;
    }

    T Get(int index) {
        MPointer<Node<T>> current = MPointer<Node<T>>(static_cast<int>(head_->next));  // Cambiado
        int i = 0;

        while (static_cast<int>(current->next) != -1 && i < index) {
            current = MPointer<Node<T>>(static_cast<int>(current->next));  // Cambiado
            i++;
        }

        if (i != index) throw std::out_of_range("Índice inválido");
        return current->value;
    }

    int Size() {
        int count = 0;
        MPointer<Node<T>> current = head_;

        while (static_cast<int>(current->next) != -1) {
            current = MPointer<Node<T>>(static_cast<int>(current->next));  // Cambiado
            count++;
        }
        return count;
    }
};