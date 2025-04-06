#include <iostream>
#include <string>
#include <cstddef> 

using std::string;
using std::size_t;
using std::cout;

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

//Estructura para almacenar datos en el nodo del Memory Map
struct MemoryMap
{
    int id; //identificador del bloque de memoria
    size_t size; //tamaño del bloque de memoria
    string type; //tipo de dato asociado al bloque
    int refCount; //conteo de referencias
    void* ptr; //puntero a la direccion de memoria dentro del bloque de memoria
    bool isNew; //bandera para saber si el bloque esta recien creado

    //constructor de la estructura
    MemoryMap(int id, size_t size, string type, void* ptr)
        : id(id), size(size), type(type), refCount(0), ptr(ptr), isNew(true) {}

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
        : block(id, size, type, ptr), next(nullptr) {}
};


//Lista que se unitiliza en el memory map para monitoreas la asignación de memoria
class MemoryList
{
    private:
        Node* head;
        Node* last;
    
    public:
        MemoryList(): head(nullptr), last(nullptr) {}

        ~MemoryList() {
            Node* current = head;
            while (current != nullptr){
                Node* nextNode = current->next;
                delete static_cast<char*>(current->block.ptr);
                delete current;
                current = nextNode;
            }
        }

        Node* getHead() const { return head; }
        void updateHead(Node* newHead) { head = newHead; }
        Node* getLast() const { return last; }

        void insert(int id, size_t size, const string& type, void* ptr) {
            Node* newNode = new Node(id, size, type, ptr);
            if (!head) {
                head = last = newNode;
            } else {
                last->next = newNode;
                last = newNode;
            }
        }

        bool insertNextTo(int id, int newId, size_t size, const string& type, void* ptr) {
            Node* current = head;
            while (current) {
                if (current->block.id == id) {
                    // Crear nuevo nodo con el espacio sobrante
                    Node* newNode = new Node(newId, size, type, ptr);
                    newNode->next = current->next;
                    current->next = newNode;
                    return true;
                }
                current = current->next;
            }
            return false; // No se encontró el id
        }

        MemoryMap* findById(int id) {
            for (Node* current = head; current != nullptr; current = current->next) {
                if (current->block.id == id) {
                    return &current->block;
                }
            }
            return nullptr;
        }

        /*bool removeById(int id) {
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
        }*/

        bool removeById(int id) {
            Node* current = head;
            Node* prev = nullptr;
            while (current != nullptr){
                if (current->block.id == id){
                    if (current == last) {          // Línea añadida
                        last = prev;                // Línea añadida
                    }
                    std::memset(current->block.ptr, 0, current->block.size);
                    current->block.isNew = true;
                    current->block.type = "";
                    return true;
                }
                prev = current;
                current = current->next;
            }
            return false;
        }

