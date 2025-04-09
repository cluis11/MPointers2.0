// Añadir al inicio del archivo:
#pragma once
#include "MPointerID.h"

#pragma pack(push, 1)
template <typename T>
struct Node {
    using value_type = T;  // Para static_assert en MPointer
    T value;
    MPointerID next;

    Node() : value(T()), next(-1) {}
    Node(T val, MPointerID nxt) : value(val), next(nxt) {}
};
#pragma pack(pop)

#include <ostream>

template <typename T>
std::ostream& operator<<(std::ostream& os, const Node<T>& node) {
    os << "Node { value: " << node.value << ", next: " << node.next.get() << " }";
    return os;
}