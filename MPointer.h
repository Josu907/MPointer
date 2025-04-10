#ifndef MPOINTER_H
#define MPOINTER_H

#include "MemoryManager.h"
#include <iostream>
#include <memory>

#define BLUE    "\033[34m"
#define RESET   "\033[0m"

template <typename T>
class MPointer {
private:
    static MemoryManager* manager;
    int id;

public:
    MPointer();
    MPointer(const MPointer<T>& other);
    ~MPointer();

    static void Init(MemoryManager* memMgr);
    static MPointer<T> New();

    MPointer<T>& operator=(const MPointer<T>& other);
    T operator*() const;
    void set(const T& value);
    int operator&() const;
    bool isNull() const;
};

template <typename T>
MemoryManager* MPointer<T>::manager = nullptr;

template <typename T>
void MPointer<T>::Init(MemoryManager* memMgr) {
    manager = memMgr;
    std::cout << BLUE << "[MPointer] Inicializado con MemoryManager" << RESET << std::endl;
}

template <typename T>
MPointer<T>::MPointer() : id(0) {}

template <typename T>
MPointer<T>::MPointer(const MPointer<T>& other) {
    this->id = other.id;
    if (manager && id != 0) {
        manager->increaseRefCount(id);
        std::cout << BLUE << "[MPointer] Constructor copia llamado, ID: " << id << RESET << std::endl;
    }
}

template <typename T>
MPointer<T> MPointer<T>::New() {
    if (!manager)
        throw std::runtime_error("MemoryManager no inicializado");

    MPointer<T> ptr;
    ptr.id = manager->create(sizeof(T));
    std::cout << BLUE << "[MPointer] Creado con ID " << ptr.id << RESET << std::endl;
    return ptr;
}

template <typename T>
MPointer<T>::~MPointer() {
    if (id != 0 && manager) {
        manager->decreaseRefCount(id);
        std::cout << BLUE << "[MPointer] Destructor llamado, ID: " << id << RESET << std::endl;
    }
}

template <typename T>
MPointer<T>& MPointer<T>::operator=(const MPointer<T>& other) {
    if (this->id != other.id && other.id != 0) {
        if (this->id != 0 && manager) {
            manager->decreaseRefCount(this->id);
            std::cout << BLUE << "[MPointer] Operador = liberó ID previo: " << this->id << RESET << std::endl;
        }

        this->id = other.id;
        manager->increaseRefCount(this->id);
        std::cout << BLUE << "[MPointer] Operador = copió ID " << this->id << RESET << std::endl;
    }
    return *this;
}

template <typename T>
T MPointer<T>::operator*() const {
    T value;
    manager->get(id, &value, sizeof(T));
    std::cout << BLUE << "[MPointer] Operador * leyó valor de ID " << id << RESET << std::endl;
    return value;
}

template <typename T>
void MPointer<T>::set(const T& value) {
    manager->set(id, &value, sizeof(T));
    std::cout << BLUE << "[MPointer] Método set() guardó valor en ID " << id << RESET << std::endl;
}

template <typename T>
int MPointer<T>::operator&() const {
    return id;
}

template <typename T>
bool MPointer<T>::isNull() const {
    return id == 0;
}

#endif
