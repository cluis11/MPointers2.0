// Añadir al inicio del archivo:
#pragma once
#include "MPointerID.h"

template <typename T>
struct Node {
    using value_type = T;  // Para static_assert en MPointer
    T value;
    MPointerID next;

    Node() : value(T()), next(-1) {}
    Node(T val, MPointerID nxt) : value(val), next(nxt) {}
};