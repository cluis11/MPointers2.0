#pragma once
#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "../proto/memory_manager.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using memorymanager::MemoryManager;
using memorymanager::CreateRequest;
using memorymanager::CreateResponse;
using memorymanager::SetRequest;
using memorymanager::SetResponse;
using memorymanager::GetRequest;
using memorymanager::GetResponse;
using memorymanager::IncreaseRefCountRequest;
using memorymanager::IncreaseRefCountResponse;
using memorymanager::DecreaseRefCountRequest;
using memorymanager::DecreaseRefCountResponse;

template<class T>
class MPointer {
private:
    static std::unique_ptr<MemoryManager::Stub> stub_;
    static std::string serverAddress;
    int id; // ID del bloque en el servidor
    
    // Métodos privados de comunicación
    static bool CreateOnServer(size_t size, const std::string& type, int* outId) {
        ClientContext context;
        CreateRequest request;
        CreateResponse response;
        
        request.set_size(size);
        request.set_type(type);
        
        Status status = stub_->Create(&context, request, &response);
        if (status.ok() && response.success()) {
            *outId = response.id();
            return true;
        }
        return false;
    }
    
    bool SetOnServer(const T& value) {
        ClientContext context;
        SetRequest request;
        SetResponse response;
        
        request.set_id(id);
        
        if constexpr (std::is_same_v<T, int>) {
            request.set_int_value(value);
        } else if constexpr (std::is_same_v<T, float>) {
            request.set_float_value(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            request.set_string_value(value);
        } else {
            throw std::runtime_error("Tipo no soportado para Set");
        }
        
        Status status = stub_->Set(&context, request, &response);
        return status.ok() && response.success();
    }
    
    bool GetFromServer(T* outValue) {
        ClientContext context;
        GetRequest request;
        GetResponse response;
        
        request.set_id(id);
        
        Status status = stub_->Get(&context, request, &response);
        if (!status.ok() || !response.success()) {
            return false;
        }
        
        if constexpr (std::is_same_v<T, int>) {
            if (response.has_int_value()) {
                *outValue = response.int_value();
                return true;
            }
        } else if constexpr (std::is_same_v<T, float>) {
            if (response.has_float_value()) {
                *outValue = response.float_value();
                return true;
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (response.has_string_value()) {
                *outValue = response.string_value();
                return true;
            }
        }
        return false;
    }
    
    bool IncreaseRefCount() {
        ClientContext context;
        IncreaseRefCountRequest request;
        IncreaseRefCountResponse response;
        
        request.set_id(id);
        
        Status status = stub_->IncreaseRefCount(&context, request, &response);
        return status.ok() && response.success();
    }
    
    bool DecreaseRefCount() {
        ClientContext context;
        DecreaseRefCountRequest request;
        DecreaseRefCountResponse response;
        
        request.set_id(id);
        
        Status status = stub_->DecreaseRefCount(&context, request, &response);
        return status.ok() && response.success();
    }
    
public:
    // Inicialización estática
    static void Init(const std::string& address) {
        serverAddress = address;
        auto channel = grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
        stub_ = MemoryManager::NewStub(channel);
    }
    
    // Constructor por defecto (nullptr)
    MPointer() : id(-1) {}
    
    // Constructor de movimiento
    MPointer(MPointer&& other) noexcept : id(other.id) {
        other.id = -1;
    }
    
    // Destructor
    ~MPointer() {
        if (id != -1) {
            DecreaseRefCount();
        }
    }
    
    // Método estático para crear nuevos MPointer
    static MPointer<T> New() {
        MPointer<T> ptr;
        if (!CreateOnServer(sizeof(T), typeid(T).name(), &ptr.id)) {
            throw std::runtime_error("Failed to create MPointer on server");
        }
        return ptr;
    }
    
    // Operador de asignación de movimiento
    MPointer& operator=(MPointer&& other) noexcept {
        if (this != &other) {
            if (id != -1) {
                DecreaseRefCount();
            }
            id = other.id;
            other.id = -1;
        }
        return *this;
    }
    
    // Operador de asignación para compartir referencia
    MPointer& operator=(const MPointer& other) {
        if (this != &other) {
            if (id != -1) {
                DecreaseRefCount();
            }
            id = other.id;
            if (id != -1) {
                IncreaseRefCount();
            }
        }
        return *this;
    }
    
    // Operador de desreferenciación
    T operator*() {
        if (id == -1) throw std::runtime_error("Dereferencing null MPointer");
        
        T value;
        if (!GetFromServer(&value)) {
            throw std::runtime_error("Failed to get value from server");
        }
        return value;
    }
    
    // Operador de asignación de valor
    void operator=(const T& value) {
        if (id == -1) throw std::runtime_error("Assigning to null MPointer");
        
        if (!SetOnServer(value)) {
            throw std::runtime_error("Failed to set value on server");
        }
    }
    
    // Operador & para obtener el ID
    int operator&() const {
        return id;
    }
    
    // Operador -> para acceso a miembros
    T* operator->() {
        // Implementación simplificada (en realidad necesitarías cachear)
        return &(**this);
    }
    
    // Conversión a bool para verificar si es válido
    explicit operator bool() const {
        return id != -1;
    }
};

// Inicialización de miembros estáticos
template<class T>
std::unique_ptr<MemoryManager::Stub> MPointer<T>::stub_ = nullptr;

template<class T>
std::string MPointer<T>::serverAddress = "";