        int reuseFreeBlock(size_t size, const string& newType, int newId) {
            std::cout << "[MemoryList] Buscando bloque libre para reutilizar. Tamaño requerido: " 
                      << size << ", Tipo: " << newType << std::endl;
            
            Node* current = head;
            while (current) {
                if (current->block.type == "" && current->block.size >= size) {
                    std::cout << "[MemoryList] Bloque libre encontrado - ID: " << current->block.id
                              << ", Tamaño: " << current->block.size << std::endl;
                    
                    // Tamaño exacto
                    if (current->block.size == size) {
                        std::cout << "[MemoryList] Reutilizando bloque completo - ID: " 
                                  << current->block.id << std::endl;
                        current->block.type = newType;
                        return current->block.id;
                    }
                    // Bloque más grande (dividirlo)
                    else {
                        std::cout << "[MemoryList] Dividiendo bloque - ID original: " 
                                  << current->block.id << ", Tamaño original: " << current->block.size << std::endl;
                        
                        void* blockAddr = current->block.ptr;
                        size_t remainingSize = current->block.size - size;
                        void* remainingAddr = static_cast<char*>(blockAddr) + size;
                        
                        // Actualizar bloque actual
                        current->block.size = size;
                        current->block.type = newType;
                        
                        std::cout << "[MemoryList] Bloque actualizado - Nuevo tamaño: " 
                                  << current->block.size << ", Nuevo tipo: " << current->block.type << std::endl;
                        std::cout << "[MemoryList] Creando nuevo bloque libre - Tamaño: " 
                                  << remainingSize << ", Dirección: " << remainingAddr << std::endl;
                        
                        // Insertar espacio sobrante como nuevo nodo libre
                        insertNextTo(current->block.id, newId, remainingSize, "", remainingAddr);
                        return current->block.id;
                    }
                }
                current = current->next;
            }
            
            std::cout << "[MemoryList] No se encontraron bloques libres adecuados" << std::endl;
            return -1;
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
        size_t memAtEnd;
        MemoryList memList;
        int nextId=0;

        void* GetLastFreeAddr() {
            Node* lastNode = memList.getLast();
            if (!lastNode) {
                memAtEnd = memSize;
                return memBlock;
            }
        
            // Cálculo seguro (cambios aquí)
            char* lastBlockEnd = static_cast<char*>(lastNode->block.ptr) + lastNode->block.size;
            char* totalEnd = static_cast<char*>(memBlock) + memSize;
            memAtEnd = (lastBlockEnd >= totalEnd) ? 0 : totalEnd - lastBlockEnd;
            
            return (memAtEnd > 0) ? lastBlockEnd : nullptr;
        }

    public:
        MemoryBlock(size_t sizeMB){
            memSize = sizeMB * 1024 * 1024;
            memBlock = malloc(memSize);
            if (!memBlock) {
                cout << "Error al reservar la memoria";
            }
            usedMem = 0;
            memAtEnd = memSize;
        }

        ~MemoryBlock(){
            free(memBlock);
        }

        int Create(size_t size, const string& type) {
            std::cout << "[MemoryBlock] Creando bloque - Tipo: " << type << ", Tamaño: " << size << std::endl;
            
            // Validación de memoria total
            if (usedMem + size > memSize) {
                std::cout << "[MemoryBlock] ERROR: No hay espacio suficiente. Usado: " 
                          << usedMem << "/" << memSize << std::endl;
                throw std::runtime_error("No hay espacio suficiente");
            }
        
            std::cout << "[MemoryBlock] Verificando espacio contiguo al final. memAtEnd: " 
                      << memAtEnd << std::endl;
            
            // Verificar espacio contiguo al final
            if (size <= memAtEnd) {
                std::cout << "[MemoryBlock] Asignando espacio contiguo al final" << std::endl;
                void* addr = GetLastFreeAddr();
                std::cout << "[MemoryBlock] Dirección asignada: " << addr << std::endl;
                
                memList.insert(nextId, size, type, addr);
                usedMem += size;
                memAtEnd -= size;
                
                std::cout << "[MemoryBlock] Bloque creado. ID: " << nextId 
                          << ", Memoria usada: " << usedMem 
                          << ", Memoria libre al final: " << memAtEnd << std::endl;
                return nextId++;
            }
        
            size_t freeBlockSize = (memSize - usedMem) - memAtEnd;
            std::cout << "[MemoryBlock] Espacio libre entre bloques: " << freeBlockSize << std::endl;
        
            // Intentar reutilizar bloques libres
            if (freeBlockSize >= size) {
                std::cout << "[MemoryBlock] Intentando reutilizar bloque libre existente" << std::endl;
                int reusedId = memList.reuseFreeBlock(size, type, nextId);
                if (reusedId > -1) {
                    nextId++;
                    std::cout << "[MemoryBlock] Bloque reutilizado. ID: " << reusedId 
                              << ", Memoria usada: " << usedMem << std::endl;
                    return reusedId;
                }
            }
        
            // Intentar compactación si no hay espacio suficiente
            if (freeBlockSize + memAtEnd >= size) {
                std::cout << "[MemoryBlock] Intentando compactar memoria para hacer espacio" << std::endl;
                CompactMemory();
                if (memAtEnd > memSize - usedMem) {  // Línea añadida
                    memAtEnd = memSize - usedMem;    // Línea añadida (corrección forzada)
                }
                void* addr = GetLastFreeAddr();
                std::cout << "[MemoryBlock] Dirección asignada post-compactación: " << addr << std::endl;
                
                memList.insert(nextId, size, type, addr);
                usedMem += size;
                memAtEnd -= size;
                
                std::cout << "[MemoryBlock] Bloque creado después de compactación. ID: " << nextId 
                          << ", Memoria usada: " << usedMem 
                          << ", Memoria libre al final: " << memAtEnd << std::endl;
                return nextId++;
            }
        
            std::cout << "[MemoryBlock] ERROR: No se pudo asignar memoria después de todos los intentos" << std::endl;
            throw std::runtime_error("No se pudo asignar memoria");
        }



        void CompactMemory() {
            std::cout << "\n[MemoryBlock] INICIANDO COMPACTACIÓN DE MEMORIA" << std::endl;
            std::cout << "[MemoryBlock] Estado antes de compactar:" << std::endl;
            std::cout << " - Memoria total: " << memSize << std::endl;
            std::cout << " - Memoria usada: " << usedMem << std::endl;
            std::cout << " - Memoria libre al final: " << memAtEnd << std::endl;
            memList.printList();
        
            Node* current = memList.getHead();
            Node* prev = nullptr;
            char* currentPos = static_cast<char*>(memBlock);
            size_t freeSpace = 0;
            size_t movedBlocks = 0;
            size_t freedBlocks = 0;
        
            std::cout << "[MemoryBlock] Posición inicial: " << (void*)currentPos << std::endl;
        
            while (current) {
                if (current->block.type == "") {
                    // Bloque libre encontrado
                    std::cout << "[MemoryBlock] Eliminando bloque libre - ID: " 
                              << current->block.id << ", Tamaño: " << current->block.size << std::endl;
                    
                    Node* toDelete = current;
                    freeSpace += current->block.size;
                    freedBlocks++;
        
                    if (prev) {
                        prev->next = current->next;
                    } else {
                        memList.updateHead(current->next);
                    }
        
                    current = current->next;
                    delete toDelete;
                } else {
                    // Bloque ocupado
                    if (freeSpace > 0) {
                        void* oldPos = current->block.ptr;
                        void* newPos = currentPos;
                        
                        std::cout << "[MemoryBlock] Moviendo bloque - ID: " << current->block.id
                                  << "\n   De: " << oldPos << " a " << newPos
                                  << "\n   Tamaño: " << current->block.size << std::endl;
                        
                        memmove(newPos, oldPos, current->block.size);
                        current->block.ptr = newPos;
                        movedBlocks++;
                    }
        
                    currentPos = static_cast<char*>(current->block.ptr) + current->block.size;
                    prev = current;
                    current = current->next;
                }
            }
        
            memAtEnd = memSize - (currentPos - static_cast<char*>(memBlock));
            
            std::cout << "\n[MemoryBlock] COMPACTACIÓN COMPLETADA" << std::endl;
            std::cout << " - Bloques movidos: " << movedBlocks << std::endl;
            std::cout << " - Bloques liberados: " << freedBlocks << std::endl;
            std::cout << " - Espacio libre recuperado: " << freeSpace << std::endl;
            std::cout << " - Nueva memoria libre al final: " << memAtEnd << std::endl;
            std::cout << "Estado después de compactar:" << std::endl;
            memList.printList();
        }

        // Versión específica para datos binarios (reemplaza al template)
        void Set(int id, const std::string& serialized_data) {
            MemoryMap* block = memList.findById(id);
            if (!block) {
                throw std::runtime_error("Bloque de memoria no encontrado");
            }
            if (serialized_data.size() > block->size) {
                throw std::runtime_error("Datos exceden el tamaño del bloque");
            }
            // Copia directa de los bytes
            std::memcpy(block->ptr, serialized_data.data(), block->size);
            block->isNew = false;
        }

        std::string Get(int id) {
            MemoryMap* block = memList.findById(id);
            if (!block) {
                throw std::runtime_error("Bloque de memoria no encontrado");
            }
            // Creamos un string con el contenido binario del bloque
            return std::string(static_cast<char*>(block->ptr), block->size);
        }

        void CleanMemorySpace(MemoryMap* block){
            if (block->refCount == 0) {
                size_t freedSize = block->size;
                void* freedPtr = block->ptr;
                bool wasLastBlock = (memList.getLast() && memList.getLast()->block.id == block->id);  
                // Liberar el bloque
                if (memList.removeById(block->id)) {
                    usedMem -= freedSize;
                    if (wasLastBlock) { 
                        memAtEnd += freedSize;
                    }
                } 
                else {
                    std::cerr << "[MemoryBlock] ERROR: No se pudo remover el bloque" << std::endl;
                }
            }
        }

        void DecreaseRefCount(int id) {
            // Buscar el bloque de memoria
            MemoryMap* block = memList.findById(id);
            if (!block) {
                std::cerr << "[MemoryBlock] ERROR: Bloque ID " << id << " no encontrado" << std::endl;
                throw std::runtime_error("Bloque de memoria no encontrado");
            }    
            // Decrementar el contador
            --block->refCount;
        }
        
        void IncreaseRefCount(int id) {
            // Buscar el bloque de memoria
            MemoryMap* block = memList.findById(id);
            if (!block) {
                std::cerr << "[MemoryBlock] ERROR: Bloque ID " << id << " no encontrado" << std::endl;
                throw std::runtime_error("Bloque de memoria no encontrado");
            }
            // Incrementar el contador
            block->refCount++;
        }

        MemoryMap* GetMemoryMapById(int id) {
            return memList.findById(id);
        }

        void PrintMemoryList() {
            cout << "Contenido de la lista de MemoryBlock:" << std::endl;
            memList.printList();
        }
};


class MemoryManagerServiceImpl final : public MemoryManager::Service {
    public:
        MemoryManagerServiceImpl(MemoryBlock& memoryBlock) : memoryBlock_(memoryBlock) {}
    
