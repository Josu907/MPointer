#include "MemoryManager.h"
#include "MPointer.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

int main(int argc, char* argv[]) {
    size_t memSizeMB = (argc > 1) ? std::atoi(argv[1]) : 1;
    if (memSizeMB <= 0) memSizeMB = 1;

    std::filesystem::create_directory("dumps");
    std::cout << "[Main] Iniciando prueba básica con " << memSizeMB << "MB...\n";

    MemoryManager memMgr(memSizeMB);
    memMgr.setDumpFolder("dumps");
    MPointer<int>::Init(&memMgr);

    std::cout << "\n[Main] --- MPointer<int> básico ---\n";
    MPointer<int> ptr1 = MPointer<int>::New();
    ptr1.set(42);
    std::cout << "[Main] Valor de ptr1: " << *ptr1 << std::endl;

    MPointer<int> ptr2 = ptr1;
    std::cout << "[Main] Valor de ptr2 (copia): " << *ptr2 << std::endl;

    ptr1.set(99);
    std::cout << "[Main] Nuevo valor en ptr1: " << *ptr1 << std::endl;
    std::cout << "[Main] Valor en ptr2 (debe coincidir): " << *ptr2 << std::endl;

    MPointer<int> ptr3;
    std::cout << "[Main] ptr3.isNull()? " << (ptr3.isNull() ? "sí" : "no") << std::endl;

    std::cout << "\n[Main] Recolección de basura...\n";
    memMgr.collectGarbage();

    std::cout << "\n[Main] Fin de prueba básica.\n";
    return 0;
}
