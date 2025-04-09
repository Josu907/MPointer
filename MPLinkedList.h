#ifndef MP_LINKED_LIST_H
#define MP_LINKED_LIST_H

#include "MPointer.h"
#include <iostream>

struct Node {
    int data;
    MPointer<Node> next;
};

class MPLinkedList {
private:
    MPointer<Node> head;

public:
    MPLinkedList();

    void insert(int value);
    void print();
};

#endif
