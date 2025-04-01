#pragma once
#include <memory>
#include <string>
#include <typeinfo>
#include <stdexcept>
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
    int id_ = -1;  // -1 indica nullptr

    // Métodos de comunicación con el servidor
    T getValue() const {
        if (id_ == -1) throw std::runtime_error("Dereferencing null MPointer");
        
        ClientContext context;
        GetRequest request;
        GetResponse response;
        request.set_id(id_);
        
        if (!stub_->Get(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("Failed to get value from server");
        }

        if constexpr (std::is_same_v<T, int>) {
            return response.int_value();
        } else if constexpr (std::is_same_v<T, float>) {
            return response.float_value();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return response.string_value();
        }
        throw std::runtime_error("Unsupported type");
    }

    void setValue(const T& value) {
        if (id_ == -1) throw std::runtime_error("Assigning to null MPointer");
        
        ClientContext context;
        SetRequest request;
        SetResponse response;
        request.set_id(id_);

        if constexpr (std::is_same_v<T, int>) {
            request.set_int_value(value);
        } else if constexpr (std::is_same_v<T, float>) {
            request.set_float_value(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            request.set_string_value(value);
        }

        if (!stub_->Set(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("Failed to set value on server");
        }
    }

public:
    // Clase interna Reference para manejar asignaciones
    class Reference {
        MPointer* ptr_;
    public:
        Reference(MPointer* ptr) : ptr_(ptr) {}
        
        // Operador de asignación
        Reference& operator=(const T& value) {
            ptr_->setValue(value);
            return *this;
        }
        
        // Conversión implícita a T
        operator T() const {
            return ptr_->getValue();
        }
    };

    // Resto de la implementación de MPointer...
    // ... (constructores, destructores, métodos de gestión como antes)

    // Operador de desreferencia
    Reference operator*() {
        return Reference(this);
    }

    // Operador flecha
    Reference operator->() {
        return Reference(this);
    }
};

// Definiciones de miembros estáticos
template<class T>
std::unique_ptr<MemoryManager::Stub> MPointer<T>::stub_ = nullptr;

template<class T>
std::string MPointer<T>::serverAddress = "";