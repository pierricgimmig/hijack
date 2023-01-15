#include <Windows.h>
#include <debugapi.h>

#include <iostream>

#include "hijk/hijk.h"

static volatile int g_count;

__declspec(noinline) void MyFunction(int a, float b) {
  std::cout << "a: " << a << " b: " << b << std::endl;
}

extern "C" {
void Prolog(void* original_function, struct Hijk_PrologContext* prolog_context) {
  std::cout << "User Prolog!\n";
}
void Epilog(void* original_function, struct Hijk_EpilogContext* epilog_context) {
  ++g_count;
  std::cout << "User Epilog!\n";
}
}

int main() {
  std::cout << "hi hijk\n";

  void* target_function = &MyFunction;
  Hijk_CreateHook(target_function, &Prolog, &Epilog);
  MyFunction(3, 45.5f);
  Hijk_EnableHook(target_function);
  MyFunction(3, 45.5f);
  std::cout << "after instrumented call" << std::endl;
}
