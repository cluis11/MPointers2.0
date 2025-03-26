#include <iostream>
#include <fstream>
#include <string>
#include <cstddef> 
#include <thread>
#include <mutex>
#include <vector>
#include <iomanip>
#include <chrono>
//includes para GRPC
#include <grpcpp/grpcpp.h>
#include "../proto/memory_manager.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using memorymanager::MemoryManager;
using memorymanager::CreateRequest;
using memorymanager::CreateResponse;
using memorymanager::SetRequest;
using memorymanager::SetResponse;
using memorymanager::GetRequest;
using memorymanager::GetResponse;
using memorymanager::IncreaseRefCountRequest;
using memorymanager::IncreaseRefCountResponse;
using memorymanager::DecreaseRefCountRequest;
using memorymanager::DecreaseRefCountResponse;
using std::string;
using std::size_t;
using std::cout;

//Estructura para almacenar datos en el nodo del Memory Map
struct MemoryMap
{
    std::mutex blockMutex;
    int id; //identificador del bloque de memoria
    size_t size; //tamaño del bloque de memoria
    string type; //tipo de dato asociado al bloque
    int refCount; //conteo de referencias
    void* ptr; //puntero a la direccion de memoria dentro del bloque de memoria
    bool isNew; //bandera para saber si el bloque esta recien creado

    //constructor de la estructura
    MemoryMap(int id, size_t size, string type, void* ptr)
        : id(id), size(size), type(type), refCount(0), ptr(ptr), isNew(true) {
    }

    //función para imprimir los datos de la estructura
    void print() const {
        cout << "ID: " << id
            << ", Size: " << size
            << ", Type: " << type
            << ", RefCount: " << refCount
            << ", Ptr: " << ptr << std::endl;
    }
};

//Nodo del MemoryBlock
struct Node
{
    MemoryMap block; //bloque de memoria que se va a guardar
    Node* next; //puntero al siguiente nodo de la lista

    //constructuor del nodo
    Node(int id, size_t size, string type, void* ptr)
        : block(id, size, type, ptr), next(nullptr) {
    }
};


//Lista que se unitiliza en el memory map para monitoreas la asignación de memoria
class MemoryList
{
private:
    Node* head;
    std::mutex mtx;

public:
    MemoryList() : head(nullptr) {}

    ~MemoryList() {
        std::lock_guard<std::mutex> lock(mtx);
        Node* current = head;
        while (current != nullptr) {
            Node* nextNode = current->next;
            delete static_cast<char*>(current->block.ptr);
            delete current;
            current = nextNode;
        }
    }

    void insert(int id, size_t size, const string& type, void* ptr) {
        std::lock_guard<std::mutex> lock(mtx);
        Node* newNode = new Node(id, size, type, ptr);
        if (head == nullptr) {
            head = newNode;
        }
        else {
            Node* current = head;
            while (current->next != nullptr) {
                current = current->next;
            }
            current->next = newNode;
        }
    }

    MemoryMap* findById(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        Node* current = head;
        while (current != nullptr) {
            if (current->block.id == id) {
                return &current->block;
            }
            current = current->next;
        }
        return nullptr;
    }

