import subprocess
import os
import sys

def run_command(command):
    print(f"Executing: {command}")
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error: {result.stderr}")
        return False
    if result.stdout:
        print(result.stdout)
    return True

def main():
    # Cleanup artifacts
    for f in ["target.exe", "target.o", "translated.bin", "vm_interpreter"]:
        if os.path.exists(f): os.remove(f)

    print("--- 1. Compiling Target Application ---")
    if not run_command("g++ -fno-stack-protector -O0 -c target.cpp -o target.o && g++ target.o -o target.exe"):
        return

    print("--- 2. Translating EXE to Bytecode and Packing (PE-like) ---")
    if not run_command("python3 translator.py target.exe test_func"):
        return

    print("--- 3. Compiling VM Interpreter ---")
    if not run_command("g++ -Iinclude src/vm.cpp src/mapper.cpp src/main.cpp -o vm_interpreter"):
        return

    print("--- 4. Running VM Interpreter (Manual Mapping translated.bin) ---")
    if not run_command("./vm_interpreter"):
        return

    print("--- Pipeline Completed Successfully ---")

if __name__ == "__main__":
    main()
