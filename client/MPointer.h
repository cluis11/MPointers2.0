#pragma once
#include <memory>
#include <string>
#include <typeinfo>
#include <stdexcept>
#include <grpcpp/grpcpp.h>
#include "../proto/memory_manager.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
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

template<class T>
class MPointer {
private:
    static std::unique_ptr<MemoryManager::Stub> stub_;
    static std::string serverAddress;
    int id_ = -1;  // -1 indica nullptr

    T getValue() const {
        if (id_ == -1) throw std::runtime_error("Dereferencing null MPointer");
        
        ClientContext context;
        GetRequest request;
        GetResponse response;
        request.set_id(id_);
        
        if (!stub_->Get(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("Failed to get value from server");
        }

        if constexpr (std::is_same_v<T, int>) {
            return response.int_value();
        } else if constexpr (std::is_same_v<T, float>) {
            return response.float_value();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return response.string_value();
        }
        throw std::runtime_error("Unsupported type");
    }

    void setValue(const T& value) {
        if (id_ == -1) throw std::runtime_error("Assigning to null MPointer");
        
        ClientContext context;
        SetRequest request;
        SetResponse response;
        request.set_id(id_);

        if constexpr (std::is_same_v<T, int>) {
            request.set_int_value(value);
        } else if constexpr (std::is_same_v<T, float>) {
            request.set_float_value(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            request.set_string_value(value);
        }

        if (!stub_->Set(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("Failed to set value on server");
        }
    }

    void manageRefCount(bool increase) {
        if (id_ == -1) return;
        
        ClientContext context;
        if (increase) {
            IncreaseRefCountRequest request;
            IncreaseRefCountResponse response;
            request.set_id(id_);
            if (!stub_->IncreaseRefCount(&context, request, &response).ok() || !response.success()) {
                throw std::runtime_error("Failed to increase ref count");
            }
        } else {
            DecreaseRefCountRequest request;
            DecreaseRefCountResponse response;
            request.set_id(id_);
            if (!stub_->DecreaseRefCount(&context, request, &response).ok() || !response.success()) {
                throw std::runtime_error("Failed to decrease ref count");
            }
        }
    }

public:
    class Reference {
        MPointer* ptr;
    public:
        Reference(MPointer* p) : ptr(p) {}
        
        Reference& operator=(const T& value) {
            ptr->setValue(value);
            return *this;
        }
        
        operator T() const {
            return ptr->getValue();
        }
    };

    static void Init(const std::string& address) {
        serverAddress = address;
        stub_ = MemoryManager::NewStub(grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials()));
    }

    MPointer() = default;
    explicit MPointer(int id) : id_(id) { manageRefCount(true); }
    MPointer(const MPointer& other) : id_(other.id_) { manageRefCount(true); }
    MPointer(MPointer&& other) noexcept : id_(other.id_) { other.id_ = -1; }
    ~MPointer() { manageRefCount(false); }

    MPointer& operator=(const MPointer& other) {
        if (this != &other) {
            manageRefCount(false);
            id_ = other.id_;
            manageRefCount(true);
        }
        return *this;
    }
    
    MPointer& operator=(MPointer&& other) noexcept {
        if (this != &other) {
            manageRefCount(false);
            id_ = other.id_;
            other.id_ = -1;
        }
        return *this;
    }

    Reference operator*() { return Reference(this); }
    
    T* operator->() {
        static T temp;
        temp = getValue();
        return &temp;
    }

    explicit operator bool() const { return id_ != -1; }
    
    bool operator==(std::nullptr_t) const { return id_ == -1; }
    bool operator!=(std::nullptr_t) const { return id_ != -1; }
    
    int id() const { return id_; }

    static MPointer New(const T& initialValue = T()) {
        ClientContext context;
        CreateRequest request;
        CreateResponse response;
        
        request.set_size(sizeof(T));
        request.set_type(typeid(T).name());
        
        if constexpr (std::is_same_v<T, int>) {
            request.set_int_value(initialValue);
        } else if constexpr (std::is_same_v<T, float>) {
            request.set_float_value(initialValue);
        } else if constexpr (std::is_same_v<T, std::string>) {
            request.set_string_value(initialValue);
        }

        if (!stub_->Create(&context, request, &response).ok() || !response.success()) {
            throw std::runtime_error("Failed to create MPointer on server");
        }
        
        return MPointer(response.id());
    }
};

template<class T>
std::unique_ptr<MemoryManager::Stub> MPointer<T>::stub_ = nullptr;

template<class T>
std::string MPointer<T>::serverAddress = "";