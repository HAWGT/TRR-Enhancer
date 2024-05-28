#pragma once

#include <Windows.h>
#include <basetsd.h>
#include <minwindef.h>
#include <memoryapi.h>
#include <vector>
#include <cstdint>


BYTE* PatternScan(const char* signature, void* moduleOverride = nullptr);
void Patch(BYTE* src, BYTE* dst, const ULONG64 size);
bool Detour64(BYTE* src, BYTE* dst, const ULONG64 size);
BYTE* TrampHook64(BYTE* src, BYTE* dst, const ULONG64 size);
BYTE* RemoveHook(BYTE* src, BYTE* orig, const ULONG64 size);

inline std::uintptr_t GetAddressFromInstruction(std::uintptr_t address, int instruction_size)
{
	if (address == (std::uintptr_t)nullptr || instruction_size < 5) throw 0;
	return address + instruction_size + *(int*)(address + instruction_size - 4);
}