    bool removeById(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        Node* current = head;
        Node* previous = nullptr;
        while (current != nullptr) {
            if (current->block.id == id) {
                if (previous == nullptr) {
                    head = current->next;
                }
                else {
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
        std::lock_guard<std::mutex> lock(mtx);
        Node* current = head;
        while (current != nullptr) {
            current->block.print();
            current = current->next;
        }
    }

    Node* getHead() {
        std::lock_guard<std::mutex> lock(mtx);
        return head;
    }

    void setHead(Node* newHead) {
        std::lock_guard<std::mutex> lock(mtx);
        head = newHead;
    }

    int getSize() {
        int num = 0;
        Node* current = head;
        while (current != nullptr) {
            num++;
            current = current->next;
        }
        return num;
    }
};


class Dumps {
private:
    string dumpFolder;
    MemoryList& memList;

public:
    Dumps(string folder, MemoryList& list) : dumpFolder(folder), memList(list) {}

    void CreateDump() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y%m%dT%H%M%S")
            << std::setw(3) << std::setfill('0') << ms.count();


        std::string datetime = oss.str();
        std::string fileName = dumpFolder + "/" + datetime + ".txt";

        std::fstream archive;
        archive.open(fileName, std::ios::out);

        if (!archive.is_open()) {
            std::cerr << "Error al abrir el arhcivo del dump: " << fileName << std::endl;
            return;
        }

        Node* current = memList.getHead(); // Obtener la cabeza de la lista

        while (current != nullptr) {
            archive << "ID: " << current->block.id
                << ", Size: " << current->block.size
                << ", Type: " << current->block.type
                << ", RefCount: " << current->block.refCount
                << ", Ptr: " << current->block.ptr;

            // Escribir el valor almacenado en la memoria si es un tipo primitivo
            if (current->block.type == "int") {
                archive << ", Value: " << *static_cast<int*>(current->block.ptr);
            }
            else if (current->block.type == "float") {
                archive << ", Value: " << *static_cast<float*>(current->block.ptr);
            }
            else if (current->block.type == "string") {
                archive << ", Value: " << *static_cast<std::string*>(current->block.ptr);
            }
            else {
                archive << ", Value: [No serializable]";
            }

            archive << std::endl;
            current = current->next;
        }

        archive.close();
        std::cout << "Dump creado: " << fileName << std::endl;
    }
};


class MemoryBlock {
private:
    void* memBlock;
    size_t memSize;
    size_t usedMem;
    MemoryList memList;
    Dumps dumps;
    int nextId = 0;

public:
    MemoryBlock(size_t sizeMB, const string& dumpFolder)
        : memSize(sizeMB * 1024 * 1024), memList(), dumps(dumpFolder, memList) {
        memBlock = malloc(memSize);
        if (!memBlock) {
            cout << "Error al reservar la memoria";
        }
        usedMem = 0;
    }

    ~MemoryBlock() {
        free(memBlock);
    }

    int Create(size_t size, const string& type) {
        if (usedMem + size > memSize) {
            throw std::runtime_error("No hay espacio suficiente");
        }

        if (type == "int" && size != sizeof(int)) {
            throw std::runtime_error("La memoria dada no coincide con el tipo de dato 'int'");
        }
        else if (type == "float" && size != sizeof(float)) {
            throw std::runtime_error("La memoria dada no coincide con el tipo de dato 'float'");
        }
        else if (type == "string" && size != sizeof(string)) {
            throw std::runtime_error("La memoria dada no coincide con el tipo de dato 'string'");
        }

        void* addr = static_cast<char*>(memBlock) + usedMem;

        memList.insert(nextId, size, type, addr);
        usedMem += size;
        return nextId++;
    }

    template <typename T>
    void Set(int id, const T& value) {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            throw std::runtime_error("Bloque de memoria no se ha encontrado");
        }
        if (sizeof(T) > block->size) {
            throw std::runtime_error("El valor es más grande que el bloque de memoria");
        }

        std::lock_guard<std::mutex> lock(block->blockMutex);
        *static_cast<T*>(block->ptr) = value;
        block->refCount = 1;
        block->isNew = false;
        dumps.CreateDump();
    }


    template <typename T>
    T Get(int id) {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            throw std::runtime_error("Bloque de memoria no encontrado");
        }
        return *static_cast<T*>(block->ptr);
    }

    void IncreaseRefCount(int id) {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            throw std::runtime_error("Bloque de memoria no encontrado");
        }
        block->refCount++;
    }

    void DecreaseRefCount(int id) {
        MemoryMap* block = memList.findById(id);
        if (!block) {
            throw std::runtime_error("Bloque de memoria no encontrado");
        }
        if (--block->refCount < 0) {
            block->refCount = 0;
        }
    }

    MemoryMap* GetMemoryMapById(int id) {
        return memList.findById(id);
    }

    void PrintMemoryList() {
        cout << "Contenido de la lista de MemoryBlock:" << std::endl;
        memList.printList();
    }

    MemoryList& GetMemoryList() {
        return memList;
    }

};



class GarbageCollector {
private:
    MemoryList& memList;
    std::atomic<bool> running;

public:
    GarbageCollector(MemoryList& memlist) : memList(memlist), running(true) {}

    void Stop() {
        running = false;
    }

