#include <iostream>
#include "MemoryBlock.h"
#include <MemoryManager.h>

NodeStructure::NodeStructure(int dataId, int nextId)
    : data_id(dataId), next_id(nextId) {}

MemoryManagerServiceImpl::MemoryManagerServiceImpl(MemoryBlock& memoryBlock) 
    : memoryBlock_(memoryBlock) {}

//Metodo Create
grpc::Status MemoryManagerServiceImpl::Create(
    grpc::ServerContext* context,
    const memorymanager::CreateRequest* request,
    memorymanager::CreateResponse* response) {
    std::cout << "[Servidor] Recibida solicitud Create - Tipo: " << request->type()
        << ", Tamaño: " << request->size() << std::endl;
    try {
        int id = memoryBlock_.Create(request->size(), request->type());
        std::cout << "[Servidor] Bloque creado exitosamente - ID: " << id << std::endl;
        response->set_success(true);
        response->set_id(id);
    }
    catch (const std::exception& e) {
        //cout std::cerr << "[Servidor] Error en Create: " << e.what() << std::endl;
        response->set_success(false);
    }
    return grpc::Status::OK;
}

//Método Set
grpc::Status MemoryManagerServiceImpl::Set(grpc::ServerContext* context,
    const memorymanager::SetRequest* request,
    memorymanager::SetResponse* response) {
    //cout 
    std::cout << "[Servidor] Recibida solicitud Set - ID: " << request->id() << std::endl;
    try {
        if (request->has_int_value()) {
            //cout 
            std::cout << "[Servidor] Asignando valor int: " << request->int_value() << std::endl;
            memoryBlock_.Set<int>(request->id(), request->int_value());
        }
        else if (request->has_float_value()) {
            //cout 
            std::cout << "[Servidor] Asignando valor float: " << request->float_value() << std::endl;
            memoryBlock_.Set<float>(request->id(), request->float_value());
        }
        else if (request->has_string_value()) {
            //cout
            std::cout << "[Servidor] Asignando valor string: " << request->string_value()
                << " (longitud: " << request->string_value().size() << ")" << std::endl;
            memoryBlock_.Set<std::string>(request->id(), request->string_value());
            //cout
            std::cout << "[Servidor] Valor string asignado exitosamente" << std::endl;
        }
        response->set_success(true);
    }
    catch (const std::exception& e) {
        //cout
        std::cerr << "[Servidor] Error en Set: " << e.what() << std::endl;
        response->set_success(false);
    }
    return grpc::Status::OK;
}

