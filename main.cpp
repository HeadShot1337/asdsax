#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <stack>
#include <cstdint>
#include <algorithm>
#include <cctype>

// --- Opcodes and Structures (from opcodes.h) ---
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

#define VM_MAGIC 0x4D56

struct VM_DOS_HEADER {
    uint16_t e_magic;
    uint32_t e_lfanew;
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
    uint32_t Signature;
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

// --- Manual Mapper (from mapper.h/cpp) ---
class ManualMapper {
public:
    struct MappedModule {
        struct {
            uint32_t entry_point;
        } header;
        std::vector<uint8_t> code;
        std::vector<uint8_t> data;
    };

    static bool LoadBytecodeFromBuffer(const std::vector<uint8_t>& buffer, MappedModule& out_module) {
        size_t size = buffer.size();
        if (size < 0x40) return false;

        uint16_t magic;
        std::memcpy(&magic, buffer.data(), 2);
        if (magic != 0x4d56) {
            std::cerr << "Invalid DOS Magic: " << std::hex << magic << std::endl;
            return false;
        }

        uint32_t e_lfanew;
        std::memcpy(&e_lfanew, buffer.data() + 0x3C, 4);

        if (size < (size_t)(e_lfanew + 28)) return false;
        const uint8_t* nt_ptr = buffer.data() + e_lfanew;

        uint32_t sig;
        std::memcpy(&sig, nt_ptr, 4);
        if (sig != 0x00544D56) {
            std::cerr << "Invalid NT Signature: " << std::hex << sig << std::endl;
            return false;
        }

        uint16_t num_sections;
        std::memcpy(&num_sections, nt_ptr + 6, 2);

        uint32_t entry_point;
        std::memcpy(&entry_point, nt_ptr + 24, 4);

        out_module.header.entry_point = entry_point;

        const uint8_t* section_ptr = nt_ptr + 4 + 20 + 8;

        for (int i = 0; i < num_sections; ++i) {
            const uint8_t* current_sec = section_ptr + (i * 40);

            uint32_t size_of_raw;
            std::memcpy(&size_of_raw, current_sec + 16, 4);
            uint32_t ptr_to_raw;
            std::memcpy(&ptr_to_raw, current_sec + 20, 4);

            if (ptr_to_raw + size_of_raw > (uint32_t)size) return false;

            size_t old_size = out_module.code.size();
            out_module.code.resize(old_size + size_of_raw);
            std::memcpy(out_module.code.data() + old_size, buffer.data() + ptr_to_raw, size_of_raw);
        }

        std::cout << "Successfully manually mapped bytecode from memory." << std::endl;
        return true;
    }
};

// --- VM (from vm.h/cpp) ---
class VM {
public:
    VM() : pc(0), running(false) {
        for (int i = 0; i < 16; ++i) registers[i] = 0;
    }

    void Run(const ManualMapper::MappedModule& module) {
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

private:
    std::stack<uint64_t> stack;
    uint64_t registers[16];
    uint32_t pc;
    bool running;

    uint8_t fetch_byte(const ManualMapper::MappedModule& module) {
        if (pc >= module.code.size()) {
            running = false;
            return OP_HALT;
        }
        return module.code[pc++];
    }

    uint64_t fetch_uint64(const ManualMapper::MappedModule& module) {
        if (pc + 8 > module.code.size()) {
            running = false;
            return 0;
        }
        uint64_t val;
        std::memcpy(&val, &module.code[pc], 8);
        pc += 8;
        return val;
    }
};

// --- Base64 Decoding ---
static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<uint8_t> base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::vector<uint8_t> ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}

int main() {
    std::ifstream b64_file("base64.txt");
    if (!b64_file.is_open()) {
        std::cerr << "Failed to open base64.txt" << std::endl;
        return 1;
    }

    std::string encoded_data((std::istreambuf_iterator<char>(b64_file)),
                             std::istreambuf_iterator<char>());

    // Remove whitespace/newlines if any
    encoded_data.erase(std::remove_if(encoded_data.begin(), encoded_data.end(),
                       [](unsigned char x){ return std::isspace(x); }),
                       encoded_data.end());

    std::vector<uint8_t> decoded_bytes = base64_decode(encoded_data);
    if (decoded_bytes.empty()) {
        std::cerr << "Failed to decode Base64 data" << std::endl;
        return 1;
    }

    ManualMapper::MappedModule module;
    if (!ManualMapper::LoadBytecodeFromBuffer(decoded_bytes, module)) {
        std::cerr << "Failed to map bytecode" << std::endl;
        return 1;
    }

    VM vm;
    vm.Run(module);

    return 0;
}
