#include <Windows.h>
#include <debugapi.h>

#include <iostream>

#include "hijk/hijk.h"

static volatile int g_count;

__declspec(noinline) void MyFunction(int a, float b) {
  std::cout << "a: " << a << " b: " << b << std::endl;
}

extern "C" {
void Prologue(void* original_function, struct PrologueContext* prologue_context) {
  std::cout << "User Prologue!\n";
}
void Epilogue(void* original_function, struct EpilogueContext* epilogue_context) {
  ++g_count;
  std::cout << "UserEpilogue!\n";
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
