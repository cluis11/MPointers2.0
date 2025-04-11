#include "include/MemoryManager.h"
#include <grpcpp/grpcpp.h>
#include <string>

void RunServer(const std::string& listenPort, size_t memSize, std::string folder) {
    MemoryBlock memoryBlock(memSize, folder);
    MemoryManagerServiceImpl service(memoryBlock);

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:" + listenPort, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
}

int main(int argc, char** argv) {
    if (argc != 7) {
        return 1;
    }

    std::string listenPort;
    size_t memSize = 0;
    std::string dumpFolder;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            listenPort = argv[++i];
        } else if (arg == "--memsize" && i + 1 < argc) {
            memSize = std::stoul(argv[++i]);
        } else if (arg == "--dumpFolder" && i + 1 < argc) {
            dumpFolder = argv[++i];
        } else {
            return 1;
        }
    }

    if (listenPort.empty() || memSize == 0 || dumpFolder.empty()) {
        return 1;
    }

    RunServer(listenPort, memSize, dumpFolder);
    return 0;
}