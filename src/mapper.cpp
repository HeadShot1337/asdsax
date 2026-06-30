#include "mapper.h"
#include <fstream>
#include <iostream>
#include <cstring>

bool ManualMapper::LoadBytecode(const std::string& filename, MappedModule& out_module) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return false;

    // 1. Parse DOS Header
    if (size < 0x40) return false;

    uint16_t magic;
    std::memcpy(&magic, buffer.data(), 2);
    if (magic != 0x4d56) { // 'VM' in little-endian
        std::cerr << "Invalid DOS Magic: " << std::hex << magic << std::endl;
        return false;
    }

    uint32_t e_lfanew;
    std::memcpy(&e_lfanew, buffer.data() + 0x3C, 4);

    // 2. Parse NT Headers
    if (size < (std::streamsize)(e_lfanew + 28)) return false;
    uint8_t* nt_ptr = buffer.data() + e_lfanew;

    uint32_t sig;
    std::memcpy(&sig, nt_ptr, 4);
    if (sig != 0x00544D56) { // 'VMT\0'
        std::cerr << "Invalid NT Signature: " << std::hex << sig << std::endl;
        return false;
    }

    uint16_t num_sections;
    std::memcpy(&num_sections, nt_ptr + 6, 2);

    uint32_t entry_point;
    std::memcpy(&entry_point, nt_ptr + 24, 4);

    out_module.header.entry_point = entry_point;

    // 3. Parse Sections
    uint8_t* section_ptr = nt_ptr + 4 + 20 + 8; // Sig + FileHeader + OptionalHeader

    for (int i = 0; i < num_sections; ++i) {
        uint8_t* current_sec = section_ptr + (i * 40);

        uint32_t size_of_raw;
        std::memcpy(&size_of_raw, current_sec + 16, 4);
        uint32_t ptr_to_raw;
        std::memcpy(&ptr_to_raw, current_sec + 20, 4);

        if (ptr_to_raw + size_of_raw > (uint32_t)size) return false;

        size_t old_size = out_module.code.size();
        out_module.code.resize(old_size + size_of_raw);
        std::memcpy(out_module.code.data() + old_size, buffer.data() + ptr_to_raw, size_of_raw);
    }

    std::cout << "Successfully manually mapped bytecode." << std::endl;
    return true;
}
