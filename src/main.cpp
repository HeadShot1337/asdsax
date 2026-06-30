#include "vm.h"
#include "mapper.h"
#include <iostream>

int main() {
    ManualMapper::MappedModule module;
    if (!ManualMapper::LoadBytecode("translated.bin", module)) {
        std::cerr << "Failed to load/map bytecode" << std::endl;
        return 1;
    }

    VM vm;
    vm.Run(module);

    return 0;
}