//Método Get
grpc::Status MemoryManagerServiceImpl::Get(grpc::ServerContext* context,
    const memorymanager::GetRequest* request,
    memorymanager::GetResponse* response) {
    //cout 
    std::cout << "[Servidor] Recibida solicitud Get - ID: " << request->id() << std::endl;
    try {
        MemoryMap* block = memoryBlock_.GetMemoryMapById(request->id());
        if (!block) {
            //cout
            std::cout << "[Servidor] Bloque no encontrado - ID: " << request->id() << std::endl;
            response->set_success(false);
            return grpc::Status::OK;
        }

        //cout
        std::cout << "[Servidor] Bloque encontrado - Tipo: " << block->type << std::endl;
        response->set_success(true);
        if (block->type == "int") {
            int value = memoryBlock_.Get<int>(request->id());
            std::cout << "[Servidor] Valor int obtenido: " << value << std::endl;
            response->set_int_value(value);
        }
        else if (block->type == "float") {
            float value = memoryBlock_.Get<float>(request->id());
            std::cout << "[Servidor] Valor float obtenido: " << value << std::endl;
            response->set_float_value(value);
        }
        else if (block->type == "string") {
            std::string value = memoryBlock_.Get<std::string>(request->id());
            std::cout << "[Servidor] Valor string obtenido: " << value
                << " (longitud: " << value.size() << ")" << std::endl;
            response->set_string_value(value);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[Servidor] Error en Get: " << e.what() << std::endl;
        response->set_success(false);
    }
    return grpc::Status::OK;
}

//Método IncreaseRefCount
grpc::Status MemoryManagerServiceImpl::IncreaseRefCount(grpc::ServerContext* context,
    const memorymanager::IncreaseRefCountRequest* request,
    memorymanager::IncreaseRefCountResponse* response) {
    std::cout << "[Servidor] Incrementando ref count - ID: " << request->id() << std::endl;
    try {
        memoryBlock_.IncreaseRefCount(request->id());
        response->set_success(true);
    }
    catch (const std::exception& e) {
        std::cerr << "[Servidor] Error en IncreaseRefCount: " << e.what() << std::endl;
        response->set_success(false);
    }
    return grpc::Status::OK;
}

//Método DecreaseRefCount
grpc::Status MemoryManagerServiceImpl::DecreaseRefCount(grpc::ServerContext* context,
    const memorymanager::DecreaseRefCountRequest* request,
    memorymanager::DecreaseRefCountResponse* response) {
    std::cout << "[Servidor] Decrementando ref count - ID: " << request->id() << std::endl;
    try {
        memoryBlock_.DecreaseRefCount(request->id());
        response->set_success(true);
    }
    catch (const std::exception& e) {
        std::cerr << "[Servidor] Error en DecreaseRefCount: " << e.what() << std::endl;
        response->set_success(false);
    }
    return grpc::Status::OK;
}

//Método CreateNode
grpc::Status MemoryManagerServiceImpl::CreateNode(grpc::ServerContext* context,
    const memorymanager::CreateNodeRequest* request,
    memorymanager::CreateNodeResponse* response) {
    try {
        // 1. Crear bloque para los datos del nodo
        int dataId = memoryBlock_.Create(request->initial_data().size(), "node_data");

        // 2. Almacenar datos serializados
        memoryBlock_.Set<std::string>(dataId, request->initial_data());

        // 3. Crear estructura de nodo
        NodeStructure nodeStruct{ dataId, 0 };
        int nodeId = memoryBlock_.Create(sizeof(NodeStructure), "node_structure");
        memoryBlock_.Set<NodeStructure>(nodeId, nodeStruct);

        response->set_success(true);
        response->set_node_id(nodeId);
        response->set_data_id(dataId);
    }
    catch (const std::exception& e) {
        std::cerr << "Error en CreateNode: " << e.what() << std::endl;
        response->set_success(false);
    }
    return grpc::Status::OK;
}

//Método GetNode
grpc::Status MemoryManagerServiceImpl::GetNode(grpc::ServerContext* context,
    const memorymanager::GetNodeRequest* request,
    memorymanager::GetNodeResponse* response) {
    try {
        NodeStructure nodeStruct = memoryBlock_.Get<NodeStructure>(request->node_id());

        response->set_success(true);
        response->set_data_id(nodeStruct.data_id);
        response->set_next_id(nodeStruct.next_id);
    }
    catch (...) {
        response->set_success(false);
    }
    return grpc::Status::OK;
}

//Método UpdateNode
grpc::Status MemoryManagerServiceImpl::UpdateNode(grpc::ServerContext* context,
    const memorymanager::UpdateNodeRequest* request,
    memorymanager::UpdateNodeResponse* response) {
    try {
        // 1. Obtener nodo existente
        NodeStructure nodeStruct = memoryBlock_.Get<NodeStructure>(request->node_id());

        // 2. Actualizar next_id
        nodeStruct.next_id = request->next_id();
        memoryBlock_.Set<NodeStructure>(request->node_id(), nodeStruct);

        // 3. Actualizar datos si se proporcionaron
        if (!request->updated_data().empty()) {
            memoryBlock_.Set<std::string>(nodeStruct.data_id, request->updated_data());
        }

        response->set_success(true);
    }
    catch (...) {
        response->set_success(false);
    }
    return grpc::Status::OK;
}