#include "hijk/hijk.h"

#include <Windows.h>
#include <iostream>
#include <debugapi.h>

__declspec(noinline) void MyFunction(int a, float b) {
    std::cout << "a: " << a << " b: " << b << std::endl;
}

extern "C" {
    void Prologue(void* original_function, struct PrologueContext* prologue_context) {
        std::cout << "Prologue!\n";
    }
    void Epilogue(void* original_function, struct EpilogueContext* epilogue_context) {
        std::cout << "Epilogue!\n";
    }
}

int main() {
    std::cout << "hi hijk\n";

    void* target_function = &MyFunction;
    Hijk_CreateHook(target_function, &Prologue, &Epilogue);
    MyFunction(3, 45.5f);
    Hijk_EnableHook(target_function);
    MyFunction(3, 45.5f);
    std::cout << "after instrumented call" << std::endl;
}
