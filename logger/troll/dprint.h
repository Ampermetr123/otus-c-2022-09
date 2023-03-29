#pragma once
#include <stdio.h>

#ifndef NDEBUG
#define dprint(FMT, ...) fprintf(stderr, FMT "\n", ##__VA_ARGS__)
#else
#define dprint(...)
#endif