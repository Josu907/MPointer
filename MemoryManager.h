#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

struct MemoryBlock {
    size_t offset;
    size_t size;
    int refCount;
    bool free;

};

class MemoryManager {
private:
    uint8_t* memory;
    size_t totalSize;
    std::unordered_map<int, MemoryBlock> blocks;
    int nextId;
    size_t currentOffset;
    std::string dumpFolder = "dumps"; // Ruta por defecto
    std::string lastOperation = "Inicialización";


public:
    MemoryManager(size_t sizeInMB);
    ~MemoryManager();

    int create(size_t size);
    void set(int id, const void* value, size_t size);
    void get(int id, void* out, size_t size);
    void increaseRefCount(int id);
    void decreaseRefCount(int id);
    void collectGarbage();

    void dumpMemory(const std::string& operation = "");
    void setDumpFolder(const std::string& folder);

private:
    void* getAddress(int id);
};

#endif
