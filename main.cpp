#include "mpointer.h"
#include <iostream>
#include <string>
#include <windows.h> // Para Sleep

int main() {
    // Inicializa MPointer para conectarse al Memory Manager
    MPointer<int>::Init(50051);
    MPointer<std::string>::Init(50051);

    try {
        // Prueba 1: Crear un MPointer<int> y asignar un valor
        std::cout << "Creando MPointer<int>...\n";
        MPointer<int> ptr1 = MPointer<int>::New();
        std::cout << "ID de ptr1 después de New(): " << &ptr1 << "\n"; // Depuración adicional
        ptr1.set(42); // Usar set en lugar de *ptr1 = 42
        std::cout << "ptr1 value: " << ptr1.get() << " (ID: " << &ptr1 << ")\n"; // Usar get en lugar de *ptr1

        // Prueba 2: Copiar el MPointer y verificar el conteo de referencias
        std::cout << "Copiando ptr1 a ptr2...\n";
        MPointer<int> ptr2 = ptr1;
        std::cout << "ptr2 value: " << ptr2.get() << " (ID: " << &ptr2 << ")\n";

        // Prueba 3: Usar set y get directamente
        std::cout << "Usando set y get...\n";
        ptr2.set(100);
        std::cout << "ptr1 value after set on ptr2: " << ptr1.get() << "\n";

        // Prueba 4: Crear un MPointer<std::string>
        std::cout << "Creando MPointer<std::string>...\n";
        MPointer<std::string> ptr3 = MPointer<std::string>::New();
        ptr3.set("Hello, World!"); // Usar set
        std::cout << "ptr3 value: " << ptr3.get() << " (ID: " << &ptr3 << ")\n";

        // Prueba 5: Destrucción y Garbage Collector
        std::cout << "Destruyendo ptr2 (should decrease ref count)...\n";
        ptr2 = MPointer<int>(); // Asigna un MPointer vacío para simular destrucción
        std::cout << "ptr1 value after ptr2 destruction: " << ptr1.get() << "\n";

        std::cout << "Destruyendo ptr1 (should trigger Garbage Collector)...\n";
        ptr1 = MPointer<int>(); // Esto debería liberar el bloque si el ref count llega a 0

        // Esperar un poco para que el Garbage Collector actúe (5 segundos según el Memory Manager)
        std::cout << "Esperando al Garbage Collector (5 segundos)...\n";
        Sleep(6000); // 6 segundos para asegurar que el GC corra

        // Intentar acceder al bloque después de que el GC lo haya liberado debería fallar
        try {
            std::cout << "Intentando acceder a ptr1 después de GC...\n";
            std::cout << ptr1.get() << "\n";
        } catch (const std::runtime_error& e) {
            std::cout << "Error esperado: " << e.what() << "\n";
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Pruebas completadas.\n";
    return 0;
}