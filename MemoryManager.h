#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <cstdint>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <vector> // Añadido para defragmentación
#include <thread> // Añadido para GC Thread
#include <mutex>  // Añadido para sincronización
#include <atomic> // Añadido para controlar el hilo
#include <chrono> // Añadido para el sleep del hilo

struct BlockInfo {
    size_t offset;
    size_t size;
    int refCount;
    bool free; // Indica si el bloque está lógicamente libre
};

class MemoryManager {
private:
    uint8_t* memory;
    size_t totalSize;
    size_t currentOffset; // Seguirá representando el final de la última asignación *inicial* o post-compactación
    int nextId;
    std::unordered_map<int, BlockInfo> blocks;
    std::string dumpFolder = "."; // Inicializar a un valor por defecto
    std::string lastOperation = "N/A";

    // --- NUEVOS MIEMBROS ---
    std::thread gcThread;
    std::atomic<bool> stopGcThread{false}; // Para detener el hilo limpiamente
    std::mutex memoryMutex; // Mutex para proteger el acceso a 'blocks' y 'memory'

    // --- NUEVAS FUNCIONES PRIVADAS ---
    void runGarbageCollector(); // Función ejecutada por el hilo GC
    void compactMemory(); // Lógica de defragmentación (compactación)

    // Función auxiliar (puede ser útil)
    void mergeFreeBlocks(); // Opcional: para unir bloques libres adyacentes

public:
    MemoryManager(size_t sizeInMB);
    ~MemoryManager();

    void setDumpFolder(const std::string& folder);
    void dumpMemory(const std::string& operation = ""); // Permitir llamar sin operación

    int create(size_t size); // Asumiendo que tienes o crearás esta función
    void set(int id, const void* value, size_t size);
    void get(int id, void* out, size_t size);
    void* getAddress(int id); // Cuidado al usar fuera de la clase post-defragmentación
    void increaseRefCount(int id);
    void decreaseRefCount(int id);

    // --- NUEVA FUNCIÓN PÚBLICA ---
    void defragment(); // Función para llamar externamente a la defragmentación

    // --- FUNCIÓN EXISTENTE (ahora menos relevante si el hilo está activo) ---
    void collectGarbage(); // Puede ser llamada manualmente si se desea

};

#endif // MEMORYMANAGER_H