#pragma once
#include <cereal/archives/binary.hpp>

struct NodeStructure {
    int data_id;
    int next_id;

    NodeStructure(int dataId = 0, int nextId = 0);

    template <class Archive>
    void serialize(Archive& archive) {
        archive(data_id, next_id);
    }
};