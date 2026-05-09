#include "../src/Game.h"
#include "../src/PluginAPI.h"

#define CompsDeclList           \
DeclareComp(GameSymbolsComp)    \
DeclareComp(AntiAfkComp)        \
DeclareComp(ArtBuilderComp)

#define DeclareComp(name) extern const struct IGameComponent name;
CompsDeclList
#undef DeclareComp

#define DeclareComp(name) &name,
static const struct IGameComponent* const comps[] = {
    CompsDeclList
    NULL
};
#undef DeclareComp

#define RegisterCompsEvent(callback)    \
static void Main_##callback(void) {     \
    unsigned comp = 0;                  \
    while (comps[comp]) {               \
        if (comps[comp]->callback) {    \
            comps[comp]->callback();    \
        }                               \
        ++comp;                         \
    }                                   \
}

RegisterCompsEvent(OnNewMapLoaded)
RegisterCompsEvent(Init)

#undef RegisterCompsEvent

PLUGIN_EXPORT int Plugin_ApiVersion = 1;
PLUGIN_EXPORT struct IGameComponent Plugin_Component = { 
    Main_Init, /* Init */
    NULL, /* Free */
    NULL, /* Reset */
    NULL, /* OnNewMap */
    Main_OnNewMapLoaded /* OnNewMapLoaded */
};