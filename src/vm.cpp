#include "vm.h"
#include <iostream>
#include <cstring>

VM::VM() : pc(0), running(false) {
    for (int i = 0; i < 16; ++i) registers[i] = 0;
}

uint8_t VM::fetch_byte(const ManualMapper::MappedModule& module) {
    if (pc >= module.code.size()) {
        running = false;
        return OP_HALT;
    }
    return module.code[pc++];
}

uint64_t VM::fetch_uint64(const ManualMapper::MappedModule& module) {
    if (pc + 8 > module.code.size()) {
        running = false;
        return 0;
    }
    uint64_t val;
    std::memcpy(&val, &module.code[pc], 8);
    pc += 8;
    return val;
}

void VM::Run(const ManualMapper::MappedModule& module) {
    pc = module.header.entry_point;
    running = true;

    std::cout << "Starting VM execution at PC: " << pc << "..." << std::endl;

    while (running) {
        uint8_t opcode = fetch_byte(module);
        switch (opcode) {
            case OP_HALT:
                running = false;
                break;
            case OP_PUSH_IMM: {
                uint64_t val = fetch_uint64(module);
                stack.push(val);
                break;
            }
            case OP_POP: {
                if (!stack.empty()) stack.pop();
                break;
            }
            case OP_ADD: {
                if (stack.size() < 2) { running = false; break; }
                uint64_t b = stack.top(); stack.pop();
                uint64_t a = stack.top(); stack.pop();
                stack.push(a + b);
                break;
            }
            case OP_SUB: {
                if (stack.size() < 2) { running = false; break; }
                uint64_t b = stack.top(); stack.pop();
                uint64_t a = stack.top(); stack.pop();
                stack.push(a - b);
                break;
            }
            case OP_MUL: {
                if (stack.size() < 2) { running = false; break; }
                uint64_t b = stack.top(); stack.pop();
                uint64_t a = stack.top(); stack.pop();
                stack.push(a * b);
                break;
            }
            case OP_RET: {
                if (!stack.empty()) {
                    std::cout << "VM Return Value: " << stack.top() << std::endl;
                } else {
                    std::cout << "VM Return (no value on stack)" << std::endl;
                }
                running = false;
                break;
            }
            case OP_MOV_REG_IMM: {
                uint8_t reg = fetch_byte(module);
                uint64_t val = fetch_uint64(module);
                if (reg < 16) registers[reg] = val;
                break;
            }
            case OP_MOV_REG_REG: {
                uint8_t dst = fetch_byte(module);
                uint8_t src = fetch_byte(module);
                if (dst < 16 && src < 16) registers[dst] = registers[src];
                break;
            }
            case OP_ADD_REG: {
                uint8_t r1 = fetch_byte(module);
                uint8_t r2 = fetch_byte(module);
                if (r1 < 16 && r2 < 16) registers[r1] += registers[r2];
                break;
            }
            case OP_PUSH_REG: {
                uint8_t reg = fetch_byte(module);
                if (reg < 16) stack.push(registers[reg]);
                break;
            }
            default:
                if (running) std::cerr << "Unknown opcode: " << (int)opcode << " at PC: " << pc - 1 << std::endl;
                running = false;
                break;
        }
    }
    std::cout << "VM execution finished." << std::endl;
}
