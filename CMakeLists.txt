cmake_minimum_required(VERSION 3.17)
project(MPointerMemoryManager)

# C++ versión
set(CMAKE_CXX_STANDARD 17)

# Busca gRPC y Protobuf
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

# Proto fuente
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Proto")

# Archivos .proto
set(PROTO_FILES
    ${PROTO_SRC_DIR}/memory_manager.proto
)

# Generar código de gRPC y protobuf
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_FILES})

# Directorio de salida para los generados
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROTO_SRC_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Fuente del servidor
set(SERVER_SOURCES
    memory_manager_server.cpp
    MemoryManager.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

# Crear ejecutable
add_executable(memory_manager_server ${SERVER_SOURCES})

# Enlazar con las bibliotecas necesarias
target_link_libraries(memory_manager_server
    gRPC::grpc++
    protobuf::libprotobuf
    pthread
)

# Mensaje bonito
message(STATUS "Proyecto listo. Ejecutable: memory_manager_server")
