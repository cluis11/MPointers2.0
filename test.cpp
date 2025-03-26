#pragma once
#include <iostream>
#include <string>
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

namespace MPointersLibrary {

    class MemoryManagerClient {
    public:
        MemoryManagerClient(std::shared_ptr<Channel> channel)
            : stub_(MemoryManager::NewStub(channel)) {}

        int Create(size_t size, const std::string& type) {
            CreateRequest request;
            request.set_size(size);
            request.set_type(type);

            CreateResponse response;
            ClientContext context;

            Status status = stub_->Create(&context, request, &response);

            if (status.ok() && response.success()) {
                return response.id();
            }
            throw std::runtime_error("Error al crear bloque: " + status.error_message());
        }

        template <typename T>
        void Set(int id, const T& value) {
            SetRequest request;
            request.set_id(id);

            if constexpr (std::is_same_v<T, int>) {
                request.set_int_value(value);
            } else if constexpr (std::is_same_v<T, float>) {
                request.set_float_value(value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                request.set_string_value(value);
            }

            SetResponse response;
            ClientContext context;

            Status status = stub_->Set(&context, request, &response);
            if (!status.ok() || !response.success()) {
                throw std::runtime_error("Error al asignar valor: " + status.error_message());
            }
        }

        template <typename T>
        T Get(int id) {
            GetRequest request;
            request.set_id(id);

            GetResponse response;
            ClientContext context;

            Status status = stub_->Get(&context, request, &response);

            if (status.ok() && response.success()) {
                if constexpr (std::is_same_v<T, int>) {
                    return response.int_value();
                } else if constexpr (std::is_same_v<T, float>) {
                    return response.float_value();
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return response.string_value();
                }
            }
            throw std::runtime_error("Error al obtener valor: " + status.error_message());
        }

        void IncreaseRefCount(int id) {
            IncreaseRefCountRequest request;
            request.set_id(id);

            IncreaseRefCountResponse response;
            ClientContext context;

            Status status = stub_->IncreaseRefCount(&context, request, &response);
            if (!status.ok() || !response.success()) {
                throw std::runtime_error("Error al incrementar referencia: " + status.error_message());
            }
        }

        void DecreaseRefCount(int id) {
            DecreaseRefCountRequest request;
            request.set_id(id);

            DecreaseRefCountResponse response;
            ClientContext context;

            Status status = stub_->DecreaseRefCount(&context, request, &response);
            if (!status.ok() || !response.success()) {
                throw std::runtime_error("Error al decrementar referencia: " + status.error_message());
            }
        }

    private:
        std::unique_ptr<MemoryManager::Stub> stub_;
    };

    template<class T>
    class MPointers {
    private:
        static std::unique_ptr<MemoryManagerClient> client;
        int id = -1;
        bool is_owner = false;

        static size_t GetTypeSize() {
            return sizeof(T);
        }

        static std::string GetTypeName() {
            if constexpr (std::is_same_v<T, int>) {
                return "int";
            } else if constexpr (std::is_same_v<T, float>) {
                return "float";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "string";
            } else {
                static_assert(false, "Tipo no soportado");
            }
        }

        void Acquire(int new_id, bool owner) {
            if (id != -1) Release();
            id = new_id;
            is_owner = owner;
            if (id != -1 && !is_owner) {
                client->IncreaseRefCount(id);
            }
        }

        void Release() {
            if (id != -1) {
                if (is_owner) {
                    client->DecreaseRefCount(id);
                } else {
                    client->DecreaseRefCount(id);
                }
                id = -1;
            }
        }

    public:
        // Inicialización estática del cliente
        static void Init(const std::string& server_address) {
            client = std::make_unique<MemoryManagerClient>(
                grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
        }

        // Constructor por defecto (no posee memoria)
        MPointers() = default;

        // Constructor que crea nuevo bloque de memoria
        static MPointers New() {
            MPointers ptr;
            ptr.id = client->Create(GetTypeSize(), GetTypeName());
            ptr.is_owner = true;
            return ptr;
        }

        // Constructor de movimiento
        MPointers(MPointers&& other) noexcept {
            Acquire(other.id, other.is_owner);
            other.id = -1;
            other.is_owner = false;
        }

        // Operador de asignación de movimiento
        MPointers& operator=(MPointers&& other) noexcept {
            if (this != &other) {
                Release();
                Acquire(other.id, other.is_owner);
                other.id = -1;
                other.is_owner = false;
            }
            return *this;
        }

        // Destructor
        ~MPointers() {
            Release();
        }

        // Operador de desreferencia
        T operator*() const {
            if (id == -1) throw std::runtime_error("MPointer no inicializado");
            return client->Get<T>(id);
        }

        // Operador de asignación de valor
        void operator=(const T& value) {
            if (id == -1) {
                // Si no tiene ID, crea un nuevo bloque
                *this = New();
            }
            client->Set<T>(id, value);
        }

        // Operador de asignación para compartir referencia
        MPointers& operator=(const MPointers& other) {
            if (this != &other && id != other.id) {
                Release();
                Acquire(other.id, false); // No es owner, solo referencia
            }
            return *this;
        }

        // Constructor de copia (comparte referencia)
        MPointers(const MPointers& other) {
            Acquire(other.id, false); // No es owner, solo referencia
        }

        // Métodos adicionales útiles
        bool IsValid() const { return id != -1; }
        int GetId() const { return id; }
    };

    template <class T>
    std::unique_ptr<MemoryManagerClient> MPointers<T>::client = nullptr;
}