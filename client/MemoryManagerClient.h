#pragma once
#include <grpcpp/grpcpp.h>
#include "../proto/memory_manager.grpc.pb.h"
#include <memory>
#include <string>
#include <cstring>
#include <type_traits>
#include <stdexcept>

// Estructura Node genérica
template <typename T>
struct Node {
    T value;
    int next;  // ID del siguiente nodo
};

class MemoryManagerClient {
public:
    MemoryManagerClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(memorymanager::MemoryManager::NewStub(channel)) {}

    // Operaciones básicas
    int Create(const std::string& type, size_t size) {
        memorymanager::CreateRequest request;
        request.set_type(type);
        request.set_size(size);

        memorymanager::CreateResponse response;
        grpc::ClientContext context;

        if (!stub_->Create(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("Create failed");
        }
        return response.id();
    }

    template <typename T>
    void Set(int id, const T& value) {
        memorymanager::SetRequest request;
        request.set_id(id);
        request.set_data(SerializarExacto(value));  // Serialización interna

        memorymanager::SetResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->Set(&context, request, &response);
        if (!status.ok() || !response.success()) {
            throw std::runtime_error("Set failed");
        }
    }

    template<typename T>
    T Get(int id) {
        memorymanager::GetRequest request;
        request.set_id(id);

        memorymanager::GetResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->Get(&context, request, &response);
        if (!status.ok() || !response.success()) {
            throw std::runtime_error("Get failed");
        }
        return DeserializarExacto<T>(response.data());
    }

    // Operaciones de referencias
    void IncreaseRefCount(int id) {
        memorymanager::IncreaseRefCountRequest request;
        request.set_id(id);

        memorymanager::IncreaseRefCountResponse response;
        grpc::ClientContext context;

        if (!stub_->IncreaseRefCount(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("IncreaseRefCount failed");
        }
    }

    void DecreaseRefCount(int id) {
        memorymanager::DecreaseRefCountRequest request;
        request.set_id(id);

        memorymanager::DecreaseRefCountResponse response;
        grpc::ClientContext context;

        if (!stub_->DecreaseRefCount(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("DecreaseRefCount failed");
        }
    }

private:
    std::unique_ptr<memorymanager::MemoryManager::Stub> stub_;


    // Serialización binaria cruda (siempre del mismo tamaño que el tipo)
    template <typename T>
    std::string SerializarExacto(const T& dato) {
        static_assert(std::is_trivially_copyable_v<T>, 
                    "El tipo debe ser trivialmente copiable");
        
        return std::string(reinterpret_cast<const char*>(&dato), 
                        reinterpret_cast<const char*>(&dato) + sizeof(T));
    }

    // Deserialización binaria cruda
    template <typename T>
    T DeserializarExacto(const std::string& binario) {
        static_assert(std::is_trivially_copyable_v<T>,
                    "El tipo debe ser trivialmente copiable");
        
        T resultado;
        std::memcpy(&resultado, binario.data(), sizeof(T));
        return resultado;
    }
};