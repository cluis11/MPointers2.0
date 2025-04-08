#include <iostream>
#include <string>
#include <cstddef> 
#include <cstring>
#include <type_traits>

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

// Debug macros
#define DEBUG_LOG(msg) std::cout << "[DEBUG] " << __FUNCTION__ << "(): " << msg << std::endl
#define DEBUG_LOG_VAR(var) std::cout << "[DEBUG] " << __FUNCTION__ << "(): " << #var << " = " << var << std::endl
#define ERROR_LOG(msg) std::cerr << "[ERROR] " << __FUNCTION__ << "(): " << msg << std::endl
#define MEMORY_LOG(msg) std::cout << "[MEMORY] " << msg << std::endl

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
        : id(id), size(size), type(type), refCount(0), ptr(ptr), isNew(true) {
        DEBUG_LOG("Created MemoryMap - ID: " << id << ", Size: " << size << ", Type: " << type << ", Ptr: " << ptr);
    }

    //función para imprimir los datos de la estructura
    void print() const {
        MEMORY_LOG("MemoryMap - ID: " << id
            << ", Size: " << size
            << ", Type: " << type
            << ", RefCount: " << refCount
            << ", Ptr: " << ptr 
            << ", isNew: " << isNew);
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
        DEBUG_LOG("Created Node - ID: " << id << ", Size: " << size << ", Type: " << type);
    }
};


//Lista que se unitiliza en el memory map para monitoreas la asignación de memoria
class MemoryList
{
    private:
        Node* head;
        Node* last;
    
    public:
        MemoryList(): head(nullptr), last(nullptr) {
            DEBUG_LOG("MemoryList initialized");
        }

        ~MemoryList() {
            DEBUG_LOG("MemoryList destructor called");
            Node* current = head;
            while (current != nullptr){
                Node* nextNode = current->next;
                DEBUG_LOG("Deleting block - ID: " << current->block.id << ", Ptr: " << current->block.ptr);
                delete static_cast<char*>(current->block.ptr);
                delete current;
                current = nextNode;
            }
        }

        Node* getHead() const { return head; }
        void updateHead(Node* newHead) { 
            DEBUG_LOG("Updating head from " << head << " to " << newHead);
            head = newHead; 
        }
        Node* getLast() const { return last; }

        void insert(int id, size_t size, const string& type, void* ptr) {
            DEBUG_LOG("Inserting new block - ID: " << id << ", Size: " << size << ", Type: " << type);
            Node* newNode = new Node(id, size, type, ptr);
            if (!head) {
                DEBUG_LOG("First node in list");
                head = last = newNode;
            } else {
                DEBUG_LOG("Appending to end of list");
                last->next = newNode;
                last = newNode;
            }
            DEBUG_LOG_VAR(head);
            DEBUG_LOG_VAR(last);
        }

        bool insertNextTo(int id, int newId, size_t size, const string& type, void* ptr) {
            DEBUG_LOG("Attempting to insert next to ID: " << id << ", New ID: " << newId);
            Node* current = head;
            while (current) {
                if (current->block.id == id) {
                    DEBUG_LOG("Found target block - ID: " << id);
                    Node* newNode = new Node(newId, size, type, ptr);
                    newNode->next = current->next;
                    current->next = newNode;
                    DEBUG_LOG("Inserted new block after ID: " << id);
                    return true;
                }
                current = current->next;
            }
            DEBUG_LOG("Target ID not found: " << id);
            return false; // No se encontró el id
        }

        MemoryMap* findById(int id) {
            DEBUG_LOG("Searching for block ID: " << id);
            for (Node* current = head; current != nullptr; current = current->next) {
                if (current->block.id == id) {
                    DEBUG_LOG("Found block ID: " << id << " at " << current->block.ptr);
                    return &current->block;
                }
            }
            DEBUG_LOG("Block ID not found: " << id);
            return nullptr;
        }

        bool removeById(int id) {
            DEBUG_LOG("Attempting to remove block ID: " << id);
            Node* current = head;
            Node* prev = nullptr;
            while (current != nullptr){
                if (current->block.id == id){
                    DEBUG_LOG("Found block to remove - ID: " << id);
                    if (current == last) {
                        DEBUG_LOG("Block is last in list");
                        last = prev;
                    }
                    std::memset(current->block.ptr, 0, current->block.size);
                    current->block.isNew = true;
                    current->block.type = "";
                    DEBUG_LOG("Block marked as free - ID: " << id);
                    return true;
                }
                prev = current;
                current = current->next;
            }
            DEBUG_LOG("Block to remove not found - ID: " << id);
            return false;
        }

