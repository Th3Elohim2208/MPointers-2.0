#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream> // Added for std::istringstream
#include <vector>
#include <cstring>
#include <mutex>         // Para std::mutex
#include <algorithm>     // Agregado para std::sort
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib") // Vincula la biblioteca Winsock

struct MemoryBlock {
    size_t offset;       // Posición inicial en el bloque de memoria
    size_t size;         // Tamaño del bloque
    std::string type;    // Tipo de dato
    int ref_count;       // Conteo de referencias
    bool is_free;        // Indica si está libre
};

class MemoryManager {
public:
    MemoryManager(size_t size_mb, const std::string& dump_folder)
        : total_size_(size_mb * 1024 * 1024), dump_folder_(dump_folder), next_id_(0) {
        memory_ = static_cast<char*>(malloc(total_size_));
        if (!memory_) { // Validación explícita de malloc
            throw std::runtime_error("Failed to allocate memory: malloc returned nullptr");
        }
        blocks_[next_id_++] = {0, total_size_, "free", 0, true}; // Bloque inicial libre
    }

    ~MemoryManager() {
        if (memory_) { // Asegura que memory_ no sea nullptr antes de liberar
            free(memory_);
        }
    }

    int createBlock(size_t size, const std::string& type) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, block] : blocks_) {
            if (block.is_free && block.size >= size) {
                size_t remaining_size = block.size - size;
                block.size = size;
                block.type = type;
                block.ref_count = 1;
                block.is_free = false;

                if (remaining_size > 0) {
                    int new_id = next_id_++;
                    blocks_[new_id] = {block.offset + size, remaining_size, "free", 0, true};
                }
                dumpMemoryState();
                return id;
            }
        }
        return -1; // No hay espacio suficiente
    }

    bool setValue(int id, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = blocks_.find(id);
        if (it == blocks_.end() || it->second.is_free || value.size() > it->second.size) {
            return false;
        }
        memcpy(memory_ + it->second.offset, value.data(), value.size());
        dumpMemoryState();
        return true;
    }

    std::string getValue(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = blocks_.find(id);
        if (it == blocks_.end() || it->second.is_free) {
            return "";
        }
        return std::string(memory_ + it->second.offset, it->second.size);
    }

    bool increaseRefCount(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = blocks_.find(id);
        if (it == blocks_.end() || it->second.is_free) {
            return false;
        }
        it->second.ref_count++;
        dumpMemoryState();
        return true;
    }

    bool decreaseRefCount(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = blocks_.find(id);
        if (it == blocks_.end() || it->second.is_free) {
            return false;
        }
        it->second.ref_count--;
        dumpMemoryState();
        return true;
    }

    void runGarbageCollector() {
        while (running_) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto it = blocks_.begin(); it != blocks_.end();) {
                    if (!it->second.is_free && it->second.ref_count == 0) {
                        it->second.is_free = true;
                        it->second.type = "free";
                        mergeFreeBlocks();
                        dumpMemoryState();
                    }
                    ++it;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Ejecuta cada 5 segundos
        }
    }

    void startGarbageCollector() {
        running_ = true;
        gc_thread_ = std::thread(&MemoryManager::runGarbageCollector, this);
    }

    void stopGarbageCollector() {
        running_ = false;
        if (gc_thread_.joinable()) {
            gc_thread_.join();
        }
    }

