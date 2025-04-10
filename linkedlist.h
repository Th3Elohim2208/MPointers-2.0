#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "mpointer.h"
#include <iostream>

template <typename T>
struct Node {
    T value;
    MPointer<Node<T>> next;

    Node() : value(T()), next() {}
    Node(const T& val) : value(val), next() {}
};

template <typename T>
class LinkedList {
private:
    MPointer<Node<T>> head;

public:
    LinkedList() : head() {}

    // Agrega un nodo al inicio de la lista
    void push(const T& value) {
        MPointer<Node<T>> newNode = MPointer<Node<T>>::New();
        Node<T> node(value);
        node.next = head;
        newNode.set(node); // Usar set en lugar de *newNode = node
        head = newNode;
    }

    // Elimina el primer nodo y retorna su valor
    T pop() {
        if (&head == -1) {
            throw std::runtime_error("List is empty");
        }

        MPointer<Node<T>> temp = head;
        Node<T> node = temp.get(); // Usar get en lugar de *temp
        head = node.next;
        return node.value;
    }

    // Imprime la lista
    void print() const {
        MPointer<Node<T>> current = head;
        while (&current != -1) { // Corregir "←ft" a "&current"
            Node<T> node = current.get(); // Usar get en lugar de *current
            std::cout << node.value << " -> ";
            current = node.next;
        }
        std::cout << "nullptr" << std::endl;
    }
};

#endif // LINKEDLIST_H