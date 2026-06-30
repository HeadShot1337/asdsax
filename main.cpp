#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

// --- Base64 Decoding Utility ---
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<unsigned char> base64_decode(std::string const& encoded_string) {
    int in_len = (int)encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<unsigned char> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

int main(int argc, char** argv) {
    std::ifstream b64_file("base64.txt");
    if (!b64_file.is_open()) {
        std::cerr << "Failed to open base64.txt" << std::endl;
        return 1;
    }

    std::string encoded_data((std::istreambuf_iterator<char>(b64_file)),
        std::istreambuf_iterator<char>());

    encoded_data.erase(std::remove_if(encoded_data.begin(), encoded_data.end(),
        [](unsigned char x) { return std::isspace(x); }),
        encoded_data.end());

    std::vector<unsigned char> decoded_bytes = base64_decode(encoded_data);
    if (decoded_bytes.empty()) {
        std::cerr << "Failed to decode Base64 data" << std::endl;
        return 1;
    }

    // Create anonymous file in memory
    int fd = syscall(SYS_memfd_create, "reflective_exec", 0);
    if (fd == -1) {
        perror("memfd_create");
        return 1;
    }

    // Write binary to memory file
    if (write(fd, decoded_bytes.data(), decoded_bytes.size()) != (ssize_t)decoded_bytes.size()) {
        perror("write");
        close(fd);
        return 1;
    }

    // Prepare arguments for execution
    // We pass the same arguments that were passed to this loader
    std::vector<char*> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    args.push_back(nullptr);

    // Environment variables
    char* envp[] = { nullptr };

    // Execute from file descriptor (completely in memory)
    // fexecve is not always available in all libc versions, so we use execve on /proc/self/fd/
    std::string fd_path = "/proc/self/fd/" + std::to_string(fd);
    execve(fd_path.c_str(), args.data(), envp);

    // If execve returns, it failed
    perror("execve");
    close(fd);
    return 1;
}
