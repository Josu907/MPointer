#include "MemoryManager.h"
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>   // Incluir para std::vector
#include <algorithm> // Incluir para std::sort
#include <map>       // Incluir para std::map en compactación

#define GREEN   "\033[32m"
#define YELLOW  "\033[33m" // Para GC y Defrag
#define RESET   "\033[0m"

// --- CONSTRUCTOR MODIFICADO ---
MemoryManager::MemoryManager(size_t sizeInMB) : stopGcThread(false) { // Inicializar atomic
    totalSize = sizeInMB * 1024 * 1024;
    memory = (uint8_t*)malloc(totalSize);
    if (!memory) {
        throw std::runtime_error("Fallo al asignar memoria inicial");
    }
    currentOffset = 0; // Representa el inicio del espacio libre contiguo
    nextId = 1;

    std::cout << GREEN << "[MemoryManager] Inicializado con " << sizeInMB << " MB (" << totalSize << " bytes)" << RESET << std::endl;

    // Iniciar el hilo del Garbage Collector
    gcThread = std::thread(&MemoryManager::runGarbageCollector, this);
    std::cout << YELLOW << "[MemoryManager] Hilo Garbage Collector iniciado." << RESET << std::endl;
}

// --- DESTRUCTOR MODIFICADO ---
MemoryManager::~MemoryManager() {
    std::cout << YELLOW << "[MemoryManager] Deteniendo hilo Garbage Collector..." << RESET << std::endl;
    stopGcThread.store(true); // Señalizar al hilo que debe detenerse
    if (gcThread.joinable()) {
        gcThread.join(); // Esperar a que el hilo termine
        std::cout << YELLOW << "[MemoryManager] Hilo Garbage Collector detenido." << RESET << std::endl;
    }

    if (memory) {
        free(memory);
        std::cout << GREEN << "[MemoryManager] Memoria liberada al destruir MemoryManager" << RESET << std::endl;
    }
}

// --- NUEVA FUNCIÓN: BUCLE DEL GARBAGE COLLECTOR ---
void MemoryManager::runGarbageCollector() {
    using namespace std::chrono_literals;
    while (!stopGcThread.load()) {
        // Esperar un intervalo antes de la siguiente recolección
        std::this_thread::sleep_for(5s); // Ejemplo: recolectar cada 5 segundos

        // Bloquear el mutex antes de acceder/modificar datos compartidos
        std::lock_guard<std::mutex> lock(memoryMutex);

        std::cout << YELLOW << "[GC Thread] Ejecutando ciclo de recolección..." << RESET << std::endl;
        bool blockFreed = false;
        for (auto it = blocks.begin(); it != blocks.end(); /* no incrementar aquí */) {
            if (it->second.refCount <= 0 && !it->second.free) {
                std::cout << YELLOW << "[GC Thread] Marcando bloque ID " << it->first << " como libre." << RESET << std::endl;
                it->second.free = true; // Marcar como libre
                blockFreed = true;
                // NO eliminar del mapa aquí si quieres que defragmentación lo vea
                // Si quisieras eliminarlo completamente:
                // it = blocks.erase(it);
                // continue; // Saltar al siguiente ciclo del bucle
                 ++it; // Incrementar solo si no se elimina
            } else {
                ++it;
            }
        }

        if (blockFreed) {
            // Opcional: Podrías llamar a mergeFreeBlocks aquí si la implementas
            // mergeFreeBlocks();
             dumpMemory("GC Run"); // Hacer dump si algo cambió
        }
         std::cout << YELLOW << "[GC Thread] Ciclo de recolección finalizado." << RESET << std::endl;
        // El lock_guard se libera automáticamente al salir del scope
    }
     std::cout << YELLOW << "[GC Thread] Hilo terminando." << RESET << std::endl;
}

