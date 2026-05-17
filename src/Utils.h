#ifndef UTILS_H
#define UTILS_H

void* Mem_Set(void* dst, cc_uint8 value, unsigned numBytes);
void  Chat_AddRaw(const char* raw);
void  Time_FormatSeconds(cc_string* str, float totalSeconds);

#endif