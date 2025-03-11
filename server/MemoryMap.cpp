#include <iostream>
#include <string>
#include <cstddef> 
using std::string;
using std::size_t;
using std::cout;

struct MemoryMap
{
    int id;
    size_t size;
    string type;
    int refCount;
    void* ptr;
    bool isNew;

    MemoryMap(int id, size_t size, string type, void* ptr)
        : id(id), size(size), type(type), refCount(0), ptr(ptr), isNew(true) {}

    void print() const {
        cout << "ID: " << id
            << ", Size: " << size
            << ", Type: " << type
            << ", RefCount: " << refCount
            << ", Ptr: " << ptr << std::endl;
    }
};

struct Node
{
    MemoryMap block;
    Node* next;

    Node(int id, size_t size, string type, void* ptr)
        : block(id, size, type, ptr), next(nullptr) {}
};

class MemoryList
{
    private:
        Node* head;
    
    public:
        MemoryList(): head(nullptr) {}

        ~MemoryList() {
            Node* current = head;
            while (current != nullptr){
                Node* nextNode = current->next;
                delete static_cast<char*>(current->block.ptr);
                delete current;
                current = nextNode;
            }
        }

        void insert(int id, size_t size, const string& type, void* ptr) {
            Node* newNode = new Node(id, size, type, ptr);
            if (head == nullptr){
                head = newNode;
            }
            else{
                Node* current = head;
                while (current->next != nullptr){
                    current = current->next;
                }
                current->next = newNode;
            }
        }

        MemoryMap* findById(int id) {
            Node* current = head;
            while (current != nullptr){
                if (current->block.id == id){
                    return &current->block;
                }
                current = current->next;
            }
            return nullptr;
        }

        bool removeById(int id) {
            Node* current = head;
            Node* previous = nullptr;
            while (current != nullptr){
                if (current->block.id == id){
                    if (previous == nullptr){
                        head = current->next;
                    }
                    else{
                        previous->next = current->next;
                    }
                    delete static_cast<char*>(current->block.ptr);
                    delete current;
                    return true;
                }
                previous = current;
                current = current->next;
            }
            return false;
        }

        void printList() const {
            Node* current = head;
            while (current != nullptr) {
                current->block.print(); 
                current = current->next; 
            }
        }
};

class MemoryBlock {
    private:
        void* memBlock;
        size_t memSize;
        size_t usedMem;
        MemoryList memList;
        int nextId;

    public:
        MemoryBlock(size_t sizeMB){
            memSize = sizeMB * 1024 * 1024;
            memBlock = malloc(memSize);
            if (!memBlock) {
                cout << "Error al reservar la memoria";
            }
            usedMem = 0;
        }

        ~MemoryBlock(){
            free(memBlock);
        }

        int Create(size_t size, const string& type) {
            if (usedMem + size > memSize){
                cout << "No hay espacio suficiente";
                return -1;
            }

            if (type == "int" && size != sizeof(int)){
                cout << "La memoria dada no coincide con el tipo de dato";
                return -1;
            }
            else if (type == "float" && size != sizeof(float)){
                cout << "La memoria dada no coincide con el tipo de dato";
                return -1;
            }
            else if (type == "string" && size != sizeof(string)){
                cout << "La memoria dada no coincide con el tipo de dato";
                return -1;
            }
            
            void* addr = static_cast<char*>(memBlock) + usedMem; // cambiar para casos de fragmentación

            memList.insert(nextId, size, type, addr); 
            usedMem += size; 
            return nextId++;
        }

        template <typename T>
        void Set(int id, const T& value) {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            cout << "Bloque de memoria no se ha encontrado";
        }
        if (sizeof(T) > block->size) {
            cout << "El valor es más grande que el bloque de memoria";
        }

        *static_cast<T*>(block->ptr) = value;
        block->refCount = 1;  // Actualizar refCount
        block->isNew = false;  // Ya no es nuevo
    }


    template <typename T>
    T Get(int id) const {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            cout << "Bloque de memoria no encontrado";
        }
        return *static_cast<T*>(block->ptr);
    }

    void IncreaseRefCount(int id) {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            cout << "Bloque de memoria no encontrado";
        }

        block->refCount++;
    }


};

int main() {
    // Crear una lista de bloques de memoria
    MemoryList list;

    // Crear algunos bloques de memoria y agregarlos a la lista
    void* ptr1 = new char[100]; // Bloque de memoria de 100 bytes
    list.insert(1, 100, "Type1", ptr1);

    void* ptr2 = new char[200]; // Bloque de memoria de 200 bytes
    list.insert(2, 200, "Type2", ptr2);

    void* ptr3 = new char[300]; // Bloque de memoria de 300 bytes
    list.insert(3, 300, "Type3", ptr3);

    std::cout << "Lista despues de insertar bloques:" << std::endl;
    list.printList();

    // Buscar un bloque de memoria por su id
    std::cout << "\nBuscando el bloque con ID 2:" << std::endl;
    MemoryMap* block = list.findById(2);
    if (block) {
        block->print();
    } else {
        std::cout << "Bloque con ID 2 no encontrado." << std::endl;
    }

    // Eliminar un bloque de memoria por su id
    std::cout << "\nEliminando el bloque con ID 1:" << std::endl;
    if (list.removeById(2)) {
        std::cout << "Bloque con ID 1 eliminado." << std::endl;
    } else {
        std::cout << "Bloque con ID 1 no encontrado." << std::endl;
    }

    // Imprimir la lista después de eliminar un bloque
    std::cout << "\nLista después de eliminar el bloque con ID 1:" << std::endl;
    list.printList();

    return 0;
}