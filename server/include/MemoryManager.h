#pragma once
#include <grpcpp/grpcpp.h>
#include "../proto/memory_manager.grpc.pb.h"
#include "../include/MemoryBlock.h"

class MemoryManagerServiceImpl final : public memorymanager::MemoryManager::Service {
private:
    MemoryBlock& memoryBlock_;

public:
    MemoryManagerServiceImpl(MemoryBlock& memoryBlock);
    
    grpc::Status Create(grpc::ServerContext* context, const memorymanager::CreateRequest* request, 
                       memorymanager::CreateResponse* response) override;
    
    grpc::Status Set(grpc::ServerContext* context, const memorymanager::SetRequest* request,
                    memorymanager::SetResponse* response) override;
    
    grpc::Status Get(grpc::ServerContext* context, const memorymanager::GetRequest* request,
                    memorymanager::GetResponse* response) override;
    
    grpc::Status IncreaseRefCount(grpc::ServerContext* context, const memorymanager::IncreaseRefCountRequest* request,
                                memorymanager::IncreaseRefCountResponse* response) override;
    
    grpc::Status DecreaseRefCount(grpc::ServerContext* context, const memorymanager::DecreaseRefCountRequest* request,
                                memorymanager::DecreaseRefCountResponse* response) override;
};