// --- NUEVA FUNCIÓN: IMPLEMENTACIÓN DE DEFRAGMENTACIÓN (COMPACTACIÓN) ---
void MemoryManager::compactMemory() {
    std::cout << YELLOW << "[Defragment] Iniciando compactación de memoria..." << RESET << std::endl;

    // Crear un vector de pares (offset, id) solo para bloques OCUPADOS
    std::vector<std::pair<size_t, int>> usedBlocks;
    usedBlocks.reserve(blocks.size());
    for (const auto& [id, block] : blocks) {
        if (!block.free) {
            usedBlocks.push_back({block.offset, id});
        }
    }

    // Ordenar los bloques usados por su offset actual
    std::sort(usedBlocks.begin(), usedBlocks.end());

    size_t newOffset = 0; // El nuevo offset comienza desde 0
    uint8_t* newMemory = (uint8_t*)malloc(totalSize); // Buffer temporal
    if (!newMemory) {
         std::cerr << "[Defragment] ⚠️ Error: No se pudo asignar buffer temporal para compactación." << std::endl;
         return; // O manejar el error de otra forma
    }

    std::cout << YELLOW << "[Defragment] Moviendo " << usedBlocks.size() << " bloques usados..." << RESET << std::endl;

    // Mapa temporal para guardar los nuevos offsets antes de actualizar el mapa principal
    std::map<int, size_t> newOffsets;

    for (const auto& pair : usedBlocks) {
        int id = pair.second;
        const auto& currentBlock = blocks[id]; // Obtener info del bloque actual

        // Copiar los datos del bloque actual a la nueva posición en el buffer temporal
        // Usar memcpy es seguro aquí porque copiamos a un buffer diferente
        memcpy(newMemory + newOffset, memory + currentBlock.offset, currentBlock.size);

        // Guardar el nuevo offset para este ID
        newOffsets[id] = newOffset;

        std::cout << YELLOW << "[Defragment] Moviendo Bloque ID " << id
                  << " de offset " << currentBlock.offset << " a " << newOffset
                  << " (Tamaño: " << currentBlock.size << ")" << RESET << std::endl;

        // Avanzar el nuevo offset
        newOffset += currentBlock.size;
    }

    // Reemplazar la memoria antigua con la nueva memoria compactada
    free(memory);
    memory = newMemory;

    // Actualizar los offsets en el mapa 'blocks'
    std::cout << YELLOW << "[Defragment] Actualizando offsets en la tabla de bloques..." << RESET << std::endl;
    for(const auto& [id, offset] : newOffsets) {
        blocks[id].offset = offset;
    }

    // Actualizar el 'currentOffset' global para futuras asignaciones simples (si usas ese método)
    currentOffset = newOffset; // Ahora apunta al final de los datos compactados

    std::cout << YELLOW << "[Defragment] Compactación finalizada. Memoria usada contigua: " << currentOffset << " bytes." << RESET << std::endl;
}


// --- NUEVA FUNCIÓN PÚBLICA PARA LLAMAR A LA DEFRAGMENTACIÓN ---
void MemoryManager::defragment() {
    // Bloquear durante toda la operación de compactación
    std::lock_guard<std::mutex> lock(memoryMutex);
    std::cout << YELLOW << "[MemoryManager] Solicitud de defragmentación recibida." << RESET << std::endl;
    compactMemory(); // Llama a la lógica interna
    dumpMemory("defragment"); // Hacer dump después de defragmentar
}

// --- MODIFICACIÓN DE FUNCIONES EXISTENTES PARA USAR MUTEX ---

void MemoryManager::setDumpFolder(const std::string& folder) {
    // No necesita mutex si solo asigna string y dumpFolder no se usa concurrentemente
    // Pero si se accediera desde el hilo GC, necesitaría protección. Asumimos que no.
    dumpFolder = folder;
}

