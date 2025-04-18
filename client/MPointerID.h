#pragma once

class MPointerID {
    int id_;
public:
    explicit MPointerID(int id = -1) : id_(id) {}
    operator int() const { return id_; }  // Conversión implícita
    int get() const { return id_; }       // Acceso explícito
};