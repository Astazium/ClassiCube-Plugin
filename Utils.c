#include "../src/String_.h"
#include "../src/Chat.h"
#include "GameSymbols.h"
#include "Utils.h"

void Chat_AddRaw(const char* raw) {
    cc_string str = OnceCall(FP_String_FromReadonly, STRING_FROMREADONLY)(raw);
    OnceCall(FP_Chat_AddOf, CHAT_ADDOF_)(&str, MSG_TYPE_NORMAL);
}

void Time_FormatSeconds(cc_string* str, float totalSeconds) {
    FP_String_Format1 String_Format1_;
    int hours, minutes, seconds, milliseconds;
    hours           = (int)totalSeconds / 3600;
    minutes         = ((int)totalSeconds % 3600) / 60;
    seconds         = (int)totalSeconds % 60;
    milliseconds    = (int)(((totalSeconds - (int)totalSeconds) * 1000.0f) + 0.5f);;
    String_Format1_ = (FP_String_Format1)GetGameRawSymbol(STRING_FORMAT1_);

    if (hours)
        String_Format1_(str, "%ih ", &hours);
    if (minutes)
        String_Format1_(str, "%im ", &minutes);
    OnceCall(FP_String_Format2, STRING_FORMAT2_)(str, "%is %ims", &seconds, &milliseconds);
}