// dumpMemory idealmente debería tomar el lock si accede a 'blocks' o 'currentOffset'
// Lo haremos opcional para evitar deadlocks si se llama desde una función que ya tiene el lock
void MemoryManager::dumpMemory(const std::string& operation) {
    // Tomar el lock solo si no lo tenemos ya (esto es complejo de saber directamente)
    // Una forma simple es NO tomar el lock aquí, y asegurarse que quien llama a dumpMemory
    // ya tiene el lock si es necesario (como hacemos en las otras funciones).
    // std::lock_guard<std::mutex> lock(memoryMutex); // <- Podría causar doble bloqueo

    using namespace std::chrono;
     if (!operation.empty())
        lastOperation = operation;

    // ... (resto del código de dumpMemory sin cambios)...

    // Leer 'blocks' requiere protección si se llama desde fuera de un lock
    // Como lo llamamos desde funciones que ya tienen lock, está bien por ahora.
     size_t calculatedUsed = 0;
     for (const auto& [id, block] : blocks) {
         if (!block.free) calculatedUsed += block.size;
     }
     // Usar calculatedUsed en lugar de currentOffset para el dump, ya que currentOffset
     // podría no ser preciso después de liberaciones/defragmentación.
     size_t freeMemory = totalSize - calculatedUsed;


    file << "[Memory Dump - " << /* ... */ << "]\n";
    file << "Operación: " << lastOperation << "\n";
    file << "Memoria total: " << totalSize << " bytes\n";
    file << "Memoria utilizada (calculada): " << calculatedUsed << " bytes\n"; // Modificado
    file << "Memoria libre: " << freeMemory << " bytes\n\n";
     file << "Blocks Map (" << blocks.size() << " entradas):\n";
    for (const auto& [id, block] : blocks) {
        file << "  Bloque ID: " << id
             << " | Offset: " << block.offset
             << " | Tamaño: " << block.size
             << " | RefCount: " << block.refCount
             << " | Estado: " << (block.free ? "LIBRE" : "OCUPADO") << "\n";
    }
     // ... (resto sin cambios) ...
}

// --- IMPLEMENTACIÓN DE CREATE (Ejemplo Básico - SIN búsqueda de huecos) ---
// Esta versión es muy simple y no reutiliza bloques libres ni busca huecos.
// Necesitarías una lógica más avanzada para eso ("first fit", "best fit").
int MemoryManager::create(size_t size) {
    std::lock_guard<std::mutex> lock(memoryMutex); // BLOQUEAR

    // Lógica de asignación MUY simple: solo al final
    if (currentOffset + size > totalSize) {
        // Aquí podrías intentar llamar a compactMemory() si quieres que sea automático
        // compactMemory();
        // Y luego volver a verificar:
        // if (currentOffset + size > totalSize) {
        //     throw std::runtime_error("Out of memory");
        // }
        // O simplemente fallar:
         dumpMemory("create failed - OOM");
        throw std::runtime_error("Out of memory (simple allocation)");
    }

    int newId = nextId++;
    blocks[newId] = {currentOffset, size, 1, false}; // refCount empieza en 1, no libre
    currentOffset += size; // Avanzar offset lineal

    std::cout << GREEN << "[MemoryManager] Bloque creado con ID " << newId << ", tamaño: " << size << " bytes, offset: " << blocks[newId].offset << RESET << std::endl;
    dumpMemory("create");
    return newId;
}


void MemoryManager::set(int id, const void* value, size_t size) {
    std::lock_guard<std::mutex> lock(memoryMutex); // BLOQUEAR
    if (!blocks.count(id) || blocks[id].free) {
         dumpMemory("set failed - invalid ID");
        throw std::runtime_error("Bloque no válido o ya liberado para set");
    }
     if (size > blocks[id].size) {
         dumpMemory("set failed - size mismatch");
         throw std::runtime_error("Tamaño de valor excede tamaño de bloque asignado");
     }

    void* addr = memory + blocks[id].offset;
    memcpy(addr, value, size);
    std::cout << GREEN << "[MemoryManager] Valor establecido en bloque ID " << id << ", tamaño: " << size << " bytes" << RESET << std::endl;
    dumpMemory("set");
}

