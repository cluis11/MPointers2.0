# Versión mínima requerida de CMake
cmake_minimum_required(VERSION 3.10)

# Nombre del proyecto
project(MPointers2.0)

# Configura el estándar de C++ a C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Opciones para Windows
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

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
    "${CMAKE_CURRENT_SOURCE_DIR}/server/main.cpp"  
    "${CMAKE_CURRENT_SOURCE_DIR}/server/src/MemoryBlock.cpp"  
    "${CMAKE_CURRENT_SOURCE_DIR}/server/src/MemoryList.cpp"  
    "${CMAKE_CURRENT_SOURCE_DIR}/server/src/MemoryManager.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/server/src/MemoryMap.cpp"   
    "${CMAKE_CURRENT_SOURCE_DIR}/server/src/MemoryDump.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/server/src/GarbageCollector.cpp"  
    ${PROTO_SRCS}
)

# Incluye directorios
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/server/include
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

# Ejecutable del servidor
add_executable(mem-mgr ${SERVER_SRCS})
target_link_libraries(mem-mgr
    PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)

# Configuración del cliente
set(CLIENT_SRCS
    client/client.cpp
    ${PROTO_SRCS}
)
# Ejecutable del cliente
add_executable(mem-mgr-client ${CLIENT_SRCS})
target_include_directories(mem-mgr-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/client
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
)
target_link_libraries(mem-mgr-client
    PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)