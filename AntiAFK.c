#include "../src/Commands.h"
#include "../src/Entity.h"
#include "../src/Game.h"
#include "../src/Server.h"

#include "GameSymbols.h"
#include "Utils.h"

static void AntiAfk_Init(void);
static void AntiAfk_OnNewMapLoaded(void);
static void AntiAfk_Reset(void);

const struct IGameComponent AntiAfkComp = {
    AntiAfk_Init, /* Init */
    NULL, /* Free */
    AntiAfk_Reset, /* Reset */
    NULL, /* OnNewMap */
    AntiAfk_OnNewMapLoaded, /* OnNewMapLoaded */
};

static const char AntiAfk_Enabled[]  = "&eEnabled.";
static const char AntiAfk_Disabled[] = "&eDisabled.";

static cc_bool g_Enabled;
static float   g_Interval = 1.0;

static void AntiAfk_Reset(void) {
    if (g_Enabled) {
        g_Enabled = false;
        Chat_AddRaw(AntiAfk_Disabled);
    }
}

static void AntiAfk_Execute(const cc_string* args, int argsCount) {
    cc_bool enabled;
    if (argsCount == 0) {
        Chat_AddRaw("&eToo few arguments.");
        return;
    }
    if (!OnceCall(FP_Convert_ParseBool, CONVERT_PARSEBOOL_)(args, &enabled)) {
        float interval;
        if (OnceCall(FP_Convert_ParseFloat, CONVERT_PARSEFLOAT_)(args, &interval)) {
            if (interval < 0.2f) {
                Chat_AddRaw("&eInterval is too small.");
                return;
            }
            g_Interval = interval;
            OnceCall(FP_Chat_Add1, CHAT_ADD1_)("&eInterval updated to %f2 sec.", &interval);
            return;
        }
        Chat_AddRaw("&eCould not parse value.");
        return;
    }
    if (g_Enabled == enabled) {
        Chat_AddRaw("&eValue doesn't change");
        return;
    }
    g_Enabled = enabled;
    if (enabled) {
        Chat_AddRaw(AntiAfk_Enabled);
    } else {
        Chat_AddRaw(AntiAfk_Disabled);
    }
}

static struct ChatCommand AntiAfkCmd = {
    "AntiAFK", AntiAfk_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client AntiAFK [true/false or interval in seconds]",
        "&eRotates you with specified interval in seconds"
    }
};

static struct Entity* PlayerEntity;

static void AntiAfk_Task(struct ScheduledTask* task) {
    task->interval = g_Interval;
    if (g_Enabled) {
        struct LocationUpdate update;
        update.flags = LU_HAS_YAW;
        update.yaw   = PlayerEntity->Yaw + 5.0f;
        PlayerEntity->VTABLE->SetLocation(PlayerEntity, &update);
    }
}

static void AntiAfk_Init(void) {
    OnceCall(FP_ScheduledTask_Add, SCHEDULEDTASK_ADD_)(g_Interval, AntiAfk_Task);
    OnceCall(FP_Commands_Register, COMMANDS_REGISTER_)(&AntiAfkCmd);
}

static void AntiAfk_OnNewMapLoaded(void) {
    PlayerEntity = &((struct _EntitiesData*)GetGameSymbol(ENTITIES_))->CurPlayer->Base;
    if (g_Enabled) {
        g_Enabled = false;
        Chat_AddRaw(AntiAfk_Disabled);
    }
}