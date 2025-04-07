#pragma once
#include "MPointerID.h"  // Nuevo archivo para MPointerID

template <typename T>
struct Node {
    T value;
    MPointerID next;  // Cambiado de int a MPointerID

    Node() : value(), next(-1) {}
    Node(T val, MPointerID nxt) : value(val), next(nxt) {}
};