#include "linkedlist.h"
#include <iostream>
#include <windows.h> // Para Sleep

int main() {
    // Inicializa MPointer para conectarse al Memory Manager
    std::cout << "Inicializando el sistema de memoria...\n";
    MPointer<int>::Init(50051);
    MPointer<Node<int>>::Init(50051);

    try {
        // Crea una lista enlazada
        std::cout << "Creando una lista enlazada...\n";
        LinkedList<int> list;

        // Agrega elementos
        std::cout << "Agregando los elementos 3, 2, 1 a la lista...\n";
        list.push(1);
        list.push(2);
        list.push(3);

        // Imprime la lista
        std::cout << "Lista actual: ";
        list.print();

        // Elimina un elemento
        std::cout << "Eliminando el primer elemento...\n";
        int removed = list.pop();
        std::cout << "Elemento eliminado: " << removed << "\n";

        // Imprime la lista nuevamente
        std::cout << "Lista luego de eliminar el primer elemento: ";
        list.print();

        // Esperar para ver el comportamiento del Garbage Collector
        std::cout << "Esperando 5 segundos para que el Garbage Collector libere la memoria...\n";
        Sleep(6000); // 6 segundos para asegurar que el GC corra

    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Pruebas completadas exitosamente.\n";
    return 0;
}