private:
    char* memory_;                          // Bloque de memoria principal
    size_t total_size_;                     // Tamaño total en bytes
    std::string dump_folder_;               // Carpeta para los dumps
    std::map<int, MemoryBlock> blocks_;     // Mapa de bloques
    int next_id_;                           // Próximo ID disponible
    std::mutex mutex_;                      // Protección para acceso concurrente
    std::thread gc_thread_;                 // Hilo del garbage collector
    bool running_;                          // Control del GC

    void mergeFreeBlocks() {
        std::vector<int> sorted_ids;
        for (const auto& [id, block] : blocks_) {
            sorted_ids.push_back(id);
        }
        std::sort(sorted_ids.begin(), sorted_ids.end(),
                  [this](int a, int b) { return blocks_[a].offset < blocks_[b].offset; });

        for (size_t i = 0; i < sorted_ids.size() - 1;) {
            int id1 = sorted_ids[i];
            int id2 = sorted_ids[i + 1];
            auto& block1 = blocks_[id1];
            auto& block2 = blocks_[id2];

            if (block1.is_free && block2.is_free &&
                block1.offset + block1.size == block2.offset) {
                block1.size += block2.size;
                blocks_.erase(id2);
                sorted_ids.erase(sorted_ids.begin() + i + 1);
            } else {
                ++i;
            }
        }
    }

    void dumpMemoryState() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm* ptm = std::localtime(&tt);

        std::ostringstream filename;
        filename << dump_folder_ << "\\" // Barra invertida para Windows
                 << std::put_time(ptm, "%Y%m%d_%H%M%S") << "_" << std::setfill('0') << std::setw(3) << ms.count() << ".txt";

        std::ofstream file(filename.str());
        if (file.is_open()) {
            file << "Memory State:\n";
            for (const auto& [id, block] : blocks_) {
                file << "ID: " << id << ", Offset: " << block.offset << ", Size: " << block.size
                     << ", Type: " << block.type << ", RefCount: " << block.ref_count
                     << ", Free: " << (block.is_free ? "Yes" : "No") << "\n";
            }
            file.close();
        }
    }
};

// Función para manejar cada conexión de cliente
void handleClient(SOCKET client_socket, MemoryManager& manager) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            closesocket(client_socket); // Usar closesocket en lugar de close
            return;
        }

        std::string request(buffer);
        std::istringstream iss(request);
        std::string command;
        iss >> command;

        std::string response;
        if (command == "CREATE") {
            size_t size;
            std::string type;
            iss >> size >> type;
            int id = manager.createBlock(size, type);
            response = (id != -1) ? std::to_string(id) : "ERROR: No memory available";
        } else if (command == "SET") {
            int id;
            std::string value;
            iss >> id;
            std::getline(iss, value);
            value = value.substr(1); // Elimina el espacio inicial
            bool success = manager.setValue(id, value);
            response = success ? "OK" : "ERROR: Invalid ID or size";
        } else if (command == "GET") {
            int id;
            iss >> id;
            std::string value = manager.getValue(id);
            response = value.empty() ? "ERROR: Invalid ID" : value;
        } else if (command == "INC_REF") {
            int id;
            iss >> id;
            bool success = manager.increaseRefCount(id);
            response = success ? "OK" : "ERROR: Invalid ID";
        } else if (command == "DEC_REF") {
            int id;
            iss >> id;
            bool success = manager.decreaseRefCount(id);
            response = success ? "OK" : "ERROR: Invalid ID";
        } else {
            response = "ERROR: Unknown command";
        }

        send(client_socket, response.c_str(), static_cast<int>(response.size()), 0);
    }
}

void RunServer(int port, size_t size_mb, const std::string& dump_folder) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { // Inicialización de Winsock
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return;
    }

    MemoryManager manager(size_mb, dump_folder);
    manager.startGarbageCollector();

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_socket); // Usar closesocket
        WSACleanup();
        return;
    }

    if (listen(server_socket, 10) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_socket); // Usar closesocket
        WSACleanup();
        return;
    }

    std::cout << "Memory Manager listening on port " << port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::thread client_thread(handleClient, client_socket, std::ref(manager));
        client_thread.detach(); // Maneja cada cliente en un hilo separado
    }

    closesocket(server_socket); // Usar closesocket
    WSACleanup(); // Limpieza de Winsock
    manager.stopGarbageCollector();
}

int main(int argc, char* argv[]) {
    if (argc != 7 || std::string(argv[1]) != "--port" || std::string(argv[3]) != "--memsize" || std::string(argv[5]) != "--dumpFolder") {
        std::cerr << "Usage: mem-mgr.exe --port LISTEN_PORT --memsize SIZE_MB --dumpFolder DUMP_FOLDER\n";
        return 1;
    }

    int port = std::stoi(argv[2]);
    std::
    size_t size_mb = std::stoul(argv[4]);
    std::string dump_folder = argv[6];
    std::cout << "\nport:" << port << "\n";
    std::cout << "\nsize:" << port << "\n";
    std::cout << "\ndump folder:" << port << "\n";
    RunServer(port, size_mb, dump_folder);
    return 0;
}