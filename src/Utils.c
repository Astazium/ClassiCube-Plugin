#include "../ClassiCube/src/String_.h"
#include "../ClassiCube/src/Chat.h"
#include "GameSymbols.h"
#include "Utils.h"

void Chat_AddRaw(const char* raw) {
    cc_string str = GetFP(FP_String_FromReadonly, STRING_FROMREADONLY)(raw);
    GetFP(FP_Chat_AddOf, CHAT_ADDOF_)(&str, MSG_TYPE_NORMAL);
}

void Time_FormatSeconds(cc_string* str, float totalSeconds) {
    FP_String_Format2 String_Format2_;
    int hours, minutes, seconds, milliseconds;
    int total_ms;

    total_ms = (int)(totalSeconds * 1000.0f + 0.5f);
    milliseconds = total_ms % 1000;
    seconds = (total_ms / 1000) % 60;
    minutes = (total_ms / 60000) % 60;
    hours   = (total_ms / 3600000);

    String_Format2_ = GetFP(FP_String_Format2, STRING_FORMAT2_);

    if (hours) {
        String_Format2_(str, "%ih %im", &hours, &minutes);
        return;
    }
    if (minutes) {
        String_Format2_(str, "%im %is", &minutes, &seconds);
        return;
    }
    String_Format2_(str, "%is %ims", &seconds, &milliseconds);
}