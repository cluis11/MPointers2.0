# Versión mínima requerida de CMake
cmake_minimum_required(VERSION 3.10)

# Nombre del proyecto
project(MPointers2.0)

# Configura el estándar de C++ a C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encuentra las librerías de gRPC y Protocol Buffers
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Archivos generados manualmente a partir del .proto
set(PROTO_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.pb.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.grpc.pb.cc"
)

# Archivos de cabecera generados manualmente
set(PROTO_HDRS
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.pb.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/memory_manager.grpc.pb.h"
)

# Archivos fuente del servidor
set(SERVER_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/server/MemoryMap.cpp"  
)

set(CLIENT_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/client/client.cpp"
)

# Ejecutable del servidor
add_executable(mem-mgr ${SERVER_SRCS} ${PROTO_SRCS})
# Ejecutable del cliente
add_executable(mem-mgr-client ${CLIENT_SRCS} ${PROTO_SRCS})

# Incluye los directorios de los archivos generados por protoc
target_include_directories(mem-mgr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/proto)
# Incluye los directorios de los archivos generados por protoc
target_include_directories(mem-mgr-client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/proto)

# Enlaza las librerías necesarias
target_link_libraries(mem-mgr
    PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)

# Enlaza las librerías necesarias
target_link_libraries(mem-mgr-client
    PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)