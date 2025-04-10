#ifndef MPOINTER_H
#define MPOINTER_H

#include <string>

class MPointerBase {
protected:
public:
    static void Init(int port, const std::string& ip = "127.0.0.1");
    static std::string sendCommand(const std::string& command);
};

template <typename T>
class MPointer : public MPointerBase {
private:
    int id_;

public:
    MPointer() : id_(-1) {}          // Constructor por defecto
    MPointer(int id) : id_(id) {}    // Constructor con ID
    static MPointer New();
    ~MPointer();

    void set(T value);
    T get() const;

    T operator*() const;
    MPointer<T>& operator=(const T& value);
    MPointer<T>& operator=(const MPointer<T>& other);
    int operator&() const;
};

#endif // MPOINTER_H