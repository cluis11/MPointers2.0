#pragma once
#include "MPointer.h"
#include "Node.h"

template <typename T>
class LinkedList {
private:
    MPointer<Node<T>> head_;

public:
    LinkedList() {
        head_ = MPointer<Node<T>>::NewNode({T(), MPointerID(-1)});
    }

    void Add(T value) {
        MPointer<Node<T>> newNode = MPointer<Node<T>>::NewNode({value, MPointerID(-1)});

        if (head_->next.get() == -1) {
            head_->next = newNode.GetId();
        } else {
            MPointer<Node<T>> current = head_;
            while (current->next.get() != -1) {
                current = current->next.get();
            }
            current->next = newNode.GetId();
        }
    }

    bool Remove(T value) {
        if (head_->next.get() == -1) return false;

        MPointer<Node<T>> prev = head_;
        MPointer<Node<T>> current = prev->next.get();

        while (current->next.get() != -1 && current->value != value) {
            prev = current;
            current = current->next.get();
        }

        if (current->value != value) return false;

        prev->next = current->next.get();
        return true;
    }

    T Get(int index) {
        MPointer<Node<T>> current = head_->next.get();
        int i = 0;

        while (current->next.get() != -1 && i < index) {
            current = current->next.get();
            i++;
        }

        if (i != index) throw std::out_of_range("Índice inválido");
        return current->value;
    }

    int Size() {
        int count = 0;
        MPointer<Node<T>> current = head_;

        while (current->next.get() != -1) {
            current = current->next.get();
            count++;
        }
        return count;
    }
};