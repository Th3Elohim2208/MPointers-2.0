#include "mpointer.h"
#include "linkedlist.h"
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

// Variables estáticas para la configuración del servidor
static std::string server_ip_;
static int server_port_;
static bool initialized_ = false;

void MPointerBase::Init(int port, const std::string& ip) {
    server_ip_ = ip;
    server_port_ = port;
    initialized_ = true;
}

std::string MPointerBase::sendCommand(const std::string& command) {
    if (!initialized_) {
        throw std::runtime_error("MPointer not initialized. Call Init() first.");
    }

    // Eliminar mensaje de depuración
    // std::cout << "Enviando comando: " << command << "\n";

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(WSAGetLastError()));
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket: " + std::to_string(WSAGetLastError()));
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_);
    inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(client_socket);
        WSACleanup();
        throw std::runtime_error("Connect failed: " + std::to_string(WSAGetLastError()));
    }

    send(client_socket, command.c_str(), static_cast<int>(command.size()), 0);

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    std::string response;
    if (bytes_received <= 0) {
        closesocket(client_socket);
        WSACleanup();
        throw std::runtime_error("Failed to receive response from server");
    }
    response = std::string(buffer, bytes_received);

    // Eliminar mensaje de depuración
    // std::cout << "Respuesta recibida: " << response << "\n";

    closesocket(client_socket);
    WSACleanup();
    return response;
}

template <typename T>
MPointer<T> MPointer<T>::New() {
    MPointer<T> ptr;
    size_t size = sizeof(T);
    std::string type = typeid(T).name();
    std::string command = "CREATE " + std::to_string(size) + " " + type;
    std::string response = sendCommand(command);

    try {
        ptr.id_ = std::stoi(response);
        // Eliminar mensaje de depuración
        // std::cout << "ID asignado a ptr: " << ptr.id_ << "\n";
    } catch (...) {
        throw std::runtime_error("Failed to create memory block: " + response);
    }
    return ptr;
}

template <typename T>
void MPointer<T>::set(T value) {
    if (id_ == -1) {
        throw std::runtime_error("Invalid MPointer: not initialized");
    }
    std::string value_str;
    if constexpr (std::is_same_v<T, Node<int>>) {
        value_str = std::to_string(value.value) + " " + std::to_string(&value.next);
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        value_str = value;
    }
    else {
        value_str = std::to_string(value);
    }

    std::string command = "SET " + std::to_string(id_) + " " + value_str;
    std::string response = sendCommand(command);
    if (response != "OK") {
        throw std::runtime_error("Failed to set value: " + response);
    }
}

template <typename T>
T MPointer<T>::get() const {
    if (id_ == -1) {
        throw std::runtime_error("Invalid MPointer: not initialized");
    }
    std::string command = "GET " + std::to_string(id_);
    std::string response = sendCommand(command);
    if (response.find("ERROR") == 0) {
        throw std::runtime_error(response);
    }

    T value;
    if constexpr (std::is_same_v<T, Node<int>>) {
        size_t space_pos = response.find(' ');
        if (space_pos == std::string::npos) {
            throw std::runtime_error("Invalid response format for Node<int>");
        }
        std::string value_part = response.substr(0, space_pos);
        std::string next_id_part = response.substr(space_pos + 1);

        value.value = std::stoi(value_part);
        int next_id = std::stoi(next_id_part);
        value.next = MPointer<Node<int>>(next_id);
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        value = response;
    }
    else if constexpr (std::is_same_v<T, int>) {
        value = std::stoi(response);
    }
    else if constexpr (std::is_same_v<T, double>) {
        value = std::stod(response);
    }
    else {
        throw std::runtime_error("Unsupported type for get");
    }
    return value;
}

template <typename T>
T MPointer<T>::operator*() const {
    return get();
}

template <typename T>
MPointer<T>& MPointer<T>::operator=(const T& value) {
    set(value);
    return *this;
}

template <typename T>
MPointer<T>& MPointer<T>::operator=(const MPointer<T>& other) {
    if (id_ != other.id_) {
        if (id_ != -1) {
            sendCommand("DEC_REF " + std::to_string(id_));
        }
        id_ = other.id_;
        if (id_ != -1) {
            sendCommand("INC_REF " + std::to_string(id_));
        }
        // Eliminar mensaje de depuración
        // std::cout << "ID luego de copiar: " << id_ << "\n";
    }
    return *this;
}

template <typename T>
int MPointer<T>::operator&() const {
    return id_;
}

template <typename T>
MPointer<T>::~MPointer() {
    if (id_ != -1) {
        sendCommand("DEC_REF " + std::to_string(id_));
    }
}

// Instanciaciones explícitas de MPointer para los tipos usados
template class MPointer<int>;
template class MPointer<double>;
template class MPointer<std::string>;
template class MPointer<Node<int>>;