        Status Create(ServerContext* context, const CreateRequest* request, CreateResponse* response)
        override {
            //cout
            std::cout << "[Servidor] Recibida solicitud Create - Tipo: " << request->type() 
                      << ", Tamaño: " << request->size() << std::endl;
            try {
                int id = memoryBlock_.Create(request->size(), request->type());
                //cout
                std::cout << "[Servidor] Bloque creado exitosamente - ID: " << id << std::endl;
                response->set_success(true);
                response->set_id(id);
            } catch (const std::exception& e) {
                //cout std::cerr << "[Servidor] Error en Create: " << e.what() << std::endl;
                response->set_success(false);
            }
            return Status::OK;
        }
    
        Status Set(ServerContext* context, const SetRequest* request, SetResponse* response)
        override {
            try {
                // Obtener los datos serializados del request
                const std::string& serialized_data = request->data();
                // Almacenar los datos directamente como bytes/bloque binario
                memoryBlock_.Set(request->id(), serialized_data);
                response->set_success(true);
                std::cout << "[Servidor] Datos asignados exitosamente" << std::endl;
                response->set_success(true);
            } 
            catch (const std::exception& e) {
                std::cerr << "[Servidor] Error en Set: " << e.what() << std::endl;
                response->set_success(false);
            }
            return Status::OK;
        }
    
        Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) 
        override {
            try {
                MemoryMap* block = memoryBlock_.GetMemoryMapById(request->id());
                if (!block) {
                    response->set_success(false);
                    return Status::OK;
                }


                // Obtenemos los datos como binario crudo
                std::string binary_data = memoryBlock_.Get(request->id());
                response->set_success(true);
                response->set_data(binary_data);  // Campo 'data' de tipo bytes en el proto
            } 
            catch (const std::exception& e) {
                std::cerr << "[Servidor] Error en Get: " << e.what() << std::endl;
                response->set_success(false);
            }
            return Status::OK;
        }

