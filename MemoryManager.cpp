#include "MemoryManager.h"
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

#define GREEN   "\033[32m"
#define RESET   "\033[0m"

MemoryManager::MemoryManager(size_t sizeInMB) {
    totalSize = sizeInMB * 1024 * 1024;
    memory = (uint8_t*)malloc(totalSize);
    currentOffset = 0;
    nextId = 1;

    std::cout << GREEN << "[MemoryManager] Inicializado con " << sizeInMB << " MB (" << totalSize << " bytes)" << RESET << std::endl;
}

MemoryManager::~MemoryManager() {
    if (memory) {
        free(memory);
        std::cout << GREEN << "[MemoryManager] Memoria liberada al destruir MemoryManager" << RESET << std::endl;
    }
}

void MemoryManager::setDumpFolder(const std::string& folder) {
    dumpFolder = folder;
}

void MemoryManager::dumpMemory(const std::string& operation) {
    using namespace std::chrono;

    if (!operation.empty())
        lastOperation = operation;

    auto now = high_resolution_clock::now();
    auto timeNow = system_clock::now();
    auto in_time_t = system_clock::to_time_t(timeNow);

    auto ns = duration_cast<nanoseconds>(now.time_since_epoch()) % 1000000000;

    std::stringstream filename;
    std::tm buf;
    localtime_r(&in_time_t, &buf);

    filename << dumpFolder << "/";
    filename << std::put_time(&buf, "%Y-%m-%d_%H-%M-%S")
             << "-" << std::setw(9) << std::setfill('0') << ns.count()
             << ".txt";

    std::ofstream file(filename.str());
    if (!file.is_open()) {
        std::cerr << GREEN << "[MemoryManager]  No se pudo crear el archivo de dump: " << filename.str() << RESET << std::endl;
        return;
    }

    size_t usedMemory = currentOffset;
    size_t freeMemory = totalSize - usedMemory;

    file << "[Memory Dump - " << std::put_time(&buf, "%Y-%m-%d %H:%M:%S")
         << "." << std::setw(9) << std::setfill('0') << ns.count() << "]\n";
    file << "Operación: " << lastOperation << "\n";
    file << "Memoria total: " << totalSize << " bytes\n";
    file << "Memoria utilizada: " << usedMemory << " bytes\n";
    file << "Memoria libre: " << freeMemory << " bytes\n\n";

    for (const auto& [id, block] : blocks) {
        file << "Bloque ID: " << id
             << " | Offset: " << block.offset
             << " | Tamaño: " << block.size
             << " | RefCount: " << block.refCount
             << " | Estado: " << (block.free ? "LIBRE" : "OCUPADO") << "\n";
    }

    file.close();
    std::cout << GREEN << "[MemoryManager] Dump creado tras operación '" << lastOperation << "': " << filename.str() << RESET << std::endl;
}

int MemoryManager::create(size_t size) {
    if (currentOffset + size > totalSize)
        throw std::runtime_error("No hay suficiente memoria");

    int id = nextId++;
    blocks[id] = {currentOffset, size, 1, false};
    currentOffset += size;

    std::cout << GREEN << "[MemoryManager] Bloque creado con ID " << id << ", tamaño: " << size << " bytes, offset: " << blocks[id].offset << RESET << std::endl;
    dumpMemory("create");
    return id;
}


void MemoryManager::set(int id, const void* value, size_t size) {
    void* addr = getAddress(id);
    memcpy(addr, value, size);
    std::cout << GREEN << "[MemoryManager] Valor establecido en bloque ID " << id << ", tamaño: " << size << " bytes" << RESET << std::endl;
    dumpMemory("set");
}

void MemoryManager::get(int id, void* out, size_t size) {
    void* addr = getAddress(id);
    memcpy(out, addr, size);
    std::cout << GREEN << "[MemoryManager] Valor obtenido del bloque ID " << id << ", tamaño: " << size << " bytes" << RESET << std::endl;
}

void* MemoryManager::getAddress(int id) {
    if (!blocks.count(id) || blocks[id].free)
        throw std::runtime_error("Bloque no válido o ya liberado");

    return memory + blocks[id].offset;
}

void MemoryManager::increaseRefCount(int id) {
    if (blocks.count(id)) {
        blocks[id].refCount++;
        std::cout << GREEN << "[MemoryManager] RefCount incrementado para bloque ID " << id << " a " << blocks[id].refCount << RESET << std::endl;
        dumpMemory("increaseRefCount");
    }
}

void MemoryManager::decreaseRefCount(int id) {
    if (blocks.count(id)) {
        blocks[id].refCount--;
        std::cout << GREEN << "[MemoryManager] RefCount decrementado para bloque ID " << id << " a " << blocks[id].refCount << RESET << std::endl;

        if (blocks[id].refCount <= 0) {
            blocks[id].free = true;
            std::cout << GREEN << "[MemoryManager] Bloque ID " << id << " marcado como libre (RefCount = 0)" << RESET << std::endl;
        }
        dumpMemory("decreaseRefCount");
    }
}

void MemoryManager::collectGarbage() {
    std::cout << GREEN << "[MemoryManager] Ejecutando garbage collector..." << RESET << std::endl;

    for (auto& [id, block] : blocks) {
        if (block.refCount <= 0 && !block.free) {
            block.free = true;
            std::cout << GREEN << "[MemoryManager] Recolectado bloque ID " << id << RESET << std::endl;
        }
    }
}
