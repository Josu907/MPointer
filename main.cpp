#include "MemoryManager.h"
#include "MPointer.h"
#include "MPLinkedList.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // Leer parámetro de memoria desde línea de comandos
    size_t memSizeMB = 1;
    if (argc > 1) {
        memSizeMB = std::atoi(argv[1]);
        if (memSizeMB <= 0) {
            std::cerr << "[Main] ⚠️ Tamaño inválido. Usando 1MB por defecto.\n";
            memSizeMB = 1;
        }
    }

    std::cout << "[Main] Iniciando MemoryManager con " << memSizeMB << "MB de memoria...\n";
    std::filesystem::create_directory("dumps");

    MemoryManager memMgr(memSizeMB);
    memMgr.setDumpFolder("dumps");
    MPointer<int>::Init(&memMgr);
    MPointer<Node>::Init(&memMgr);

    // Crear y modificar un MPointer
    std::cout << "\n[Main] --- Prueba básica de MPointer<int> ---\n";
    MPointer<int> ptr1 = MPointer<int>::New();
    ptr1.set(42);
    std::cout << "[Main] Valor de ptr1: " << *ptr1 << std::endl;

    // Copiar puntero y verificar refcount
    MPointer<int> ptr2 = ptr1;
    std::cout << "[Main] Valor de ptr2 (copia): " << *ptr2 << std::endl;

    // Asignar nuevo valor al puntero original
    ptr1.set(99);
    std::cout << "[Main] Nuevo valor en ptr1: " << *ptr1 << std::endl;
    std::cout << "[Main] Valor en ptr2 (debe coincidir): " << *ptr2 << std::endl;

    // Verificación de isNull
    MPointer<int> ptr3;
    std::cout << "[Main] ptr3.isNull()? " << (ptr3.isNull() ? "sí" : "no") << std::endl;

    // Recolección manual de basura
    std::cout << "\n[Main] Ejecutando recolección de basura manual...\n";
    memMgr.collectGarbage();

    // Prueba de lista enlazada
    std::cout << "\n[Main] --- Prueba de MPLinkedList ---\n";
    MPLinkedList lista;
    lista.insert(10);
    lista.insert(20);
    lista.insert(30);
    lista.print();

    std::cout << "\n[Main] Finalizando programa...\n";
    return 0;
}
