#pragma once

#include <stddef.h>

void* operator new(size_t count);
void* operator new[](size_t count);
void operator delete(void* ptr) noexcept;
void operator delete[](void *ptr) noexcept;
