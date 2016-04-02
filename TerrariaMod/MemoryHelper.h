#pragma once

void WriteToMemory(void* dst, void* src, size_t size);
int FindPattern(void* block, size_t blockSize, void* pattern, size_t patternSize);
void* ScanPattern(void* blockAddress, size_t blockSize, void* pattern, size_t patternSize);