#include "../src/Chat.h"
#include "GameSymbols.h"
#include "Utils.h"

void Chat_AddRaw(const char* raw) {
	cc_string str = OnceCall(FP_String_FromReadonly, STRING_FROMREADONLY)(raw);
	OnceCall(FP_Chat_AddOf, CHAT_ADDOF_)(&str, MSG_TYPE_NORMAL);
}