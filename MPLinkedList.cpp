#include "MPLinkedList.h"

MPLinkedList::MPLinkedList() {}

void MPLinkedList::insert(int value) {
    // Crear un nuevo nodo
    MPointer<Node> newNode = MPointer<Node>::New();

    // Inicializar el nodo
    Node temp;
    temp.data = value;
    temp.next = head; // Apunta al nodo actual al frente
    newNode.set(temp); // Guardar el nodo en memoria

    // Reasignar head
    head = newNode;

    std::cout << "[MPLinkedList] Insertado " << value << " al frente de la lista\n";
}

void MPLinkedList::print() {
    MPointer<Node> current = head;

    std::cout << "[MPLinkedList] Lista actual: ";
    while (!current.isNull()) { // ✅ más seguro que &current != 0
        Node nodo = *current;
        std::cout << nodo.data << " -> ";
        current = nodo.next;
    }
    std::cout << "NULL\n";
}