void MemoryManager::get(int id, void* out, size_t size) {
    std::lock_guard<std::mutex> lock(memoryMutex); // BLOQUEAR
     if (!blocks.count(id) || blocks[id].free) {
         // dumpMemory("get failed - invalid ID"); // dump opcional en get
        throw std::runtime_error("Bloque no válido o ya liberado para get");
    }
     if (size > blocks[id].size) {
         // dumpMemory("get failed - size mismatch");
         throw std::runtime_error("Tamaño solicitado excede tamaño de bloque asignado");
     }
    void* addr = memory + blocks[id].offset;
    memcpy(out, addr, size);
    std::cout << GREEN << "[MemoryManager] Valor obtenido del bloque ID " << id << ", tamaño: " << size << " bytes" << RESET << std::endl;
    // dumpMemory("get"); // Dump en get es opcional
}

// getAddress ahora es más peligroso si se usa externamente debido a la defragmentación
void* MemoryManager::getAddress(int id) {
    std::lock_guard<std::mutex> lock(memoryMutex); // BLOQUEAR
    if (!blocks.count(id) || blocks[id].free)
        throw std::runtime_error("Bloque no válido o ya liberado para getAddress");

    return memory + blocks[id].offset;
}

void MemoryManager::increaseRefCount(int id) {
    std::lock_guard<std::mutex> lock(memoryMutex); // BLOQUEAR
    if (blocks.count(id) && !blocks[id].free) { // Solo incrementar si existe y no está libre
        blocks[id].refCount++;
        std::cout << GREEN << "[MemoryManager] RefCount incrementado para bloque ID " << id << " a " << blocks[id].refCount << RESET << std::endl;
        dumpMemory("increaseRefCount");
    } else {
         std::cout << YELLOW << "[MemoryManager] Advertencia: Intento de increaseRefCount en bloque ID " << id << " no válido o libre." << RESET << std::endl;
         dumpMemory("increaseRefCount failed");
    }
}

void MemoryManager::decreaseRefCount(int id) {
    std::lock_guard<std::mutex> lock(memoryMutex); // BLOQUEAR
    if (blocks.count(id) && !blocks[id].free) { // Solo decrementar si existe y no está libre
        blocks[id].refCount--;
        std::cout << GREEN << "[MemoryManager] RefCount decrementado para bloque ID " << id << " a " << blocks[id].refCount << RESET << std::endl;

        if (blocks[id].refCount <= 0) {
             // El hilo GC se encargará de marcarlo como 'free' eventualmente.
             // No lo marcamos aquí para evitar conflictos con el hilo GC.
            std::cout << GREEN << "[MemoryManager] RefCount para bloque ID " << id << " es <= 0. Será recolectado por GC." << RESET << std::endl;
        }
        dumpMemory("decreaseRefCount");
    } else {
        std::cout << YELLOW << "[MemoryManager] Advertencia: Intento de decreaseRefCount en bloque ID " << id << " no válido o libre." << RESET << std::endl;
        dumpMemory("decreaseRefCount failed");
    }
}

// Esta función manual ahora es menos necesaria, pero se puede mantener.
// El hilo GC hace esto periódicamente.
void MemoryManager::collectGarbage() {
     std::lock_guard<std::mutex> lock(memoryMutex); // BLOQUEAR
    std::cout << YELLOW << "[MemoryManager] Ejecutando garbage collector MANUAL..." << RESET << std::endl;
    bool blockFreed = false;
    for (auto& [id, block] : blocks) {
        if (block.refCount <= 0 && !block.free) {
            block.free = true; // Marcar como libre
            blockFreed = true;
            std::cout << YELLOW << "[MemoryManager] Recolectado (manual) bloque ID " << id << RESET << std::endl;
        }
    }
     if (blockFreed) dumpMemory("Manual GC Run");
}

// --- NUEVA FUNCIÓN PRIVADA AUXILIAR (OPCIONAL) ---
void MemoryManager::mergeFreeBlocks() {
    // Esta función necesitaría lógica para encontrar bloques marcados como 'free'
    // que sean adyacentes en memoria y unirlos en un solo bloque 'free' más grande.
    // Requiere ordenar por offset o iterar cuidadosamente.
    // Por simplicidad, no se implementa aquí, pero sería una mejora.
     std::cout << YELLOW << "[MemoryManager] Lógica de mergeFreeBlocks no implementada." << RESET << std::endl;
}