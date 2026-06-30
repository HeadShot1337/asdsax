#ifndef OPCODES_H
#define OPCODES_H

#include <cstdint>

// Custom VM Opcodes
enum Opcode : uint8_t {
    OP_HALT = 0x00,
    OP_PUSH_IMM = 0x01,
    OP_POP = 0x02,
    OP_ADD = 0x03,
    OP_SUB = 0x04,
    OP_MUL = 0x05,
    OP_DIV = 0x06,
    OP_RET = 0x07,
    OP_MOV_REG_REG = 0x08,
    OP_MOV_REG_IMM = 0x09,
    OP_ADD_REG = 0x0A,
    OP_PUSH_REG = 0x0B,
};

// Simplified PE-like structures for educational purposes
#define VM_MAGIC 0x4D56 // 'VM'

struct VM_DOS_HEADER {
    uint16_t e_magic;    // Magic number (VM_MAGIC)
    uint32_t e_lfanew;   // File address of new exe header
};

struct VM_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
};

struct VM_OPTIONAL_HEADER {
    uint32_t AddressOfEntryPoint;
    uint32_t ImageBase;
};

struct VM_NT_HEADERS {
    uint32_t Signature; // 'VMT'
    VM_FILE_HEADER FileHeader;
    VM_OPTIONAL_HEADER OptionalHeader;
};

struct VM_SECTION_HEADER {
    char Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t Characteristics;
};

#endif // OPCODES_H
