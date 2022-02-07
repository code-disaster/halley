#pragma once

#include "iserialization_dictionary.h"
#include <optional>
#include <vector>
#include "halley/text/halleystring.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
    class ConfigNode;
    
    class SerializationDictionary : public ISerializationDictionary {
    public:
        SerializationDictionary();
        SerializationDictionary(const ConfigNode& config);
        
        std::optional<size_t> stringToIndex(const String& string) override;
        const String& indexToString(size_t index) override;

        void addEntry(String str);
        void addEntry(size_t idx, String str);

    private:
        std::vector<String> strings;
        HashMap<String, int> indices;
    };
}
