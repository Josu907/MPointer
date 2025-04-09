#include "MemoryManager.h" // Mantenemos MemoryManager
#include "MPointer.h"    // Mantenemos MPointer
// #include "MPLinkedList.h" // ELIMINADO
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <vector> // Añadido para el ejemplo de fragmentación

int main(int argc, char* argv[]) {
    // Leer parámetro de memoria desde línea de comandos
    size_t memSizeMB = 1; // Tamaño por defecto
    if (argc > 1) {
        try {
            memSizeMB = std::stoul(argv[1]); // Usar stoul para size_t
            if (memSizeMB == 0) {
                 std::cerr << "[Main] ⚠️ Tamaño no puede ser 0. Usando 1MB por defecto.\n";
                 memSizeMB = 1;
            }
        } catch (const std::exception& e) {
             std::cerr << "[Main] ⚠️ Error al leer tamaño: " << e.what() << ". Usando 1MB por defecto.\n";
             memSizeMB = 1;
        }
    }

    std::cout << "[Main] Iniciando MemoryManager con " << memSizeMB << "MB de memoria...\n";
    std::filesystem::create_directory("dumps"); // Asegurarse que el directorio existe

    MemoryManager memMgr(memSizeMB);
    memMgr.setDumpFolder("dumps");

    // Inicializar MPointer SOLO para los tipos que usaremos
    MPointer<int>::Init(&memMgr);
    MPointer<double>::Init(&memMgr); // Ejemplo: si usáramos doubles también
    // MPointer<Node>::Init(&memMgr); // ELIMINADO - Ya no usamos Node

    // --- Prueba básica de MPointer<int> ---
    std::cout << "\n[Main] --- Prueba básica de MPointer<int> ---\n";
    MPointer<int> ptr1 = MPointer<int>::New();
    if (!ptr1.isNull()) { // Buena práctica verificar
        ptr1.set(42);
        std::cout << "[Main] Valor de ptr1: " << *ptr1 << std::endl;

        // Copiar puntero y verificar refcount
        MPointer<int> ptr2 = ptr1; // Llama al constructor copia (incrementa refcount)
        std::cout << "[Main] Valor de ptr2 (copia): " << *ptr2 << std::endl;

        // Asignar nuevo valor al puntero original
        ptr1.set(99);
        std::cout << "[Main] Nuevo valor en ptr1: " << *ptr1 << std::endl;
        std::cout << "[Main] Valor en ptr2 (debe coincidir): " << *ptr2 << std::endl; // Debería ser 99
    } else {
        std::cerr << "[Main] ⚠️ No se pudo crear ptr1." << std::endl;
    }


    // Verificación de isNull
    MPointer<int> ptr3; // Creado por defecto, debe ser null
    std::cout << "[Main] ptr3.isNull()? " << (ptr3.isNull() ? "sí" : "no") << std::endl;

    // --- Simular uso y posible fragmentación ---
    std::cout << "\n[Main] --- Simulación de uso y fragmentación ---\n";
    { // Usamos un bloque para que los punteros se destruyan al salir
        std::vector<MPointer<int>> pointers;
        const int num_pointers = 10;
        const int initial_value = 100;

        std::cout << "[Main] Creando " << num_pointers << " punteros MPointer<int>...\n";
        for (int i = 0; i < num_pointers; ++i) {
            MPointer<int> tempPtr = MPointer<int>::New();
            if (!tempPtr.isNull()) {
                tempPtr.set(initial_value + i);
                pointers.push_back(tempPtr); // Copia el MPointer (incrementa ref count)
            } else {
                 std::cerr << "[Main] ⚠️ Falla al crear MPointer " << i << std::endl;
            }
            // 'tempPtr' se destruye aquí (decrementa ref count del bloque original, pero la copia en 'pointers' lo mantiene vivo)
        }
        std::cout << "[Main] Punteros creados. Tamaño del vector: " << pointers.size() << std::endl;

        // Simular "liberación" de algunos punteros (impares)
         std::cout << "[Main] Liberando punteros con índice impar del vector...\n";
         for(size_t i = 0; i < pointers.size(); ++i) {
             if (i % 2 != 0) {
                 pointers[i] = MPointer<int>(); // Asigna un MPointer nulo (decrementa ref count del bloque)
             }
         }

        // Intentar crear un MPointer más grande (ejemplo con double)
        std::cout << "[Main] Intentando crear un MPointer<double>...\n";
        MPointer<double> doublePtr = MPointer<double>::New();
         if (!doublePtr.isNull()) {
             doublePtr.set(3.14159);
             std::cout << "[Main] MPointer<double> creado con valor: " << *doublePtr << std::endl;
         } else {
              std::cerr << "[Main] ⚠️ Falla al crear MPointer<double>." << std::endl;
         }

        std::cout << "[Main] Saliendo del bloque de simulación (se destruirán los MPointers restantes en el vector)...\n";
        // Al salir de este bloque, el vector 'pointers' se destruye,
        // y los destructores de los MPointer restantes son llamados (decrementando ref counts).
    } // Fin del bloque de simulación

    // Dar tiempo al GC Thread para que actúe (opcional, pero útil para observar)
    std::cout << "\n[Main] Esperando 6 segundos para que el GC actúe...\n";
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // Recolección manual (opcional, ya que el hilo GC debería haber actuado)
    // std::cout << "\n[Main] Ejecutando recolección de basura manual (puede ser redundante)...\n";
    // memMgr.collectGarbage();

    // --- Prueba de Defragmentación ---
    std::cout << "\n[Main] --- Prueba de Defragmentación ---\n";
    std::cout << "[Main] Solicitando defragmentación explícita...\n";
    memMgr.defragment();
    std::cout << "[Main] Defragmentación solicitada.\n";

    // Intentar crear otro MPointer después de defragmentar
    std::cout << "[Main] Intentando crear MPointer<int> post-defragmentación...\n";
    MPointer<int> ptr_after_defrag = MPointer<int>::New();
    if (!ptr_after_defrag.isNull()) {
        ptr_after_defrag.set(111);
         std::cout << "[Main] MPointer<int> post-defragmentación creado con valor: " << *ptr_after_defrag << std::endl;
    } else {
         std::cerr << "[Main] ⚠️ Falla al crear MPointer<int> post-defragmentación." << std::endl;
    }


    // Prueba de lista enlazada (ELIMINADA)
    // std::cout << "\n[Main] --- Prueba de MPLinkedList ---\n";
    // MPLinkedList lista;
    // lista.insert(10);
    // lista.insert(20);
    // lista.insert(30);
    // lista.print();

    std::cout << "\n[Main] Finalizando programa...\n";
    // El destructor de memMgr se llamará aquí automáticamente, deteniendo el hilo GC.
    return 0;
}