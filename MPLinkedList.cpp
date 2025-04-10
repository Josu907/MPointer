#include "MPLinkedList.h"

MPLinkedList::MPLinkedList() : head() {}

void MPLinkedList::insert(int value) {
    MPointer<Node> newNode = MPointer<Node>::New();

    Node temp;
    temp.data = value;
    temp.next = head;  // Conserva la cabeza actual

    newNode.set(temp);
    head = newNode;

    std::cout << "[MPLinkedList] Insertado " << value << " al frente de la lista\n";
}

void MPLinkedList::print() {
    MPointer<Node> current = head;
    int contador = 0;

    std::cout << "[MPLinkedList] Lista actual: ";
    while (!current.isNull()) {
        if (contador++ > 100) {
            std::cout << "\n⚠️ [MPLinkedList] Límite de nodos excedido. Posible ciclo o corrupción.\n";
            break;
        }

        try {
            Node nodo = *current;
            std::cout << nodo.data << " -> ";

            // ✅ Asignar solo si ID cambia, para evitar liberar referencias aún activas
            if ((&current != &nodo.next) && ((&current) != (&nodo.next))) {
                current = nodo.next;
            } else {
                std::cout << "\n❌ [MPLinkedList] Nodo apunta a sí mismo. Fin anticipado para evitar bucle.\n";
                break;
            }
            

        } catch (const std::exception& e) {
            std::cout << "\n❌ [MPLinkedList] Error al acceder a nodo: " << e.what() << "\n";
            break;
        }
    }
    std::cout << "NULL\n";
}
