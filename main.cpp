#include "mpointer.h"
#include <iostream>
#include <windows.h> // Para Sleep

int main() {
    // Inicializa MPointer para conectarse al Memory Manager
    std::cout << "Inicializando el sistema de memoria...\n";
    MPointer<int>::Init(50051);

    try {
        // Prueba 1: Crear un MPointer y usar operator*()
        std::cout << "\nPrueba 1: Creando un MPointer y usando operator*()\n";
        MPointer<int> ptr1 = MPointer<int>::New();
        std::cout << "ID de ptr1: " << &ptr1 << "\n"; // Usar operator&()
        ptr1 = 42; // Usar operator=(const T&)
        int value1 = *ptr1; // Usar operator*()
        std::cout << "Valor asignado a ptr1: 42\n";
        std::cout << "Valor obtenido con *ptr1: " << value1 << "\n";
        

        // Prueba 2: Copiar un MPointer con operator=(const MPointer<T>&)
        std::cout << "\nPrueba 2: Copiando un MPointer con operator=(const MPointer<T>&)\n";
        MPointer<int> ptr2;
        std::cout << "ID inicial de ptr2: " << &ptr2 << "\n"; // Debería ser -1
        ptr2 = ptr1; // Usar operator=(const MPointer<T>&)
        std::cout << "ID de ptr2 luego de copiar ptr1: " << &ptr2 << "\n"; // Debería ser igual al ID de ptr1
        int value2 = *ptr2; // Usar operator*()
        std::cout << "Valor obtenido con *ptr2: " << value2 << "\n";

        // Prueba 3: Modificar el valor a través de una copia y verificar en el original
        std::cout << "\nPrueba 3: Modificando el valor a través de una copia\n";
        ptr2 = 100; // Usar operator=(const T&) en ptr2
        int value3 = *ptr1; // Verificar si ptr1 también cambió (debería, porque comparten el mismo ID)
        std::cout << "Nuevo valor asignado a ptr2: 100\n";
        std::cout << "Valor obtenido con *ptr1 luego de modificar ptr2: " << value3 << "\n";

        // Prueba 4: Verificar el conteo de referencias y el Garbage Collector
        std::cout << "\nPrueba 4: Verificando el conteo de referencias y el Garbage Collector\n";
        std::cout << "ID de ptr1 antes de destruir: " << &ptr1 << "\n";
        {
            MPointer<int> ptr3 = ptr1; // Crear una tercera copia
            std::cout << "ID de ptr3: " << &ptr3 << "\n";
            std::cout << "Valor de *ptr3: " << *ptr3 << "\n";
        } // ptr3 se destruye aquí, disminuye el conteo de referencias
        std::cout << "ptr3 destruido. Esperando al Garbage Collector...\n";
        Sleep(6000); // Esperar 6 segundos para que el GC corra
        std::cout << "Intentando acceder a ptr1 luego de esperar al GC...\n";
        try {
            int value4 = *ptr1; // Debería seguir funcionando si el conteo de referencias es > 0
            std::cout << "Valor de *ptr1: " << value4 << "\n";
            std::cout << "Resultado: CORRECTO (el bloque sigue vivo porque ptr1 y ptr2 lo referencian)\n";
        } catch (const std::runtime_error& e) {
            std::cout << "Resultado: ERROR - " << e.what() << "\n";
        }

        // Liberar ptr2 y ptr1 para que el GC pueda limpiar todo
        std::cout << "\nLiberando ptr2 y ptr1...\n";
        ptr2 = MPointer<int>(); // Asignar un MPointer vacío, disminuye el conteo de referencias
        ptr1 = MPointer<int>(); // Asignar un MPointer vacío, debería liberar el bloque
        std::cout << "Esperando al Garbage Collector para liberar el bloque...\n";
        Sleep(6000); // Esperar 6 segundos para que el GC corra

    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nPruebas de operadores de MPointer completadas exitosamente.\n";
    return 0;
}