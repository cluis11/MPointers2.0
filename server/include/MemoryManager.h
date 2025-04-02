#pragma once
#include <sstream>
#include <string>
#include <cstddef>
#include <cereal/archives/binary.hpp>	
#include <cereal/types/string.hpp>

#include "MemoryBlock.h"
#include "NodeStructure.h"
#include <grpcpp/grpcpp.h>
#include "../proto/memory_manager.grpc.pb.h"


class MemoryManagerServiceImpl final : public memorymanager::MemoryManager::Service {
	public:
		explicit MemoryManagerServiceImpl(MemoryBlock& memoryBlock);
		grpc::Status Create(grpc::ServerContext* context,
			const memorymanager::CreateRequest* request,
			memorymanager::CreateResponse* response) override;
		grpc::Status Set(grpc::ServerContext* context,
			const memorymanager::SetRequest* request,
			memorymanager::SetResponse* response) override;
		grpc::Status Get(grpc::ServerContext* context,
			const memorymanager::GetRequest* request,
			memorymanager::GetResponse* response) override;
		grpc::Status IncreaseRefCount(grpc::ServerContext* context,
			const memorymanager::IncreaseRefCountRequest* request,
			memorymanager::IncreaseRefCountResponse* response) override;
		grpc::Status DecreaseRefCount(grpc::ServerContext* context,
			const memorymanager::DecreaseRefCountRequest* request,
			memorymanager::DecreaseRefCountResponse* response) override;
		grpc::Status CreateNode(grpc::ServerContext* context,
			const memorymanager::CreateNodeRequest* request,
			memorymanager::CreateNodeResponse* response) override;
		grpc::Status GetNode(grpc::ServerContext* context,
			const memorymanager::GetNodeRequest* request,
			memorymanager::GetNodeResponse* response) override;
		grpc::Status UpdateNode(grpc::ServerContext* context,
			const memorymanager::UpdateNodeRequest* request,
			memorymanager::UpdateNodeResponse* response) override;

	private:
		MemoryBlock& memoryBlock_;
		template <typename T>
		std::string serialize(const T& data) {
			std::ostringstream oss;
			{
				cereal::BinaryOutputArchive archive(oss);
				archive(data);
			}
			return oss.str();
		}

		template <typename T>
		T deserialize(const std::string& str) {
			std::istringstream iss(str);
			T data;
			{
				cereal::BinaryInputArchive archive(iss);
				archive(data);
			}
			return data;
		}
};