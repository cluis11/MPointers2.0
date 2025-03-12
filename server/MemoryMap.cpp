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

        MemoryMap* findById(int id){
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

        void printList() {
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
        block->refCount = 1;  
        block->isNew = false;  
    }


    template <typename T>
    T Get(int id) {
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

    void DecreaseRefCount(int id) {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            cout << "Bloque de memoria no encontrado";
        }
        --block->refCount;
    }

    void PrintMemoryList() {
        cout << "Contenido de la lista de MemoryBlock:" << std::endl;
        memList.printList();
    }

};

int main() {
    // Crear un bloque de memoria de 10 MB
    MemoryBlock memoryBlock(10);

    // Prueba de Create
    int intId = memoryBlock.Create(sizeof(int), "int");
    if (intId != -1) {
        std::cout << "Bloque de memoria para entero creado con ID: " << intId << std::endl;
    }

    int floatId = memoryBlock.Create(sizeof(float), "float");
    if (floatId != -1) {
        std::cout << "Bloque de memoria para float creado con ID: " << floatId << std::endl;
    }

    int stringId = memoryBlock.Create(sizeof(std::string), "string");
    if (stringId != -1) {
        std::cout << "Bloque de memoria para string creado con ID: " << stringId << std::endl;
    }

    // Imprimir la lista después de crear los bloques
    std::cout << "\nEstado de la lista después de Create:" << std::endl;
    memoryBlock.PrintMemoryList();

    // Prueba de Set
    if (intId != -1) {
        memoryBlock.Set(intId, 42);
        std::cout << "\nValor del bloque de memoria para entero (ID " << intId << "): "
                  << memoryBlock.Get<int>(intId) << std::endl;
    }

    if (floatId != -1) {
        memoryBlock.Set(floatId, 3.14f);
        std::cout << "Valor del bloque de memoria para float (ID " << floatId << "): "
                  << memoryBlock.Get<float>(floatId) << std::endl;
    }

    if (stringId != -1) {
        memoryBlock.Set(stringId, std::string("Hola, mundo!"));
        std::cout << "Valor del bloque de memoria para string (ID " << stringId << "): "
                  << memoryBlock.Get<std::string>(stringId) << std::endl;
    }

    // Imprimir la lista después de Set
    std::cout << "\nEstado de la lista después de Set:" << std::endl;
    memoryBlock.PrintMemoryList();

    // Prueba de IncreaseRefCount y DecreaseRefCount
    if (intId != -1) {
        memoryBlock.IncreaseRefCount(intId);
        memoryBlock.IncreaseRefCount(intId);
        std::cout << "\nRefCount del bloque de memoria para entero (ID " << intId << ") incrementado dos veces." << std::endl;
    }

    if (floatId != -1) {
        memoryBlock.DecreaseRefCount(floatId);
        std::cout << "RefCount del bloque de memoria para float (ID " << floatId << ") decrementado una vez." << std::endl;
    }

    // Imprimir la lista después de modificar RefCount
    std::cout << "\nEstado de la lista después de modificar RefCount:" << std::endl;
    memoryBlock.PrintMemoryList();

    return 0;
}