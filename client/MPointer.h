#pragma once
#include "MemoryManagerClient.h"
#include <memory>
#include <stdexcept>
#include <mutex>

template<class T>
class MPointer {

    private:

    // Clase proxy para operator*
    class PointerProxy {
        MPointer<T>& parent_;
    public:
        explicit PointerProxy(MPointer<T>& parent) : parent_(parent) {}
        
        void operator=(const T& value) {
            parent_.GetClient()->Set<T>(parent_.id_, value);
        }
        
        operator T() {
            return parent_.GetClient()->Get<T>(parent_.id_);
        }
    };

    int id_;
    mutable T cached_value_;
    static std::shared_ptr<MemoryManagerClient> client_instance_;
    static std::mutex client_mutex_;

    // Constructor privado
    explicit MPointer(int id) : id_(id) {}


    // Obtiene o crea el cliente singleton
    static std::shared_ptr<MemoryManagerClient> GetClient() {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (!client_instance_) {
            client_instance_ = std::make_shared<MemoryManagerClient>(
                grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials())
            );
        }
        return client_instance_;
    }

    void ValidatePointer() const {
        if (id_ == -1) {
            throw std::runtime_error("MPointer no inicializado");
        }
    }

    void ReleaseCurrent() {
        if (id_ != -1) {
            GetClient()->DecreaseRefCount(id_);
            id_ = -1;
        }
    }

    static std::string TypeName() {
        if constexpr (std::is_same_v<T, int>) return "int";
        else if constexpr (std::is_same_v<T, float>) return "float";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else if constexpr (std::is_same_v<T, Node<int>>) return "Node<int>";
        else if constexpr (std::is_same_v<T, Node<float>>) return "Node<float>";
        else if constexpr (std::is_same_v<T, Node<std::string>>) return "Node<string>";
        else return typeid(T).name();
    }

public:
    // Método New estático (única interfaz pública)
    static MPointer New() {
        int new_id = GetClient()->Create(TypeName(), sizeof(T));
        return MPointer(new_id);
    }

    // Operador * optimizado
    PointerProxy operator*() {
        ValidatePointer();
        return PointerProxy(*this);
    }

    // Operador ->
    T* operator->() {
        ValidatePointer();
        cached_value_ = GetClient()->Get<T>(id_);
        return &cached_value_;
    }

    // Operador = para asignación de valor
    MPointer& operator=(const T& value) {
        ValidatePointer();
        GetClient()->Set<T>(id_, value);
        return *this;
    }

    // Operador = para compartir referencia
    MPointer& operator=(const MPointer& other) {
        if (this != &other) {
            ReleaseCurrent();
            id_ = other.id_;
            if (id_ != -1) {
                GetClient()->IncreaseRefCount(id_);
            }
        }
        return *this;
    }

    // Operador & para obtener ID
    int operator&() const {
        return id_;
    }

    // Move constructor
    MPointer(MPointer&& other) noexcept : id_(other.id_) {
        other.id_ = -1;
    }

    // Move assignment
    MPointer& operator=(MPointer&& other) noexcept {
        if (this != &other) {
            ReleaseCurrent();
            id_ = other.id_;
            other.id_ = -1;
        }
        return *this;
    }

    // Destructor
    ~MPointer() {
        ReleaseCurrent();
    }
};

// Inicialización de miembros estáticos
template<class T>
std::shared_ptr<MemoryManagerClient> MPointer<T>::client_instance_ = nullptr;

template<class T>
std::mutex MPointer<T>::client_mutex_;