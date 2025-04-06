#include "MemoryManager.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <functional> // Para std::function
#include <cstring>    // Para std::strcmp
#include <thread>     // Para std::this_thread
#include <chrono>     // Para std::chrono
#include <stdexcept>  // Para std::out_of_range

void runTest(const std::string& testName, std::function<void(MemoryManager&)> testFunction, MemoryManager& manager) {
    std::cout << "[TEST] Iniciando prueba: " << testName << std::endl;
    testFunction(manager);
    std::cout << "[TEST] Se completó la prueba: " << testName << std::endl << std::endl;
}

int main() {
    const size_t memorySize = 256;
    MemoryManager manager(memorySize);

    // Prueba 1: Crear bloques y verificar sus IDs
    runTest("Creación de bloques", [&](MemoryManager& mm) {
        int id1 = mm.Create(50);
        int id2 = mm.Create(100);
        if (id1 != -1 && id2 != -1 && id1 != id2) {
            std::cout << "[INFO] Prueba de creación de bloques: ÉXITO. IDs creados: " << id1 << ", " << id2 << std::endl;
        } else {
            std::cout << "[ERROR] Prueba de creación de bloques: FALLO." << std::endl;
        }
    }, manager);

    // Prueba 2: Intentar crear un bloque demasiado grande
    runTest("Creación de bloque sin memoria suficiente", [&](MemoryManager& mm) {
        int id = mm.Create(memorySize + 1);
        if (id == -1) {
            std::cout << "[INFO] Prueba de falta de memoria: ÉXITO. No se pudo crear el bloque." << std::endl;
        } else {
            std::cout << "[ERROR] Prueba de falta de memoria: FALLO. Se creó un bloque inesperadamente." << std::endl;
        }
    }, manager);

    // Prueba 3: Escribir y leer datos de un bloque
    runTest("Escritura y lectura de datos", [&](MemoryManager& mm) {
        int id = mm.Create(20);
        if (id != -1) {
            const char* dataToWrite = "Hola";
            size_t dataSize = strlen(dataToWrite) + 1;
            mm.Set(id, dataToWrite, dataSize);
            void* readData = mm.Get(id);
            if (readData != nullptr && std::strcmp(static_cast<const char*>(readData), dataToWrite) == 0) {
                std::cout << "[INFO] Prueba de escritura y lectura: ÉXITO. Datos leídos correctamente." << std::endl;
            } else {
                std::cout << "[ERROR] Prueba de escritura y lectura: FALLO." << std::endl;
            }
        } else {
            std::cout << "[ERROR] Prueba de escritura y lectura: FALLO al crear el bloque." << std::endl;
        }
    }, manager);

    // Prueba 4: Intentar escribir fuera de los límites
    runTest("Escritura fuera de límites", [&](MemoryManager& mm) {
        int id = mm.Create(10);
        if (id != -1) {
            const char* dataToWrite = "Esto es demasiado largo";
            size_t dataSize = strlen(dataToWrite) + 1;
            try {
                mm.Set(id, dataToWrite, dataSize);
                std::cout << "[ERROR] Prueba de escritura fuera de límites: FALLO. No se lanzó excepción." << std::endl;
            } catch (const std::out_of_range& e) {
                std::cout << "[INFO] Prueba de escritura fuera de límites: ÉXITO. Se lanzó la excepción esperada." << std::endl;
            } catch (...) {
                std::cout << "[ERROR] Prueba de escritura fuera de límites: FALLO. Se lanzó una excepción inesperada." << std::endl;
            }
        } else {
            std::cout << "[ERROR] Prueba de escritura fuera de límites: FALLO al crear el bloque." << std::endl;
        }
    }, manager);

    // Prueba 5: Verificar el incremento y decremento del contador de referencias
    runTest("Contador de referencias", [&](MemoryManager& mm) {
        int id = mm.Create(30);
        if (id != -1) {
            mm.IncreaseRefCount(id);
            mm.DecreaseRefCount(id);
            std::cout << "[INFO] Prueba de contador de referencias: ÉXITO. Las operaciones se realizaron sin errores." << std::endl;
        } else {
            std::cout << "[ERROR] Prueba de contador de referencias: FALLO al crear el bloque." << std::endl;
        }
    }, manager);

    // Prueba 6: Simular la recolección de basura
    runTest("Recolección de basura (simulación)", [&](MemoryManager& mm) {
        int id1 = mm.Create(40);
        if (id1 != -1) {
            mm.DecreaseRefCount(id1);
            std::cout << "[INFO] Prueba de recolección de basura: Iniciada. Esperar unos segundos para la posible liberación del bloque " << id1 << "." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(7)); // Esperar más que el intervalo del GC
            if (mm.Get(id1) == nullptr) {
                std::cout << "[INFO] Prueba de recolección de basura: POSIBLE ÉXITO. El bloque " << id1 << " parece haber sido liberado." << std::endl;
            } else {
                std::cout << "[WARNING] Prueba de recolección de basura: POSIBLE FALLO. El bloque " << id1 << " aún existe." << std::endl;
            }
        } else {
            std::cout << "[ERROR] Prueba de recolección de basura: FALLO al crear el bloque." << std::endl;
        }
    }, manager);

    std::cout << "[TEST] Todas las pruebas han finalizado." << std::endl;

    return 0;
}