        int reuseFreeBlock(size_t size, const string& newType, int newId) {
            DEBUG_LOG("Looking for free block - Size needed: " << size << ", Type: " << newType);
            
            Node* current = head;
            while (current) {
                if (current->block.type == "" && current->block.size >= size) {
                    DEBUG_LOG("Found free block - ID: " << current->block.id << ", Size: " << current->block.size);
                    
                    if (current->block.size == size) {
                        DEBUG_LOG("Exact size match - reusing entire block");
                        current->block.type = newType;
                        return current->block.id;
                    }
                    else {
                        DEBUG_LOG("Splitting block - Original ID: " << current->block.id << ", Size: " << current->block.size);
                        
                        void* blockAddr = current->block.ptr;
                        size_t remainingSize = current->block.size - size;
                        void* remainingAddr = static_cast<char*>(blockAddr) + size;
                        
                        current->block.size = size;
                        current->block.type = newType;
                        
                        DEBUG_LOG("Updated block - New size: " << current->block.size << ", New type: " << current->block.type);
                        DEBUG_LOG("Creating new free block - Size: " << remainingSize << ", Addr: " << remainingAddr);
                        
                        insertNextTo(current->block.id, newId, remainingSize, "", remainingAddr);
                        return current->block.id;
                    }
                }
                current = current->next;
            }
            
            DEBUG_LOG("No suitable free blocks found");
            return -1;
        }

        void printList() {
            DEBUG_LOG("Memory List Contents:");
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
            DEBUG_LOG("Calculating last free address");
            Node* lastNode = memList.getLast();
            if (!lastNode) {
                memAtEnd = memSize;
                DEBUG_LOG("No blocks - returning start of memory: " << memBlock);
                return memBlock;
            }
        
            char* lastBlockEnd = static_cast<char*>(lastNode->block.ptr) + lastNode->block.size;
            char* totalEnd = static_cast<char*>(memBlock) + memSize;
            memAtEnd = (lastBlockEnd >= totalEnd) ? 0 : totalEnd - lastBlockEnd;
            
            DEBUG_LOG("Last block ends at: " << (void*)lastBlockEnd);
            DEBUG_LOG("Total memory ends at: " << (void*)totalEnd);
            DEBUG_LOG("memAtEnd calculated as: " << memAtEnd);
            
            return (memAtEnd > 0) ? lastBlockEnd : nullptr;
        }

    public:
        MemoryBlock(size_t sizeMB){
            DEBUG_LOG("Initializing MemoryBlock with size: " << sizeMB << "MB");
            memSize = sizeMB * 1024 * 1024;
            memBlock = malloc(memSize);
            if (!memBlock) {
                ERROR_LOG("Failed to allocate memory block");
                throw std::runtime_error("Error al reservar la memoria");
            }
            usedMem = 0;
            memAtEnd = memSize;
            DEBUG_LOG("MemoryBlock initialized - Total size: " << memSize << ", Start addr: " << memBlock);
        }

        ~MemoryBlock(){
            DEBUG_LOG("MemoryBlock destructor called");
            free(memBlock);
        }

        int Create(size_t size, const string& type) {
            DEBUG_LOG("Create request - Type: " << type << ", Size: " << size);
            
            if (usedMem + size > memSize) {
                ERROR_LOG("Not enough space - Used: " << usedMem << "/" << memSize);
                throw std::runtime_error("No hay espacio suficiente");
            }
        
            DEBUG_LOG("Checking contiguous space at end. memAtEnd: " << memAtEnd);
            
            if (size <= memAtEnd) {
                DEBUG_LOG("Allocating at end of memory");
                void* addr = GetLastFreeAddr();
                DEBUG_LOG("Allocated address: " << addr);
                
                memList.insert(nextId, size, type, addr);
                usedMem += size;
                memAtEnd -= size;
                
                DEBUG_LOG("Block created - ID: " << nextId 
                          << ", Used memory: " << usedMem 
                          << ", Remaining at end: " << memAtEnd);
                return nextId++;
            }
        
            size_t freeBlockSize = (memSize - usedMem) - memAtEnd;
            DEBUG_LOG("Free space between blocks: " << freeBlockSize);
        
            if (freeBlockSize >= size) {
                DEBUG_LOG("Attempting to reuse free block");
                int reusedId = memList.reuseFreeBlock(size, type, nextId);
                if (reusedId > -1) {
                    nextId++;
                    DEBUG_LOG("Block reused - ID: " << reusedId << ", Used memory: " << usedMem);
                    return reusedId;
                }
            }
        
            if (freeBlockSize + memAtEnd >= size) {
                DEBUG_LOG("Attempting memory compaction");
                CompactMemory();
                if (memAtEnd > memSize - usedMem) {
                    memAtEnd = memSize - usedMem;
                }
                void* addr = GetLastFreeAddr();
                DEBUG_LOG("Allocated address after compaction: " << addr);
                
                memList.insert(nextId, size, type, addr);
                usedMem += size;
                memAtEnd -= size;
                
                DEBUG_LOG("Block created after compaction - ID: " << nextId 
                          << ", Used memory: " << usedMem 
                          << ", Remaining at end: " << memAtEnd);
                return nextId++;
            }
        
            ERROR_LOG("Failed to allocate memory after all attempts");
            throw std::runtime_error("No se pudo asignar memoria");
        }

