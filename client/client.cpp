#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
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

class MemoryTestClient {
public:
    MemoryTestClient(std::shared_ptr<Channel> channel)
        : stub_(MemoryManager::NewStub(channel)) {
    }

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
        }
        else {
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

    // NUEVA FUNCIÓN: Obtener valor float de un bloque
    float GetFloatValue(int id) {
        GetRequest request;
        request.set_id(id);

        GetResponse response;
        ClientContext context;

        Status status = stub_->Get(&context, request, &response);

        if (status.ok() && response.success()) {
            if (response.has_float_value()) {
                return response.float_value();
            }
        }
        throw std::runtime_error("Failed to get value for block " + std::to_string(id));
    }

    void RunEnhancedFragmentationTest() {
        const size_t FLOAT_SIZE = sizeof(float);
        const size_t TOTAL_SPACE = 256;
        std::vector<int> all_ids;
        std::vector<int> active_ids;
        size_t block_size = 4 * FLOAT_SIZE;

        // 1. Llenar memoria inicial
        std::cout << "\n=== Fase 1: Llenado inicial ("
            << (TOTAL_SPACE / block_size) << " bloques de "
            << block_size << " bytes) ===" << std::endl;

        for (size_t i = 0; i < TOTAL_SPACE / block_size; i++) {
            int id = Create(block_size, "float");
            if (id != -1) {
                SetFloat(id, 1.0f + i);
                IncreaseRef(id);
                all_ids.push_back(id);
                active_ids.push_back(id);
                std::cout << "  - Creado bloque ID: " << id << " con valor: " << 1.0f + i << std::endl;
            }
        }

        // 2. Liberar bloques alternos
        std::cout << "\n=== Fase 2: Liberar bloques alternos ===" << std::endl;
        for (size_t i = 1; i < all_ids.size(); i += 2) {
            std::cout << "Liberando bloque ID: " << all_ids[i] << std::endl;
            if (DecreaseRef(all_ids[i])) {
                active_ids.erase(std::remove(active_ids.begin(), active_ids.end(), all_ids[i]),
                    active_ids.end());
            }
        }

        // 3. Asignar bloque pequeño
        std::cout << "\n=== Fase 3: Asignar bloque pequeño (4B) ===" << std::endl;
        int small_id = Create(FLOAT_SIZE, "float");
        if (small_id != -1) {
            SetFloat(small_id, 99.9f);
            IncreaseRef(small_id);
            active_ids.push_back(small_id);
            std::cout << "  - Creado bloque pequeño ID: " << small_id << " con valor: 99.9" << std::endl;
        }

        // 4. Intentar bloque grande
        std::cout << "\n=== Fase 4: Intentar bloque grande (32B) ===" << std::endl;
        int large_id = Create(8 * FLOAT_SIZE, "float");
        if (large_id != -1) {
            SetFloat(large_id, 999.9f);
            IncreaseRef(large_id);
            active_ids.push_back(large_id);
            std::cout << "  - ¡Éxito! Bloque grande ID: " << large_id << " con valor: 999.9" << std::endl;
        }
        else {
            std::cout << "  - Fallo esperado (sin compactación)" << std::endl;
        }

        // 5. Verificación final con GET
        VerifyFinalBlocks(active_ids);
    }

private:
    std::unique_ptr<MemoryManager::Stub> stub_;

    void VerifyFinalBlocks(const std::vector<int>& active_ids) {
        std::cout << "\n=== VERIFICACIÓN FINAL CON GET ===" << std::endl;
        std::cout << "Obteniendo valores de los bloques activos:" << std::endl;

        for (int id : active_ids) {
            try {
                float value = GetFloatValue(id);
                std::cout << "  - Bloque ID: " << id << " contiene valor: " << value << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "  - Error en bloque " << id << ": " << e.what() << std::endl;
            }
        }

        // Reporte adicional
        std::cout << "\n=== RESUMEN FINAL ===" << std::endl;
        std::cout << "Total bloques activos: " << active_ids.size() << std::endl;
        std::cout << "IDs activos: ";
        for (int id : active_ids) {
            std::cout << id << " ";
        }
        std::cout << std::endl;
    }
};

int main(int argc, char** argv) {
    MemoryTestClient client(
        grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials())
    );

    std::cout << "=== Test de Fragmentación de Memoria ==="
        << "\nConfiguración:"
        << "\n- Tamaño total: 256 bytes"
        << "\n- Tamaño de float: " << sizeof(float) << " bytes"
        << std::endl;

    client.RunEnhancedFragmentationTest();

    return 0;
}