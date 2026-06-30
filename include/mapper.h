#ifndef MAPPER_H
#define MAPPER_H

#include "opcodes.h"
#include <vector>
#include <string>

class ManualMapper {
public:
    struct MappedModule {
        struct {
            uint32_t entry_point;
        } header;
        std::vector<uint8_t> code;
        std::vector<uint8_t> data;
    };

    static bool LoadBytecode(const std::string& filename, MappedModule& out_module);
};

#endif // MAPPER_H
