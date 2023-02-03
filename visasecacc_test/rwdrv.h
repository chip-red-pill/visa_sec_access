#pragma once

#include <windows.h>

int rwdrv_read_phys_mem(unsigned long long addr, DWORD* out_val);
int rwdrv_write_phys_mem(unsigned long long addr, DWORD val);
