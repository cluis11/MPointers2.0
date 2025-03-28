#include <iostream>
#include <memory>
#include <string>

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

class MemoryManagerClient {
public:
    MemoryManagerClient(std::shared_ptr<Channel> channel)
        : stub_(MemoryManager::NewStub(channel)) {}

    // Método para crear un bloque de memoria
    int Create(int size, const std::string& type) {
        CreateRequest request;
        request.set_size(size);
        request.set_type(type);

        CreateResponse response;
        ClientContext context;

        Status status = stub_->Create(&context, request, &response);

        if (status.ok()) {
            if (response.success()) {
                return response.id();
            } else {
                std::cerr << "Error: No se pudo crear el bloque de memoria." << std::endl;
            }
        } else {
            std::cerr << "Error en la llamada RPC: " << status.error_message() << std::endl;
        }
        return -1; // Indica un error
    }

    // Método para asignar un valor a un bloque de memoria
    bool Set(int id, int value) {
        SetRequest request;
        request.set_id(id);
        request.set_int_value(value);

        SetResponse response;
        ClientContext context;

        Status status = stub_->Set(&context, request, &response);

        if (status.ok()) {
            return response.success();
        } else {
            std::cerr << "Error en la llamada RPC: " << status.error_message() << std::endl;
            return false;
        }
    }

    // Método para obtener el valor de un bloque de memoria
    int Get(int id) {
        GetRequest request;
        request.set_id(id);

        GetResponse response;
        ClientContext context;

        Status status = stub_->Get(&context, request, &response);

        if (status.ok()) {
            if (response.success()) {
                return response.int_value();
            } else {
                std::cerr << "Error: No se pudo obtener el valor del bloque de memoria." << std::endl;
            }
        } else {
            std::cerr << "Error en la llamada RPC: " << status.error_message() << std::endl;
        }
        return -1; // Indica un error
    }

    // Método para incrementar el conteo de referencias
    bool IncreaseRefCount(int id) {
        IncreaseRefCountRequest request;
        request.set_id(id);

        IncreaseRefCountResponse response;
        ClientContext context;

        Status status = stub_->IncreaseRefCount(&context, request, &response);

        if (status.ok()) {
            return response.success();
        } else {
            std::cerr << "Error en la llamada RPC: " << status.error_message() << std::endl;
            return false;
        }
    }

    // Método para decrementar el conteo de referencias
    bool DecreaseRefCount(int id) {
        DecreaseRefCountRequest request;
        request.set_id(id);

        DecreaseRefCountResponse response;
        ClientContext context;

        Status status = stub_->DecreaseRefCount(&context, request, &response);

        if (status.ok()) {
            return response.success();
        } else {
            std::cerr << "Error en la llamada RPC: " << status.error_message() << std::endl;
            return false;
        }
    }

private:
    std::unique_ptr<MemoryManager::Stub> stub_;
};

int main() {
    // Dirección del servidor
    std::string server_address("localhost:50051");

    // Crea el cliente
    MemoryManagerClient client(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials())
    );

    // Prueba el método Create
    int block_id = client.Create(4, "int");
    if (block_id != -1) {
        std::cout << "Bloque de memoria creado con ID: " << block_id << std::endl;

        // Prueba el método Set
        if (client.Set(block_id, 42)) {
            std::cout << "Valor asignado correctamente." << std::endl;

            // Prueba el método Get
            int value = client.Get(block_id);
            if (value != -1) {
                std::cout << "Valor obtenido: " << value << std::endl;
            }
            // Prueba el método IncreaseRefCount
            if (client.IncreaseRefCount(block_id)) {
                std::cout << "Conteo de referencias incrementado." << std::endl;
            }
            if (client.Set(block_id, 12)) {
                std::cout << "Valor asignado correctamente." << std::endl;
            }
        }
    }

    int block_id2 = client.Create(4, "int");
    if (block_id2 != -1) {
        std::cout << "Bloque de memoria creado con ID: " << block_id2 << std::endl;

        // Prueba el método Set
        if (client.Set(block_id2, 5)) {
            std::cout << "Valor asignado correctamente." << std::endl;

            // Prueba el método Get
            int value = client.Get(block_id2);
            if (value != -1) {
                std::cout << "Valor obtenido: " << value << std::endl;
            }
            
            //Prueba metodo DecreaseRefCount
            if (client.DecreaseRefCount(block_id2)) {
                std::cout << "Conteo de referencias decrementado." << std::endl;
            }
        }
    }

    //int block_id3 = client.Create(4, "int");
    //if (block_id3 != -1) {
    //    std::cout << "Bloque de memoria creado con ID: " << block_id3 << std::endl;

    //    // Prueba el método Set
    //    if (client.Set(block_id3, 200)) {
    //        std::cout << "Valor asignado correctamente." << std::endl;

    //        // Prueba el método Get
    //        int value = client.Get(block_id3);
    //        if (value != -1) {
    //            std::cout << "Valor obtenido: " << value << std::endl;
    //        }
    //        // Prueba el método IncreaseRefCount
    //        if (client.IncreaseRefCount(block_id3)) {
    //            std::cout << "Conteo de referencias incrementado." << std::endl;
    //        }
    //    }
    //}
    return 0;
}