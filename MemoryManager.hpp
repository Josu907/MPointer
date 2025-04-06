#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring> // memmove
#include <fstream>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip> // put_time
#include <stdexcept> // out_of_range, invalid_argument
#include <filesystem>

class MemoryManager {
private:
    void* memory;
    size_t totalSize;
    size_t usedSize;
    bool running;
    std::map<int, void*> allocations;
    std::map<int, size_t> allocationSizes;
    std::map<int, int> refCount;
    std::thread garbageCollector;
    std::mutex memoryMutex;

    void logMemoryAction(const std::string& action, int id) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        std::ofstream logFile("memory_log.txt", std::ios::app);
        if (logFile) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm now_tm = *std::localtime(&now_c);
            char timeBuffer[20];
            std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &now_tm);
            logFile << "[" << timeBuffer << "] " << action << " ID " << id << std::endl;
        }
    }

    void dumpMemory() {
        std::cout << "[DEBUG] Entrando en dumpMemory()" << std::endl;
        std::lock_guard<std::mutex> lock(memoryMutex);
        std::cout << "[DEBUG] Mutex adquirido en dumpMemory()" << std::endl;
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_c);
        std::stringstream ss;
        ss << "memory_dump_" << std::put_time(&now_tm, "%Y%m%d_%H%M%S") << ".bin";

        std::string fullPath = ss.str();
        std::cout << "[DEBUG] Intentando volcar en: " << fullPath << std::endl;

        std::ofstream dumpFile(fullPath, std::ios::binary);
        std::cout << "[DEBUG] Archivo de volcado (intentando abrir): " << fullPath << std::endl;
        if (dumpFile) {
            std::cout << "[DEBUG] Archivo de volcado abierto correctamente." << std::endl;
            dumpFile.write(static_cast<char*>(memory), totalSize);
            std::cout << "[INFO] Volcado de memoria realizado en " << fullPath << std::endl;
        } else {
            std::cerr << "[ERROR] No se pudo crear el archivo de volcado en: " << fullPath << std::endl;
        }
        std::cout << "[DEBUG] Saliendo de dumpMemory()" << std::endl;
    }

public:
    MemoryManager(size_t size) : totalSize(size), usedSize(0), running(true) {
        memory = malloc(size);
        if (!memory) throw std::bad_alloc();
        std::cout << "[INFO] Memoria reservada con " << size << " bytes." << std::endl;
        // garbageCollector = std::thread(&MemoryManager::runGarbageCollector, this);
    }

    ~MemoryManager() {
        running = false;
        // garbageCollector.join();
        free(memory);
        std::cout << "[INFO] Memoria liberada correctamente." << std::endl;
    }

    int Create(size_t size) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        if (usedSize + size > totalSize) {
            std::cerr << "[ERROR] No hay suficiente memoria disponible." << std::endl;
            return -1;
        }
        static int idCounter = 1;
        void* address = static_cast<char*>(memory) + usedSize;
        allocations[idCounter] = address;
        allocationSizes[idCounter] = size;
        refCount[idCounter] = 1;
        usedSize += size;
        std::cout << "[INFO] Creado bloque ID " << idCounter << " de " << size << " bytes." << std::endl;
        logMemoryAction("Creado bloque", idCounter);
        std::cout << "[DEBUG] Llamando a dumpMemory() desde Create()" << std::endl;
        // dumpMemory();
        std::cout << "[DEBUG] usedSize después de Create: " << usedSize << std::endl;
        std::cout << "[DEBUG] allocations.size() después de Create: " << allocations.size() << std::endl;
        std::cout << "[DEBUG] allocationSizes.size() después de Create: " << allocationSizes.size() << std::endl;
        return idCounter++;
    }

    void IncreaseRefCount(int id) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        if (refCount.find(id) != refCount.end()) {
            refCount[id]++;
            logMemoryAction("Incremento de referencia", id);
        }
    }

    void DecreaseRefCount(int id) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        if (refCount.find(id) != refCount.end()) {
            refCount[id]--;
            logMemoryAction("Decremento de referencia", id);
            if (refCount[id] <= 0) {
                std::cout << "[INFO] Bloque ID " << id << " marcado para liberación." << std::endl;
            }
        }
    }

    void Set(int id, const void* data, size_t size) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        auto it = allocations.find(id);
        if (it != allocations.end()) {
            if (size <= allocationSizes[id]) {
                std::memcpy(it->second, data, size);
                logMemoryAction("Datos escritos en", id);
                dumpMemory();
            } else {
                std::cerr << "[ERROR] Intento de escribir más allá del tamaño del bloque ID " << id << "." << std::endl;
                logMemoryAction("Error al escribir en", id);
                throw std::out_of_range("Tamaño de datos excede el tamaño del bloque.");
            }
        } else {
            std::cerr << "[ERROR] Bloque ID " << id << " no encontrado en Set." << std::endl;
            logMemoryAction("Error al escribir en", id);
            throw std::invalid_argument("ID de bloque inválido.");
        }
    }

    void* Get(int id) {
        std::lock_guard<std::mutex> lock(memoryMutex);
        if (allocations.find(id) != allocations.end()) {
            return allocations[id];
        }
        std::cerr << "[WARNING] Bloque ID " << id << " no encontrado en Get." << std::endl;
        return nullptr;
    }

    void runGarbageCollector() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            cleanMemory();
        }
    }

    void cleanMemory() {
        std::lock_guard<std::mutex> lock(memoryMutex);
        bool memoryModified = false;
        for (auto it = allocations.begin(); it != allocations.end(); ) {
            int id = it->first;
            if (refCount[id] <= 0) {
                usedSize -= allocationSizes[id];
                allocationSizes.erase(id);
                refCount.erase(id);
                logMemoryAction("Liberando bloque", id);
                it = allocations.erase(it);
                memoryModified = true;
            } else {
                ++it;
            }
        }
        if (memoryModified && !allocations.empty()) {
            Defragment();
            dumpMemory();
        } else if (memoryModified && allocations.empty()) {
            dumpMemory(); // Volcar incluso si no hay bloques después de la limpieza
        }
    }

    void Defragment() {
        size_t newOffset = 0;
        std::map<int, void*> newAllocations;
        for (auto& entry : allocations) {
            int id = entry.first;
            void* oldAddress = entry.second;
            size_t size = allocationSizes[id];
            if (refCount[id] > 0) {
                void* newAddress = static_cast<char*>(memory) + newOffset;
                if (newAddress != oldAddress) {
                    std::memmove(newAddress, oldAddress, size);
                }
                newAllocations[id] = newAddress;
                newOffset += size;
            }
        }
        allocations = std::move(newAllocations);
        usedSize = newOffset;
        logMemoryAction("Defragmentación completada", -1);
        if (!allocations.empty()) {
            dumpMemory(); // Volcar después de la defragmentación
        }
    }
};