#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>

class MemoryManager {
private:
    void* memory; // Bloque de memoria principal
    size_t totalSize;
    size_t usedSize;
    bool running;

    std::map<int, void*> allocations;      // Mapa de ID a dirección en memoria
    std::map<int, size_t> allocationSizes; // Mapa de ID a tamaño del bloque
    std::map<int, int> refCount;           // Mapa de ID a número de referencias
    std::thread garbageCollector;          // Hilo del Garbage Collector

public:
    MemoryManager(size_t size) {
        totalSize = size;
        usedSize = 0;
        running = true;
        memory = malloc(size); // Se reserva un solo bloque de memoria
        
        if (!memory) {
            throw std::bad_alloc(); // Si falla la reserva, lanza error
        }

        std::cout << "[INFO] Memoria reservada con " << size << " bytes." << std::endl;

        // Iniciar el hilo del Garbage Collector
        garbageCollector = std::thread(&MemoryManager::runGarbageCollector, this);
    }

    ~MemoryManager() {
        running = false;  // Se detiene el Garbage Collector
        garbageCollector.join(); // Espera que termine
        free(memory); // Libera la memoria principal
        std::cout << "[INFO] Memoria liberada correctamente." << std::endl;
    }

    // Crea un bloque de memoria dentro del espacio ya reservado
    int Create(size_t size) {
        if (usedSize + size > totalSize) {
            std::cerr << "[ERROR] No hay suficiente memoria disponible." << std::endl;
            return -1; // Indica error
        }

        static int idCounter = 1; // Genera IDs únicos
        void* address = static_cast<char*>(memory) + usedSize;

        allocations[idCounter] = address;
        allocationSizes[idCounter] = size;
        refCount[idCounter] = 1;

        usedSize += size;

        std::cout << "[INFO] Creado bloque ID " << idCounter << " de " << size << " bytes en " << address << std::endl;
        return idCounter++;
    }

    // Establece datos en un bloque de memoria
    bool Set(int id, const void* data, size_t size) {
        if (allocations.find(id) == allocations.end()) {
            std::cerr << "[ERROR] Bloque ID " << id << " no encontrado." << std::endl;
            return false;
        }

        if (size > allocationSizes[id]) {
            std::cerr << "[ERROR] Tamaño de datos excede el tamaño del bloque ID " << id << "." << std::endl;
            return false;
        }

        std::memcpy(allocations[id], data, size);
        std::cout << "[INFO] Datos escritos en bloque ID " << id << "." << std::endl;
        return true;
    }

    // Obtiene datos de un bloque de memoria
    bool Get(int id, void* buffer, size_t size) {
        if (allocations.find(id) == allocations.end()) {
            std::cerr << "[ERROR] Bloque ID " << id << " no encontrado." << std::endl;
            return false;
        }

        if (size > allocationSizes[id]) {
            std::cerr << "[ERROR] Tamaño de lectura excede el tamaño del bloque ID " << id << "." << std::endl;
            return false;
        }

        std::memcpy(buffer, allocations[id], size);
        std::cout << "[INFO] Datos leídos desde el bloque ID " << id << "." << std::endl;
        return true;
    }

    // Aumentar el contador de referencias
    void IncreaseRefCount(int id) {
        if (refCount.find(id) != refCount.end()) {
            refCount[id]++;
            std::cout << "[INFO] Referencias de bloque ID " << id << " ahora son " << refCount[id] << std::endl;
        }
    }

    // Disminuir el contador de referencias
    void DecreaseRefCount(int id) {
        if (refCount.find(id) != refCount.end()) {
            refCount[id]--;
            std::cout << "[INFO] Referencias de bloque ID " << id << " ahora son " << refCount[id] << std::endl;

            if (refCount[id] <= 0) {
                std::cout << "[INFO] Bloque ID " << id << " marcado para liberación." << std::endl;
            }
        }
    }

    // Hilo del Garbage Collector
    void runGarbageCollector() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            cleanMemory();
        }
    }

    // Limpieza de bloques con 0 referencias
    void cleanMemory() {
        for (auto it = allocations.begin(); it != allocations.end(); ) {
            int id = it->first;
            if (refCount[id] <= 0) {
                std::cout << "[INFO] Liberando bloque ID " << id << std::endl;
                usedSize -= allocationSizes[id]; // Se reduce el uso de memoria
                allocationSizes.erase(id);
                refCount.erase(id);
                it = allocations.erase(it); // Elimina el bloque de la lista
            } else {
                ++it;
            }
        }
    }
};

// ** PRUEBAS EN main() **
int main() {
    MemoryManager manager(1024); // 1 KB de memoria

    int id1 = manager.Create(128);
    int id2 = manager.Create(256);

    manager.IncreaseRefCount(id1);
    manager.DecreaseRefCount(id1);
    manager.DecreaseRefCount(id2); // Se marcará para liberación

    // ** Prueba de Set y Get **
    int value = 42;
    manager.Set(id1, &value, sizeof(value));

    int retrievedValue = 0;
    manager.Get(id1, &retrievedValue, sizeof(retrievedValue));
    std::cout << "[TEST] Valor obtenido: " << retrievedValue << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(6)); // Esperar al GC

    return 0;
}
