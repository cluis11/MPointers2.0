#include <iostream>
#include <memory>
#include <vector>
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
using memorymanager::IncreaseRefCountRequest;
using memorymanager::IncreaseRefCountResponse;
using memorymanager::DecreaseRefCountRequest;
using memorymanager::DecreaseRefCountResponse;

class MemoryTestClient {
public:
    MemoryTestClient(std::shared_ptr<Channel> channel) 
        : stub_(MemoryManager::NewStub(channel)) {}

    int Create(size_t size, const std::string& type) {
        CreateRequest request;
        request.set_size(size);
        request.set_type(type);

        CreateResponse response;
        ClientContext context;

        std::cout << "Creating block - Size: " << size << " Type: " << type << std::endl;

        Status status = stub_->Create(&context, request, &response);

        if (status.ok() && response.success()) {
            std::cout << "Created ID: " << response.id() << std::endl;
            return response.id();
        } else {
            std::cerr << "Create failed" << std::endl;
            return -1;
        }
    }

    bool SetFloat(int id, float value) {
        SetRequest request;
        request.set_id(id);
        request.set_float_value(value);

        SetResponse response;
        ClientContext context;

        Status status = stub_->Set(&context, request, &response);

        if (status.ok() && response.success()) {
            std::cout << "Set ID " << id << " = " << value << std::endl;
            return true;
        }
        return false;
    }

    bool IncreaseRef(int id) {
        IncreaseRefCountRequest request;
        request.set_id(id);

        IncreaseRefCountResponse response;
        ClientContext context;

        Status status = stub_->IncreaseRefCount(&context, request, &response);
        return status.ok() && response.success();
    }

    bool DecreaseRef(int id) {
        DecreaseRefCountRequest request;
        request.set_id(id);

        DecreaseRefCountResponse response;
        ClientContext context;

        Status status = stub_->DecreaseRefCount(&context, request, &response);
        return status.ok() && response.success();
    }

    void RunPreciseFragmentationTest() {
        const size_t FLOAT_SIZE = sizeof(float); // 4 bytes
        const size_t TOTAL_SPACE = 256; // Tamaño total controlado (ajustable)
        
        // 1. Llenar memoria con bloques de 4 floats (16 bytes)
        std::vector<int> ids;
        size_t block_size = 4 * FLOAT_SIZE; // 16 bytes
        
        std::cout << "\n=== Filling memory (" << TOTAL_SPACE << " bytes) ===" << std::endl;
        for (size_t i = 0; i < TOTAL_SPACE / block_size; i++) {
            int id = Create(block_size, "float");
            if (id != -1) {
                SetFloat(id, 1.0f + i);
                IncreaseRef(id);
                ids.push_back(id);
            }
        }

        // 2. Liberar bloques alternos para fragmentación
        std::cout << "\n=== Creating fragmentation ===" << std::endl;
        for (size_t i = 1; i < ids.size(); i += 2) {
            std::cout << "Releasing block " << ids[i] << std::endl;
            DecreaseRef(ids[i]);
        }

        // 3. Intentar reutilizar espacio con bloque pequeño (1 float = 4 bytes)
        std::cout << "\n=== Testing small block (4B) ===" << std::endl;
        int small_id = Create(FLOAT_SIZE, "float");
        if (small_id != -1) {
            SetFloat(small_id, 99.9f);
            IncreaseRef(small_id);
        }

        // 4. Intentar bloque grande (8 floats = 32 bytes)
        std::cout << "\n=== Testing large block (32B) ===" << std::endl;
        int large_id = Create(8 * FLOAT_SIZE, "float");
        if (large_id != -1) {
            SetFloat(large_id, 999.9f);
            IncreaseRef(large_id);
        } else {
            std::cout << "Large block failed (expected without defrag)" << std::endl;
        }
    }

private:
    std::unique_ptr<MemoryManager::Stub> stub_;
};

int main(int argc, char** argv) {
    MemoryTestClient client(
        grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials())
    );

    std::cout << "=== Memory Fragmentation Test ===" << std::endl;
    std::cout << "Using controlled malloc size: 256 bytes" << std::endl;
    std::cout << "Float size: " << sizeof(float) << " bytes" << std::endl;

    client.RunPreciseFragmentationTest();

    return 0;
}