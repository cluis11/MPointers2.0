#include "Server.h"
#include <grpcpp/grpcpp.h> 
#include <iostream>

void MemoryServer::Run(const std::string& listenPort, std::size_t memSize) {
    MemoryBlock memoryBlock(memSize); // Inicializa el MemoryBlock con el tamaño especificado

    MemoryManagerServiceImpl service(memoryBlock);

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:" + listenPort, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Servidor escuchando en el puerto " << listenPort << std::endl;
    server->Wait();
}