        void CompactMemory() {
            DEBUG_LOG("Starting memory compaction");
            DEBUG_LOG("Initial state - Total: " << memSize << ", Used: " << usedMem << ", Free at end: " << memAtEnd);
            memList.printList();
        
            Node* current = memList.getHead();
            Node* prev = nullptr;
            char* currentPos = static_cast<char*>(memBlock);
            size_t freeSpace = 0;
            size_t movedBlocks = 0;
            size_t freedBlocks = 0;
        
            DEBUG_LOG("Starting position: " << (void*)currentPos);
        
            while (current) {
                if (current->block.type == "") {
                    DEBUG_LOG("Freeing block - ID: " << current->block.id << ", Size: " << current->block.size);
                    
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
                    if (freeSpace > 0) {
                        void* oldPos = current->block.ptr;
                        void* newPos = currentPos;
                        
                        DEBUG_LOG("Moving block - ID: " << current->block.id
                                  << " from " << oldPos << " to " << newPos
                                  << ", Size: " << current->block.size);
                        
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
            
            DEBUG_LOG("Compaction completed");
            DEBUG_LOG("Blocks moved: " << movedBlocks);
            DEBUG_LOG("Blocks freed: " << freedBlocks);
            DEBUG_LOG("Recovered space: " << freeSpace);
            DEBUG_LOG("New free space at end: " << memAtEnd);
            DEBUG_LOG("Final state:");
            memList.printList();
        }

        void Set(int id, const std::string& serialized_data) {
            DEBUG_LOG("Set request - ID: " << id << ", Data size: " << serialized_data.size());
            MemoryMap* block = memList.findById(id);
            if (!block) {
                ERROR_LOG("Block not found - ID: " << id);
                throw std::runtime_error("Bloque de memoria no encontrado");
            }
            if (serialized_data.size() > block->size) {
                ERROR_LOG("Data too large for block - Block size: " << block->size << ", Data size: " << serialized_data.size());
                throw std::runtime_error("Datos exceden el tamaño del bloque");
            }
            std::memcpy(block->ptr, serialized_data.data(), block->size);
            block->isNew = false;
            DEBUG_LOG("Data set successfully in block ID: " << id);
        }

        std::string Get(int id) {
            DEBUG_LOG("Get request - ID: " << id);
            MemoryMap* block = memList.findById(id);
            if (!block) {
                ERROR_LOG("Block not found - ID: " << id);
                throw std::runtime_error("Bloque de memoria no encontrado");
            }
            DEBUG_LOG("Returning data from block ID: " << id << ", Size: " << block->size);
            return std::string(static_cast<char*>(block->ptr), block->size);
        }

        void CleanMemorySpace(MemoryMap* block){
            DEBUG_LOG("Cleaning memory space - ID: " << block->id << ", RefCount: " << block->refCount);
            if (block->refCount == 0) {
                size_t freedSize = block->size;
                void* freedPtr = block->ptr;
                bool wasLastBlock = (memList.getLast() && memList.getLast()->block.id == block->id);  
                
                if (memList.removeById(block->id)) {
                    usedMem -= freedSize;
                    if (wasLastBlock) { 
                        memAtEnd += freedSize;
                    }
                    DEBUG_LOG("Memory cleaned - ID: " << block->id << ", Freed size: " << freedSize);
                } 
                else {
                    ERROR_LOG("Failed to remove block - ID: " << block->id);
                }
            }
        }

        void DecreaseRefCount(int id) {
            DEBUG_LOG("Decreasing ref count - ID: " << id);
            MemoryMap* block = memList.findById(id);
            if (!block) {
                ERROR_LOG("Block not found - ID: " << id);
                throw std::runtime_error("Bloque de memoria no encontrado");
            }    
            --block->refCount;
            DEBUG_LOG("New ref count for ID " << id << ": " << block->refCount);
            if (block->refCount == 0) {
                CleanMemorySpace(block);
            }
        }
        
        void IncreaseRefCount(int id) {
            DEBUG_LOG("Increasing ref count - ID: " << id);
            MemoryMap* block = memList.findById(id);
            if (!block) {
                ERROR_LOG("Block not found - ID: " << id);
                throw std::runtime_error("Bloque de memoria no encontrado");
            }
            ++block->refCount;
            DEBUG_LOG("New ref count for ID " << id << ": " << block->refCount);
        }

        MemoryMap* GetMemoryMapById(int id) {
            DEBUG_LOG("GetMemoryMapById request - ID: " << id);
            return memList.findById(id);
        }

        void PrintMemoryList() {
            DEBUG_LOG("Printing memory list");
            memList.printList();
        }
};


class MemoryManagerServiceImpl final : public MemoryManager::Service {
    public:
        MemoryManagerServiceImpl(MemoryBlock& memoryBlock) : memoryBlock_(memoryBlock) {
            DEBUG_LOG("MemoryManagerServiceImpl initialized");
        }
    
        Status Create(ServerContext* context, const CreateRequest* request, CreateResponse* response) override {
            DEBUG_LOG("Create request - Type: " << request->type() << ", Size: " << request->size());
            try {
                int id = memoryBlock_.Create(request->size(), request->type());
                DEBUG_LOG("Create successful - ID: " << id);
                response->set_success(true);
                response->set_id(id);
            } catch (const std::exception& e) {
                ERROR_LOG("Create failed: " << e.what());
                response->set_success(false);
            }
            return Status::OK;
        }
    
        Status Set(ServerContext* context, const SetRequest* request, SetResponse* response) override {
            DEBUG_LOG("Set request - ID: " << request->id() << ", Data size: " << request->data().size());
            try {
                memoryBlock_.Set(request->id(), request->data());
                DEBUG_LOG("Set successful - ID: " << request->id());
                response->set_success(true);
            } 
            catch (const std::exception& e) {
                ERROR_LOG("Set failed: " << e.what());
                response->set_success(false);
            }
            return Status::OK;
        }
    
        Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
            DEBUG_LOG("Get request - ID: " << request->id());
            try {
                MemoryMap* block = memoryBlock_.GetMemoryMapById(request->id());
                if (!block) {
                    DEBUG_LOG("Get failed - Block not found");
                    response->set_success(false);
                    return Status::OK;
                }

                std::string binary_data = memoryBlock_.Get(request->id());
                DEBUG_LOG("Get successful - ID: " << request->id() << ", Data size: " << binary_data.size());
                response->set_success(true);
                response->set_data(binary_data);
            } 
            catch (const std::exception& e) {
                ERROR_LOG("Get failed: " << e.what());
                response->set_success(false);
            }
            return Status::OK;
        }

        Status IncreaseRefCount(ServerContext* context, const IncreaseRefCountRequest* request, IncreaseRefCountResponse* response) override {
            DEBUG_LOG("IncreaseRefCount request - ID: " << request->id());
            try {
                memoryBlock_.IncreaseRefCount(request->id());
                DEBUG_LOG("IncreaseRefCount successful");
                response->set_success(true);
            } catch (const std::exception& e) {
                ERROR_LOG("IncreaseRefCount failed: " << e.what());
                response->set_success(false);
            }
            return Status::OK;
        }

        Status DecreaseRefCount(ServerContext* context, const DecreaseRefCountRequest* request, DecreaseRefCountResponse* response) override {
            DEBUG_LOG("DecreaseRefCount request - ID: " << request->id());
            try {
                memoryBlock_.DecreaseRefCount(request->id());
                DEBUG_LOG("DecreaseRefCount successful");
                response->set_success(true);
            } catch (const std::exception& e) {
                ERROR_LOG("DecreaseRefCount failed: " << e.what());
                response->set_success(false);
            }
            return Status::OK;
        }
     
    private:
        MemoryBlock& memoryBlock_;
};
    
void RunServer(const std::string& listenPort, size_t memSize) {
    DEBUG_LOG("Starting server on port: " << listenPort << " with memory size: " << memSize << "MB");
    MemoryBlock memoryBlock(memSize);

    MemoryManagerServiceImpl service(memoryBlock);

    ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:" + listenPort, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    DEBUG_LOG("Server listening on port " << listenPort);
    server->Wait();
}
    
int main(int argc, char** argv) {
    if (argc != 7) {
        ERROR_LOG("Invalid arguments. Usage: " << argv[0] << " --port LISTEN_PORT --memsize SIZE_MB --dumpFolder DUMP_FOLDER");
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
            ERROR_LOG("Unknown argument or missing value: " << arg);
            return 1;
        }
    }

    if (listenPort.empty() || memSize == 0 || dumpFolder.empty()) {
        ERROR_LOG("Missing required parameters");
        return 1;
    }

    RunServer(listenPort, memSize);
    return 0;
}