cmake_minimum_required(VERSION 3.10)
project(MPointers2.0)

# Configuración de C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Búsqueda de dependencias
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Archivos generados del proto (NO MODIFICADO)
set(PROTO_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.grpc.pb.cc"
)

set(PROTO_HDRS
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.grpc.pb.h"
)

### CONFIGURACIÓN DEL SERVIDOR  ###
set(SERVER_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/server/MemoryMap.cpp"  
)

add_executable(mem-mgr ${SERVER_SRCS} ${PROTO_SRCS})
target_include_directories(mem-mgr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/proto)
target_link_libraries(mem-mgr
    PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)

### CONFIGURACIÓN DEL CLIENTE  ###
set(CLIENT_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/client/client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/MPointer.h"  # MPointers.h en client/
    "${CMAKE_CURRENT_SOURCE_DIR}/client/MPointerID.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/MemoryManagerClient.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/node.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/client/LinkedList.h"
    
)

add_executable(mem-mgr-client ${CLIENT_SRCS} ${PROTO_SRCS})

# Incluye solo los directorios necesarios para el cliente
target_include_directories(mem-mgr-client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
    ${CMAKE_CURRENT_SOURCE_DIR}/client  # Para encontrar MPointers.h
)

target_link_libraries(mem-mgr-client
    PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)