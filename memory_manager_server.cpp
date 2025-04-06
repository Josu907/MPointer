#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "Proto/memory_manager.grpc.pb.h"
#include "Proto/memory_manager.pb.h"
#include "MemoryManager.hpp" // Asegúrate de que este sea el nombre correcto de tu header

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using memory_manager::MemoryManagerService;
using memory_manager::AllocateRequest;
using memory_manager::AllocateResponse;
using memory_manager::ReferenceRequest;
using memory_manager::Empty;
using memory_manager::SetRequest;
using memory_manager::SetResponse;
using memory_manager::GetRequest;
using memory_manager::GetResponse;

class MemoryManagerServiceImpl final : public MemoryManagerService::Service {
private:
    std::unique_ptr<MemoryManager> memoryManager;
    std::string dumpFolder;

public:
    MemoryManagerServiceImpl(size_t size, const std::string& dumpPath) : memoryManager(std::make_unique<MemoryManager>(size, dumpPath)), dumpFolder(dumpPath) {}

    Status Create(ServerContext* context, const AllocateRequest* request, AllocateResponse* response) override {
        int id = memoryManager->Create(request->size());
        if (id != -1) {
            response->set_id(id);
            response->set_success(true);
        } else {
            response->set_success(false);
            response->set_error_message("No hay suficiente memoria disponible.");
        }
        return Status::OK;
    }

    Status IncreaseRefCount(ServerContext* context, const ReferenceRequest* request, Empty* response) override {
        memoryManager->IncreaseRefCount(request->id());
        return Status::OK;
    }

    Status DecreaseRefCount(ServerContext* context, const ReferenceRequest* request, Empty* response) override {
        memoryManager->DecreaseRefCount(request->id());
        return Status::OK;
    }

    Status Set(ServerContext* context, const SetRequest* request, SetResponse* response) override {
        int id = request->id();
        const std::string& data = request->data();
        size_t size = data.size();
        try {
            memoryManager->Set(id, data.data(), size);
            response->set_success(true);
        } catch (const std::out_of_range& e) {
            response->set_success(false);
            response->set_error_message(e.what());
        } catch (const std::invalid_argument& e) {
            response->set_success(false);
            response->set_error_message(e.what());
        } catch (...) {
            response->set_success(false);
            response->set_error_message("Error desconocido al escribir en la memoria.");
        }
        return Status::OK;
    }

    Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
        int id = request->id();
        void* dataPtr = memoryManager->Get(id);
        size_t size = 0;
        {
            // Necesitamos obtener el tamaño del bloque para poder copiar los datos correctamente.
            // Podrías añadir un método a MemoryManager para obtener el tamaño de una asignación por ID.
            // Por ahora, lo dejaremos como 0 y la respuesta contendrá datos vacíos si no se encuentra.
            // Una mejor implementación requeriría almacenar o poder recuperar el tamaño.
            std::lock_guard<std::mutex> lock(memoryManager->memoryMutex);
            auto it = memoryManager->allocationSizes.find(id);
            if (it != memoryManager->allocationSizes.end()) {
                size = it->second;
            } else {
                response->set_success(false);
                response->set_error_message("Bloque ID " + std::to_string(id) + " no encontrado.");
                return Status::OK;
            }
        }

        if (dataPtr != nullptr && size > 0) {
            response->set_data(static_cast<char*>(dataPtr), size);
            response->set_success(true);
        } else if (response->error_message().empty()) {
            response->set_success(false);
            response->set_error_message("Error al obtener el bloque de memoria.");
        }
        return Status::OK;
    }
};

void RunServer(int port, size_t memorySizeMB, const std::string& dumpFolder) {
    std::string server_address = "0.0.0.0:" + std::to_string(port);
    MemoryManagerServiceImpl service(memorySizeMB * 1024 * 1024, dumpFolder); // Convert MB to bytes

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "[INFO] Servidor escuchando en " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "[ERROR] Uso: " << argv[0]
                  << " -port <LISTEN_PORT> -memsize <SIZE_MB> -dumpFolder <DUMP_FOLDER>" << std::endl;
        return 1;
    }

    int listenPort = -1;
    size_t memorySizeMB = 0;
    std::string dumpFolder;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-port" && i + 1 < argc) {
            listenPort = std::stoi(argv[++i]);
        } else if (std::string(argv[i]) == "-memsize" && i + 1 < argc) {
            memorySizeMB = std::stoul(argv[++i]);
        } else if (std::string(argv[i]) == "-dumpFolder" && i + 1 < argc) {
            dumpFolder = argv[++i];
        } else {
            std::cerr << "[ERROR] Parámetro desconocido: " << argv[i] << std::endl;
            return 1;
        }
    }

    if (listenPort == -1 || memorySizeMB == 0) {
        std::cerr << "[ERROR] Debes especificar el puerto de escucha y el tamaño de la memoria." << std::endl;
        return 1;
    }

    RunServer(listenPort, memorySizeMB, dumpFolder);

    return 0;
}