        Status IncreaseRefCount(ServerContext* context, const IncreaseRefCountRequest* request, IncreaseRefCountResponse* response)
         override {
            std::cout << "[Servidor] Incrementando ref count - ID: " << request->id() << std::endl;
            try {
            memoryBlock_.IncreaseRefCount(request->id());
            response->set_success(true);
            } catch (const std::exception& e) {
            std::cerr << "[Servidor] Error en IncreaseRefCount: " << e.what() << std::endl;
            response->set_success(false);
            }
            return Status::OK;
        }

        Status DecreaseRefCount(ServerContext* context, const DecreaseRefCountRequest* request, DecreaseRefCountResponse* response) 
        override {
            std::cout << "[Servidor] Decrementando ref count - ID: " << request->id() << std::endl;
            try {
            memoryBlock_.DecreaseRefCount(request->id());
            response->set_success(true);
            } catch (const std::exception& e) {
            std::cerr << "[Servidor] Error en DecreaseRefCount: " << e.what() << std::endl;
            response->set_success(false);
            }
            return Status::OK;
        }
     
    private:
        MemoryBlock& memoryBlock_;

        // Serializador/deserializador genérico
        template <typename T>
        std::string serialize(const T& data) {
            std::ostringstream oss;
            {
                cereal::BinaryOutputArchive archive(oss);
                archive(data);
            }
            return oss.str();
        }
        
        template <typename T>
        T deserialize(const std::string& str) {
            std::istringstream iss(str);
            T data;
            {
                cereal::BinaryInputArchive archive(iss);
                archive(data);
            }
            return data;
        }
    };
    
    void RunServer(const std::string& listenPort, size_t memSize) {
        MemoryBlock memoryBlock(memSize); // Inicializa el MemoryBlock con el tamaño especificado
    
        MemoryManagerServiceImpl service(memoryBlock);
    
        ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:" + listenPort, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
    
        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "Servidor escuchando en el puerto " << listenPort << std::endl;
        server->Wait();
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
            } else if (arg == "--memsize" && i + 1 < argc) {
                memSize = std::stoul(argv[++i]);
            } else if (arg == "--dumpFolder" && i + 1 < argc) {
                dumpFolder = argv[++i];
            } else {
                std::cerr << "Argumento desconocido o valor faltante: " << arg << std::endl;
                return 1;
            }
        }
    
        if (listenPort.empty() || memSize == 0 || dumpFolder.empty()) {
            std::cerr << "Faltan parámetros requeridos." << std::endl;
            return 1;
        }
    
        RunServer(listenPort, memSize);
        return 0;
    }