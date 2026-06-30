#ifndef VM_H
#define VM_H

#include "opcodes.h"
#include "mapper.h"
#include <stack>
#include <vector>
#include <cstdint>

class VM {
public:
    VM();
    void Run(const ManualMapper::MappedModule& module);

private:
    std::stack<uint64_t> stack;
    uint64_t registers[16];
    uint32_t pc;
    bool running;

    uint8_t fetch_byte(const ManualMapper::MappedModule& module);
    uint64_t fetch_uint64(const ManualMapper::MappedModule& module);
};

#endif // VM_H
