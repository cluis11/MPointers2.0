#pragma once
#include "MemoryManagerClient.h"
#include "MPointerID.h"
#include <memory>
#include <stdexcept>
#include <mutex>
#include <type_traits>
#include "Node.h" 


// Debug macros para MPointer
#define MP_DEBUG_LOG(msg) std::cout << "[MPointer] " << __FUNCTION__ << "(): " << msg << std::endl
#define MP_DEBUG_LOG_VAR(var) std::cout << "[MPointer] " << __FUNCTION__ << "(): " << #var << " = " << var << std::endl
#define MP_ERROR_LOG(msg) std::cerr << "[MPointer-ERROR] " << __FUNCTION__ << "(): " << msg << std::endl
#define MP_MEMORY_LOG(msg) std::cout << "[MPointer-MEMORY] " << msg << std::endl


template<class T>
class MPointer {
private:
    MPointerID id_;
    mutable T cached_value_;
    mutable Node<T> cached_node_;
    static std::shared_ptr<MemoryManagerClient> client_instance_;
    static std::mutex client_mutex_;

    // Constructor privado
    explicit MPointer(MPointerID id) : id_(id) {
        MP_DEBUG_LOG_VAR(id_);
    }

    class PointerProxy {
        MPointer<T>& parent_;
    public:
        explicit PointerProxy(MPointer<T>& parent) : parent_(parent) {
            MP_DEBUG_LOG("PointerProxy creado");
        }

        void operator=(const T& value) {
            MP_DEBUG_LOG("Asignando valor desde PointerProxy");
            parent_.GetClient()->Set<T>(static_cast<int>(parent_.id_), value);
        }

        operator T() {
            MP_DEBUG_LOG("Convirtiendo PointerProxy a valor");
            MP_DEBUG_LOG_VAR(parent_.id_); // Agregado: para ver qué ID se está usando
            T val = parent_.GetClient()->Get<T>(static_cast<int>(parent_.id_));
            MP_DEBUG_LOG_VAR(val);  // Mostramos el valor ya decodificado
            return val;
        }
    };

    static std::shared_ptr<MemoryManagerClient> GetClient() {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (!client_instance_) {
            MP_DEBUG_LOG("Inicializando instancia del cliente");
            client_instance_ = std::make_shared<MemoryManagerClient>(
                grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials())
            );
        }
        return client_instance_;
    }

    void ValidatePointer() const {
        MP_DEBUG_LOG("Validando puntero");
        if (static_cast<int>(id_) == -1) {
            MP_ERROR_LOG("MPointer no inicializado");
            throw std::runtime_error("MPointer no inicializado");
        }
    }

    void ReleaseCurrent() {
        MP_DEBUG_LOG("Liberando referencia actual");
        if (static_cast<int>(id_) != -1) {
            //GetClient()->DecreaseRefCount(static_cast<int>(id_));
            MP_MEMORY_LOG("Referencia liberada");
            //id_ = MPointerID(-1);
        }
    }

    static std::string TypeName() {
        MP_DEBUG_LOG("Obteniendo nombre de tipo");
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
    MPointer() : id_(-1) {
        MP_DEBUG_LOG("MPointer default constructor");
    }

    explicit MPointer(int id) : id_(id) {
        MP_DEBUG_LOG_VAR(id_);
    }

    // Constructor de copia
    MPointer(const MPointer& other) : id_(other.id_) {
        MP_DEBUG_LOG_VAR(id_);
        if (static_cast<int>(id_) != -1) GetClient()->IncreaseRefCount(static_cast<int>(id_));
    }

    // Move semantics
    MPointer(MPointer&& other) noexcept : id_(other.id_) {
        MP_DEBUG_LOG_VAR(id_);
        other.id_ = MPointerID(-1);
    }

    MPointer& operator=(int new_id) {
        MP_DEBUG_LOG_VAR(new_id);
        ReleaseCurrent();
        id_ = MPointerID(new_id);
        if (new_id != -1) GetClient()->IncreaseRefCount(new_id);
        return *this;
    }

    // Métodos estáticos
    MPointer New() {
        MP_DEBUG_LOG("Creando nuevo MPointer");
        int new_id = GetClient()->Create(TypeName(), sizeof(T));
        GetClient()->IncreaseRefCount(new_id);
        MP_DEBUG_LOG_VAR(new_id);
        return MPointer(MPointerID(new_id));
    }

    MPointer<T> NewNode(const T& initial_val) {
        MP_DEBUG_LOG("Creando nuevo MPointer con valor inicial");
        int new_id = GetClient()->Create(TypeName(), sizeof(T));


        // DEBUG: Verifica el valor ANTES de enviarlo
        std::cout << "Valor a guardar (hex): ";
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&initial_val);
        for (size_t i = 0; i < sizeof(T); ++i) {
            printf("%02x ", bytes[i]);
        }
        std::cout << "\n";

        GetClient()->IncreaseRefCount(new_id);
        GetClient()->Set<T>(new_id, initial_val);
        MP_DEBUG_LOG_VAR(new_id);
        id_ = MPointerID(new_id);
        MP_DEBUG_LOG_VAR(id_);
        return MPointer(MPointerID(new_id));
    }

    // Operadores
    PointerProxy operator*() {
        MP_DEBUG_LOG("Operador * llamado");
        ValidatePointer();
        return PointerProxy(*this);
    }

    T* operator->() {
        MP_DEBUG_LOG("Operador -> llamado con id_ = " << id_);
        ValidatePointer();
        MP_DEBUG_LOG_VAR(id_); // Agregado: para rastrear el ID con el que se llama a Get
        if constexpr (std::is_same_v<T, Node<int>> ||
            std::is_same_v<T, Node<float>> ||
            std::is_same_v<T, Node<std::string>>) {
            cached_node_ = GetClient()->Get<Node<T>>(static_cast<int>(id_));
            MP_DEBUG_LOG_VAR(cached_node_);
            return reinterpret_cast<T*>(&cached_node_);
        }
        cached_value_ = GetClient()->Get<T>(static_cast<int>(id_));
        MP_DEBUG_LOG_VAR(cached_value_); // Mostramos el valor decodificado
        return &cached_value_;
    }

    MPointer& operator=(MPointerID new_id) {
        MP_DEBUG_LOG_VAR(new_id);
        ReleaseCurrent();
        id_ = new_id;
        if (static_cast<int>(id_) != -1) GetClient()->IncreaseRefCount(static_cast<int>(id_));
        return *this;
    }

    MPointer& operator=(const MPointer& other) {
        MP_DEBUG_LOG("Asignación por copia");
        if (this != &other) {
            ReleaseCurrent();
            id_ = other.id_;
            if (static_cast<int>(id_) != -1) GetClient()->IncreaseRefCount(static_cast<int>(id_));
        }
        return *this;
    }

    MPointer& operator=(MPointer&& other) noexcept {
        MP_DEBUG_LOG("Asignación por movimiento");
        if (this != &other) {
            ReleaseCurrent();
            id_ = other.id_;
            other.id_ = MPointerID(-1);
        }
        return *this;
    }

    int GetId() const {
        MP_DEBUG_LOG_VAR(id_);
        return static_cast<int>(id_);
    }

    ~MPointer() {
        MP_DEBUG_LOG("Destructor llamado");
        ReleaseCurrent();
    }
};

// Inicialización de miembros estáticos
template<class T>
std::shared_ptr<MemoryManagerClient> MPointer<T>::client_instance_ = nullptr;

template<class T>
std::mutex MPointer<T>::client_mutex_;