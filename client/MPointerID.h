#pragma once

#pragma pack(push, 1)
class MPointerID {
    int id_;
public:
    explicit MPointerID(int id) : id_(id) {}
    operator int() const { return id_; }  // Conversión implícita
    int get() const { return id_; }       // Acceso explícito
};
#pragma pack(pop)