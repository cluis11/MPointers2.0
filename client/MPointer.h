#pragma once
#include "MemoryManagerClient.h"
#include "MPointerID.h"
#include <memory>
#include <stdexcept>
#include <mutex>
#include <type_traits>
#include "Node.h" 

template<class T>
class MPointer {
private:
    MPointerID id_;
    mutable T cached_value_;
    mutable Node<T> cached_node_;
    static std::shared_ptr<MemoryManagerClient> client_instance_;
    static std::mutex client_mutex_;

    // Constructor privado
    explicit MPointer(MPointerID id) : id_(id) {}

    class PointerProxy {
        MPointer<T>& parent_;
    public:
        explicit PointerProxy(MPointer<T>& parent) : parent_(parent) {}
        
        void operator=(const T& value) {
            parent_.GetClient()->Set<T>(static_cast<int>(parent_.id_), value);
        }
        
        operator T() {
            return parent_.GetClient()->Get<T>(static_cast<int>(parent_.id_));
        }
    };

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
        if (static_cast<int>(id_) == -1) {
            throw std::runtime_error("MPointer no inicializado");
        }
    }

    void ReleaseCurrent() {
        if (static_cast<int>(id_) != -1) {
            GetClient()->DecreaseRefCount(static_cast<int>(id_));
            id_ = MPointerID(-1);
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
    // Constructores
    MPointer() : id_(-1) {}
    explicit MPointer(int id) : id_(id) {}  // Mantenemos para compatibilidad

    // Constructor de copia
    MPointer(const MPointer& other) : id_(other.id_) {
        if (static_cast<int>(id_) != -1) GetClient()->IncreaseRefCount(static_cast<int>(id_));
    }

    // Move semantics
    MPointer(MPointer&& other) noexcept : id_(other.id_) {
        other.id_ = MPointerID(-1);
    }

    MPointer& operator=(int new_id) {
        ReleaseCurrent();
        id_ = MPointerID(new_id);
        if (new_id != -1) GetClient()->IncreaseRefCount(new_id);
        return *this;
    }

    // Métodos estáticos
    static MPointer New() {
        int new_id = GetClient()->Create(TypeName(), sizeof(T));
        return MPointer(MPointerID(new_id));
    }

    static MPointer NewNode(const T& initial_val) {
        MPointer ptr = New();
        *ptr = initial_val;  // Asignación directa
        return ptr;
    }

    // Operadores
    PointerProxy operator*() {
        ValidatePointer();
        return PointerProxy(*this);
    }

    T* operator->() {
        ValidatePointer();
        if constexpr (std::is_same_v<T, Node<int>> || 
                     std::is_same_v<T, Node<float>> || 
                     std::is_same_v<T, Node<std::string>>) {
            cached_node_ = GetClient()->Get<Node<T>>(static_cast<int>(id_));
            return reinterpret_cast<T*>(&cached_node_);
        }
        cached_value_ = GetClient()->Get<T>(static_cast<int>(id_));
        return &cached_value_;
    }

    MPointer& operator=(MPointerID new_id) {
        ReleaseCurrent();
        id_ = new_id;
        if (static_cast<int>(id_) != -1) GetClient()->IncreaseRefCount(static_cast<int>(id_));
        return *this;
    }

    MPointer& operator=(const MPointer& other) {
        if (this != &other) {
            ReleaseCurrent();
            id_ = other.id_;
            if (static_cast<int>(id_) != -1) GetClient()->IncreaseRefCount(static_cast<int>(id_));
        }
        return *this;
    }

    MPointer& operator=(MPointer&& other) noexcept {
        if (this != &other) {
            ReleaseCurrent();
            id_ = other.id_;
            other.id_ = MPointerID(-1);
        }
        return *this;
    }

    int GetId() const { return static_cast<int>(id_); }

    ~MPointer() {
        ReleaseCurrent();
    }
};

// Inicialización de miembros estáticos
template<class T>
std::shared_ptr<MemoryManagerClient> MPointer<T>::client_instance_ = nullptr;

template<class T>
std::mutex MPointer<T>::client_mutex_;