    void Search() {
        cout << "Iniciando Garbage Collector..." << std::endl;

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            Node* current = memList.getHead();
            Node* previus = nullptr;

            while (current != nullptr) {
                if (current->block.refCount == 0) {
                    cout << "Eliminando bloque de memoria con ID: " << current->block.id << std::endl;

                    Node* toDelete = current;
                    current = current->next;

                    if (previus) {
                        previus->next = current;
                    }
                    else
                    {
                        memList.setHead(current);
                    }
                    delete static_cast<char*>(toDelete->block.ptr);
                    delete toDelete;
                }
                else {
                    previus = current;
                    current = current->next;
                }
            }
        }
    }
};


class MemoryManagerServiceImpl final : public MemoryManager::Service {
public:
    MemoryManagerServiceImpl(MemoryBlock& memoryBlock) : memoryBlock_(memoryBlock) {}

    Status Create(ServerContext* context, const CreateRequest* request, CreateResponse* response) override {
        try {
            int id = memoryBlock_.Create(request->size(), request->type());
            response->set_success(true);
            response->set_id(id);
        }
        catch (const std::exception& e) {
            response->set_success(false);
        }
        return Status::OK;
    }

    Status Set(ServerContext* context, const SetRequest* request, SetResponse* response) override {
        try {
            if (request->has_int_value()) {
                memoryBlock_.Set<int>(request->id(), request->int_value());
            }
            else if (request->has_float_value()) {
                memoryBlock_.Set<float>(request->id(), request->float_value());
            }
            else if (request->has_string_value()) {
                memoryBlock_.Set<std::string>(request->id(), request->string_value());
            }
            response->set_success(true);
        }
        catch (const std::exception& e) {
            response->set_success(false);
        }
        return Status::OK;
    }

    Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
        try {
            MemoryMap* block = memoryBlock_.GetMemoryMapById(request->id());
            if (!block) {
                response->set_success(false);
                return Status::OK;
            }

            response->set_success(true);
            if (block->type == "int") {
                response->set_int_value(memoryBlock_.Get<int>(request->id()));
            }
            else if (block->type == "float") {
                response->set_float_value(memoryBlock_.Get<float>(request->id()));
            }
            else if (block->type == "string") {
                response->set_string_value(memoryBlock_.Get<std::string>(request->id()));
            }
        }
        catch (const std::exception& e) {
            response->set_success(false);
        }
        return Status::OK;
    }

    Status IncreaseRefCount(ServerContext* context, const IncreaseRefCountRequest* request, IncreaseRefCountResponse* response) override {
        try {
            memoryBlock_.IncreaseRefCount(request->id());
            response->set_success(true);
        }
        catch (const std::exception& e) {
            response->set_success(false);
        }
        return Status::OK;
    }

    Status DecreaseRefCount(ServerContext* context, const DecreaseRefCountRequest* request, DecreaseRefCountResponse* response) override {
        try {
            memoryBlock_.DecreaseRefCount(request->id());
            response->set_success(true);
        }
        catch (const std::exception& e) {
            response->set_success(false);
        }
        return Status::OK;
    }

private:
    MemoryBlock& memoryBlock_;
};

void RunServer(const std::string& listenPort, size_t memSize, std::string folder) {
    MemoryBlock memoryBlock(memSize, folder); // Inicializa el MemoryBlock con el tamaño especificado

    GarbageCollector gc(memoryBlock.GetMemoryList());
    std::thread gcTh([&gc]() { gc.Search(); });

    MemoryManagerServiceImpl service(memoryBlock);

    ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:" + listenPort, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Servidor escuchando en el puerto " << listenPort << std::endl;

    server->Wait();
    gc.Stop();
    gcTh.join();
}



int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "Uso: " << argv[0] << " --port LISTEN_PORT --memsize SIZE_MB --dumpFolder DUMP_FOLDER" << std::endl;
        return 1;
    }

    std::string listenPort;
    size_t memSize = 0;
    std::string dumpFolder;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            listenPort = argv[++i];
        }
        else if (arg == "--memsize" && i + 1 < argc) {
            memSize = std::stoul(argv[++i]);
        }
        else if (arg == "--dumpFolder" && i + 1 < argc) {
            dumpFolder = argv[++i];
        }
        else {
            std::cerr << "Argumento desconocido o valor faltante: " << arg << std::endl;
            return 1;
        }
    }

    if (listenPort.empty() || memSize == 0 || dumpFolder.empty()) {
        std::cerr << "Faltan parámetros requeridos." << std::endl;
        return 1;
    }

    RunServer(listenPort, memSize, dumpFolder);
    return 0;
}