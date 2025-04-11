#include "MemoryManager.h"

MemoryManagerServiceImpl::MemoryManagerServiceImpl(MemoryBlock& memoryBlock) 
    : memoryBlock_(memoryBlock) {}

grpc::Status MemoryManagerServiceImpl::Create(grpc::ServerContext* context, const memorymanager::CreateRequest* request, 
                                            memorymanager::CreateResponse* response) {
    try {
        int id = memoryBlock_.Create(request->size(), request->type());
        response->set_success(true);
        response->set_id(id);
    } catch (const std::exception& e) {
        response->set_success(false);
    }
    return grpc::Status::OK;
}

grpc::Status MemoryManagerServiceImpl::Set(grpc::ServerContext* context, const memorymanager::SetRequest* request,
                                         memorymanager::SetResponse* response) {
    try {
        memoryBlock_.Set(request->id(), request->data());
        response->set_success(true);
    } catch (const std::exception& e) {
        response->set_success(false);
    }
    return grpc::Status::OK;
}

grpc::Status MemoryManagerServiceImpl::Get(grpc::ServerContext* context, const memorymanager::GetRequest* request,
                                         memorymanager::GetResponse* response) {
    try {
        MemoryMap* block = memoryBlock_.GetMemoryMapById(request->id());
        if (!block) {
            response->set_success(false);
            return grpc::Status::OK;
        }

        std::string binary_data = memoryBlock_.Get(request->id());
        response->set_success(true);
        response->set_data(binary_data);
    } catch (const std::exception& e) {
        response->set_success(false);
    }
    return grpc::Status::OK;
}

grpc::Status MemoryManagerServiceImpl::IncreaseRefCount(grpc::ServerContext* context, 
                                                      const memorymanager::IncreaseRefCountRequest* request,
                                                      memorymanager::IncreaseRefCountResponse* response) {
    try {
        memoryBlock_.IncreaseRefCount(request->id());
        response->set_success(true);
    } catch (const std::exception& e) {
        response->set_success(false);
    }
    return grpc::Status::OK;
}

grpc::Status MemoryManagerServiceImpl::DecreaseRefCount(grpc::ServerContext* context, 
                                                      const memorymanager::DecreaseRefCountRequest* request,
                                                      memorymanager::DecreaseRefCountResponse* response) {
    try {
        memoryBlock_.DecreaseRefCount(request->id());
        response->set_success(true);
    } catch (const std::exception& e) {
        response->set_success(false);
    }
    return grpc::Status::OK;
}