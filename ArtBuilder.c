#include "../src/Block.h"
#include "../src/Chat.h"
#include "../src/Commands.h"
#include "../src/Entity.h"
#include "../src/Errors.h"
#include "../src/ExtMath.h"
#include "../src/Game.h"
#include "../src/Options.h"
#include "../src/Server.h"
#include "../src/Stream.h"
#include "../src/String_.h"
#include "../src/TexturePack.h"
#include "../src/World.h"

#include "GameSymbols.h"
#include "Utils.h"

static void FreeImage(void);
static void ArtBuilder_Init(void);
static void ArtBuilder_OnNewMapLoaded(void);

const struct IGameComponent ArtBuilderComp = {
    ArtBuilder_Init, /* Init */
    FreeImage, /* Free */
    FreeImage, /* Reset */
    NULL, /* OnNewMap */
    ArtBuilder_OnNewMapLoaded, /* OnNewMapLoaded */
};

static BlockID   blockCount = BLOCK_MAX_CPE;
static BitmapCol blocksColor [BLOCK_MAX_CPE];

static cc_bool Block_IsFull(const struct _BlockLists* Blocks_, BlockID block) {
    Vec3 minBB = Blocks_->MinBB[block];
    Vec3 maxBB = Blocks_->MaxBB[block];
    return minBB.x == 0.0f && minBB.y == 0.0f && minBB.z == 0.0f &&
           maxBB.x == 1.0f && maxBB.y == 1.0f && maxBB.z == 1.0f;
}

#ifdef Block_Tex
#undef Block_Tex
#endif

#define Block_Tex(blockLists, block, face) blockLists->Textures[(block) * FACE_COUNT + (face)]

static void TakeAverageBlocksColor(void) {
    struct _BlockLists*  Blocks_;
    struct _Atlas2DData* Atlas2D_;
    int block;

    Blocks_  = GetGameSymbol(BLOCKS_);
    Atlas2D_ = GetGameSymbol(ATLAS2D_);

    for (block = BLOCK_STONE; block <= blockCount; ++block) {
        cc_uint64 sumR, sumG, sumB, sumA, pixelCount;
        int face;

        if (!Blocks_->CanPlace[block] || !Blocks_->CanDelete[block] || Blocks_->IsLiquid[block] ||
            !Block_IsFull(Blocks_, block) || block == BLOCK_TNT || block == BLOCK_GLASS)
        {
            continue;
        }

        sumR = sumG = sumB = sumA = pixelCount = 0;

        for (face = 0; face < FACE_COUNT; ++face) {
            TextureLoc texLoc = Block_Tex(Blocks_, block, face);
            int tileSize = Atlas2D_->TileSize;
            int baseX = Atlas2D_TileX(texLoc) * tileSize;
            int baseY = Atlas2D_TileY(texLoc) * tileSize;

            int y;
            for (y = 0; y < tileSize; ++y) {
                BitmapCol* row = Bitmap_GetRow(&Atlas2D_->Bmp, baseY + y);

                int x;
                for (x = 0; x < tileSize; ++x) {
                    BitmapCol col = row[baseX + x];

                    sumR += BitmapCol_R(col);
                    sumG += BitmapCol_G(col);
                    sumB += BitmapCol_B(col);
                    sumA += BitmapCol_A(col);

                    pixelCount++;
                }
            }
        }

        if (pixelCount != 0) {
            cc_uint8 avgR = (cc_uint8)(sumR / pixelCount);
            cc_uint8 avgG = (cc_uint8)(sumG / pixelCount);
            cc_uint8 avgB = (cc_uint8)(sumB / pixelCount);
            cc_uint8 avgA = (cc_uint8)(sumA / pixelCount);
            blocksColor[block] = BitmapCol_Make(avgR, avgG, avgB, avgA);
        }
    }
}

static BlockID FindClosestBlock(BitmapCol col) {
    int r = (int)BitmapCol_R(col);
    int g = (int)BitmapCol_G(col);
    int b = (int)BitmapCol_B(col);

    BlockID   bestBlock = 0;
    cc_uint32 bestDist = 0xFFFFFFFF;

    int i;
    for (i = 0; i <= blockCount; ++i) {
        BitmapCol candidate = blocksColor[i];
        if (candidate != 0) {
            int cr = (int)BitmapCol_R(candidate);
            int cg = (int)BitmapCol_G(candidate);
            int cb = (int)BitmapCol_B(candidate);

            int dr = cr - r;
            int dg = cg - g;
            int db = cb - b;

            int rMean = (r + cr) / 2;
            cc_uint32 dist = (cc_uint32)(
                (2 + rMean / 256) * dr * dr +
                4 * dg * dg +
                (2 + (255 - rMean) / 256) * db * db
                );

            if (dist < bestDist) {
                bestDist = dist;
                bestBlock = (BlockID)i;
                if (dist == 0) break;
            }
        }
    }

    return bestBlock;
}

static const char* Png_GetErrorString(cc_result pngErr) {
    switch (pngErr)
    {
    case PNG_ERR_INVALID_SIG:		return "Stream doesn't start with PNG signature";
    case PNG_ERR_INVALID_HDR_SIZE:	return "Header chunk has invalid size";
    case PNG_ERR_TOO_WIDE:			return "Image is over 32,768 pixels wide";
    case PNG_ERR_TOO_TALL:			return "Image is over 32,768 pixels tall";
    case PNG_ERR_INVALID_COL_BPP:	return "Invalid colorspace and bits per sample combination";
    case PNG_ERR_COMP_METHOD:		return "Image uses unsupported compression method";
    case PNG_ERR_FILTER:			return "Image uses unsupported filter method";
    case PNG_ERR_INTERLACED:		return "Image uses interlacing, which is unimplemented";
    case PNG_ERR_PAL_SIZE:			return "Palette chunk has invalid size";
    case PNG_ERR_TRANS_COUNT:		return "Translucent chunk has invalid size";
    case PNG_ERR_TRANS_INVALID:		return "Colorspace doesn't support translucent chunk";
    case PNG_ERR_REACHED_IEND:		return "Image only has partial data";
    case PNG_ERR_NO_DATA:			return "Image is missing all data";
    case PNG_ERR_INVALID_SCANLINE:	return "Image row has invalid type";
    case PNG_ERR_16BITSAMPLES:		return "Image uses 16 bit samples, which is unimplemented";
    default:						return "Unknown error";
    }
}

typedef struct IVec2_ { int x, y; } IVec2;

struct {
    int     teleportRange;
    float   placeInterval;
    cc_bool exitOnFinish;
    cc_bool buildRunning;
    cc_bool enabled;
} MPmode = {6, 0.08f, false, false, false};

struct {
    struct Bitmap bmp;
    IVec3 pos;
    Vec3  right;
    Vec3  up;
    int x;
    int y;
} Image;

static void FreeImage(void) {
    MPmode.buildRunning = false;
    OnceCall(FP_Mem_Free, MEM_FREE_)(Image.bmp.scan0);
    Image.bmp.scan0 = NULL;
}

static struct Entity* PlayerEntity;

static void ArtBuilder_Build(const IVec2* dir);
static void ShowFSError(const char* contextMsg, cc_result errCode);
/* Whether the given coordinates lie inside the map. */
static cc_bool World_Contains_(const IVec3* pos);

static void ArtBuilder_Execute(const cc_string* args, int argsCount) {
    FP_String_CaselessEqualsConst String_CaselessEqualsConst_;
    FP_Convert_ParseInt Convert_ParseInt_;
    FP_Chat_Add1 Chat_Add1_;

    String_CaselessEqualsConst_ = (FP_String_CaselessEqualsConst)GetGameRawSymbol(STRING_CASELESSEQUALSCONST_);
    Convert_ParseInt_ = (FP_Convert_ParseInt)GetGameRawSymbol(CONVERT_PARSEINT_);
    Chat_Add1_ = (FP_Chat_Add1)GetGameRawSymbol(CHAT_ADD1_);

    if (argsCount == 1) {
        Chat_AddRaw("&eToo few arguments.");
        return;
    }

    if (String_CaselessEqualsConst_(&args[0], "build")) {
        cc_result fileSystemErr;
        cc_result pngDecodeErr;
        struct Stream s;
        IVec2 dir;
        cc_uint8 off;
        
        if (String_CaselessEqualsConst_(&args[1], "stop")) {
            int placedBlocks;
            if (!MPmode.enabled) {
                Chat_AddRaw("&eYou are not in multiplayer mode.");
                return;
            }
            if (!MPmode.buildRunning) {
                Chat_AddRaw("&eBuild already stopped.");
                return;
            }
            placedBlocks = Image.y * Image.bmp.width + Image.x;
            Chat_Add1_("&eBuild stopped, %i blocks were builded.", &placedBlocks);
            FreeImage();
            return;
        }

        if (String_CaselessEqualsConst_(&args[1], "eta")) {
            int totalBlocks, placedBlocks, remainingBlocks;
            char msgBuf[256];
            cc_string msgStr;
            if (!MPmode.enabled) {
                Chat_AddRaw("&eYou are not in multiplayer mode.");
                return;
            }
            if (!MPmode.buildRunning) {
                Chat_AddRaw("&eBuild already stopped.");
                return;
            }
            String_InitArray(msgStr, msgBuf);
            totalBlocks = Image.bmp.width * Image.bmp.height;
            placedBlocks = Image.y * Image.bmp.width + Image.x;
            remainingBlocks = totalBlocks - placedBlocks;
            OnceCall(FP_String_AppendConst, STRING_APPENDCONST_)(&msgStr, "&eRemaining time: ");
            Time_FormatSeconds(&msgStr, remainingBlocks * MPmode.placeInterval);
            OnceCall(FP_Chat_Add, CHAT_ADD_)(&msgStr);
            return;
        }

        if (argsCount == 3 || argsCount == 6) {
            Chat_AddRaw("&eToo few arguments.");
            return;
        }

        if (MPmode.buildRunning) {
            Chat_AddRaw("&eBuild already running.");
            return;
        }

        if (argsCount > 3) {
            off = argsCount >= 5;
            ++off;
            if (!Convert_ParseInt_(&args[0 + off], &Image.pos.x) ||
                !Convert_ParseInt_(&args[1 + off], &Image.pos.y) ||
                !Convert_ParseInt_(&args[2 + off], &Image.pos.z))
            {
                Chat_AddRaw("&eCould not parse coordinates.");
                return;
            }
        } else {
            Image.pos.x = (int)PlayerEntity->Position.x;
            Image.pos.y = (int)PlayerEntity->Position.y;
            Image.pos.z = (int)PlayerEntity->Position.z;
        }

        if (!World_Contains_(&Image.pos)) {
            Chat_AddRaw("&eCoordinates are outside the world boundaries.");
            return;
        }

        if (argsCount > 6) {
            off += 3;
            if (!Convert_ParseInt_(&args[0 + off], &dir.x) ||
                !Convert_ParseInt_(&args[1 + off], &dir.y))
            {
                Chat_AddRaw("&eCould not parse direction.");
                return;
            }
        } else {
            int pitch = (int)PlayerEntity->Pitch;
            int yaw   = (int)PlayerEntity->Yaw;
            dir.x = ((pitch + 45) / 90) * 90;
            dir.y = ((yaw + 45) / 90) * 90 - 90;
        }
        fileSystemErr = OnceCall(FP_Stream_OpenFile, STREAM_OPENFILE_)(&s, &args[1]);
        if (fileSystemErr) {
            ShowFSError("Could not open the image", fileSystemErr);
            return;
        }
        pngDecodeErr = OnceCall(FP_Png_Decode, PNG_DECODE_)(&Image.bmp, &s);
        if (pngDecodeErr) {
            Chat_Add1_("&eCould not decode png: %c", Png_GetErrorString(pngDecodeErr));
            return;
        }
        s.Close(&s);
        ArtBuilder_Build(&dir);
        return;
    }

    if (String_CaselessEqualsConst_(&args[0], "multiplayer") ||
        String_CaselessEqualsConst_(&args[0], "mp")) 
    {
        cc_bool enabled;
        if (!OnceCall(FP_Convert_ParseBool, CONVERT_PARSEBOOL_)(&args[1], &enabled)) {
            Chat_AddRaw("&eCould not parse value.");
            return;
        }
        MPmode.enabled = enabled;
        if (enabled) {
            Chat_AddRaw("&eMultiplayer Mode enabled.");
        } else {
            Chat_AddRaw("&eMultiplayer Mode disabled.");
        }
        return;
    }

    if (String_CaselessEqualsConst_(&args[0], "exitOnFinish")) {
        cc_bool exitOnFinish;
        if (!OnceCall(FP_Convert_ParseBool, CONVERT_PARSEBOOL_)(&args[1], &exitOnFinish)) {
            Chat_AddRaw("&eCould not parse value.");
            return;
        }
        MPmode.exitOnFinish = exitOnFinish;
        if (exitOnFinish) {
            Chat_AddRaw("&eExit on finish enabled.");
        } else {
            Chat_AddRaw("&eExit on finish disabled.");
        }
        return;
    }

    if (String_CaselessEqualsConst_(&args[0], "teleportrange") ||
        String_CaselessEqualsConst_(&args[0], "tprange")) 
    {
        int teleportRange;
        if (!Convert_ParseInt_(&args[1], &teleportRange)) {
            Chat_AddRaw("&eCould not parse value.");
            return;
        }
        if (teleportRange < 1) {
            Chat_AddRaw("&eTeleport range is too small.");
            return;
        }
        MPmode.teleportRange = teleportRange;
        Chat_Add1_("&eTeleport range setted to: %i", &teleportRange);
        return;
    }

    if (String_CaselessEqualsConst_(&args[0], "placeInterval")) {
        float placeInterval;
        if (!OnceCall(FP_Convert_ParseFloat, CONVERT_PARSEFLOAT_)(&args[1], &placeInterval)) {
            Chat_AddRaw("&eCould not parse value.");
            return;
        }
        if (placeInterval < 0.07f) {
            Chat_AddRaw("&eBlock place interval is too small.");
            return;
        }
        MPmode.placeInterval = placeInterval;
        Chat_Add1_("&eBlock place interval setted to: %f2", &placeInterval);
        return;
    }

    Chat_AddRaw("&eUnknown parameter.");
}

static struct ChatCommand BuildImageCmd = {
    "ArtBuilder", ArtBuilder_Execute,
    0,
    {
        "&a/client ArtBuilder",
        "&eView README.md",
    }
};

static void ArtBuilder_SPBuild(void);

static void ArtBuilder_Build(const IVec2* dir) {
    float yawRad   = dir->y * MATH_DEG2RAD;
    float pitchRad = dir->x * MATH_DEG2RAD;
    FP_Math_SinF Math_SinF_;
    FP_Math_CosF Math_CosF_;

    Math_SinF_ = (FP_Math_SinF)GetGameRawSymbol(MATH_SINF_);
    Math_CosF_ = (FP_Math_CosF)GetGameRawSymbol(MATH_COSF_);

    Image.right.x = Math_CosF_(yawRad);
    Image.right.y = 0.0f;
    Image.right.z = Math_SinF_(yawRad);

    Image.up.x = -Math_SinF_(yawRad) * Math_SinF_(pitchRad);
    Image.up.y = Math_CosF_(pitchRad);
    Image.up.z = Math_CosF_(yawRad) * Math_SinF_(pitchRad);

    Image.x = 0;
    Image.y = 0;

    if (MPmode.enabled) {
        MPmode.buildRunning = true;
    } else {
        ArtBuilder_SPBuild();
    }
}

static struct _WorldData*  World_;
static FP_Game_ChangeBlock Game_ChangeBlock_;

/* Whether the given coordinates lie inside the map. */
static cc_bool World_Contains_(const IVec3* pos) {
    return (unsigned)pos->x < (unsigned)World_->Width
        && (unsigned)pos->y < (unsigned)World_->Height
        && (unsigned)pos->z < (unsigned)World_->Length;
}

static void GetBuildPos(IVec3* out) {
    out->x = (int)(Image.pos.x + Image.x * Image.right.x + Image.y * Image.up.x + 0.5f);
    out->y = (int)(Image.pos.y + Image.x * Image.right.y + Image.y * Image.up.y + 0.5f);
    out->z = (int)(Image.pos.z + Image.x * Image.right.z + Image.y * Image.up.z + 0.5f);
}

static void ArtBuilder_SPBuild(void) {
    for (Image.y = 0; Image.y < Image.bmp.height; ++Image.y) {
        BitmapCol* row = Bitmap_GetRow(&Image.bmp, Image.bmp.height - 1 - Image.y);
        for (Image.x = 0; Image.x < Image.bmp.width; ++Image.x) {
            IVec3 buildPos;
            BitmapCol col = row[Image.x];
            BlockID block = FindClosestBlock(col);
            GetBuildPos(&buildPos);
            if (World_Contains_(&buildPos))
                Game_ChangeBlock_(buildPos.x, buildPos.y, buildPos.z, block);
        }
    }
    int totalBlocks = Image.bmp.width * Image.bmp.height;
    OnceCall(FP_Chat_Add1, CHAT_ADD1_)("&eSuccessfully builded %i blocks.", &totalBlocks);
    FreeImage();
}

static void ArtBuilder_MPBuildTask(struct ScheduledTask* task) {
    BitmapCol* row;
    task->interval = MPmode.placeInterval;
    if (!MPmode.enabled || !MPmode.buildRunning) return;

    if (Image.y >= Image.bmp.height) {
        int totalBlocks = Image.bmp.width * Image.bmp.height;
        OnceCall(FP_Chat_Add1, CHAT_ADD1_)("&eSuccessfully builded %i blocks.", &totalBlocks);
        FreeImage();
        if (MPmode.exitOnFinish)
            OnceCall(FP_Process_Exit, PROCESS_EXIT_)(0);
        return;
    }

    row = Bitmap_GetRow(&Image.bmp, Image.bmp.height - 1 - Image.y);

    if (Image.x < Image.bmp.width) {
        IVec3 buildPos;
        struct LocationUpdate update;
        Vec3 playerPos;
        cc_bool isPlayerTP = false;
        BitmapCol col = row[Image.x];
        BlockID block = FindClosestBlock(col);
        GetBuildPos(&buildPos);

        playerPos = PlayerEntity->Position;
        update.flags = LU_HAS_POS;
        update.pos = playerPos;

        if (playerPos.x > buildPos.x + MPmode.teleportRange ||
            playerPos.x < buildPos.x - MPmode.teleportRange)
        {
            update.pos.x = (float)buildPos.x;
            isPlayerTP = true;
        }
        if (playerPos.y > buildPos.y + MPmode.teleportRange ||
            playerPos.y < buildPos.y - MPmode.teleportRange)
        {
            update.pos.y = (float)buildPos.y;
            isPlayerTP = true;
        }
        if (playerPos.z > buildPos.z + MPmode.teleportRange ||
            playerPos.z < buildPos.z - MPmode.teleportRange)
        {
            update.pos.z = (float)buildPos.z;
            isPlayerTP = true;
        }
        if (isPlayerTP) {
            PlayerEntity->VTABLE->SetLocation(PlayerEntity, &update);
            /* Wait for one tick to server update our position */
            return;
        }

        if (World_Contains_(&buildPos))
            Game_ChangeBlock_(buildPos.x, buildPos.y, buildPos.z, block);

        ++Image.x;
    }
    else {
        Image.x = 0;
        ++Image.y;
    }
}

static void ArtBuilder_Init(void) {
    OnceCall(FP_ScheduledTask_Add, SCHEDULEDTASK_ADD_)(MPmode.placeInterval, ArtBuilder_MPBuildTask);
    OnceCall(FP_Commands_Register, COMMANDS_REGISTER_)(&BuildImageCmd);
}

static void ArtBuilder_OnNewMapLoaded(void) {
    cc_bool   hasCPE;
    struct _ServerConnectionData* Server_ = GetGameSymbol(SERVER_);

    MPmode.exitOnFinish = false;
    hasCPE = OnceCall(FP_Options_GetBool, OPTIONS_GETBOOL_)(OPT_CPE, true);
    blockCount = hasCPE ? BLOCK_MAX_CPE : BLOCK_MAX_ORIGINAL;
    OnceCall(FP_Chat_Add1, CHAT_ADD1_)("&eHas CPE: %t", &hasCPE);
    TakeAverageBlocksColor();

    PlayerEntity = &((struct _EntitiesData*)GetGameSymbol(ENTITIES_))->CurPlayer->Base;
    World_ = GetGameSymbol(WORLD_);
    Game_ChangeBlock_ = (FP_Game_ChangeBlock)GetGameRawSymbol(GAME_CHANGEBLOCK_);

    FreeImage();

    if (!Server_->IsSinglePlayer) {
        MPmode.enabled = true;
        Chat_AddRaw("&eYou are currently in multiplayer");
        Chat_AddRaw("&eArtBuilder mode setted to multiplayer");
        Chat_AddRaw("&eTo turn it off, type /client ArtBuilder mp false");
    } else {
        MPmode.enabled = false;
    }
}

#ifdef  CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>

static void ShowFSError(const char* contextMsg, cc_result errCode) {
    LPSTR errMsgBuf = NULL;
    FP_Window_ShowDialog Window_ShowDialog_ = (FP_Window_ShowDialog)GetGameRawSymbol(WINDOW_SHOWDIALOG_);
    if (!FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errMsgBuf,
        0,
        NULL)) 
    {
        cc_string errCodeStr; char errCodeBuf[512];
        String_InitArray_NT(errCodeStr, errCodeBuf);
        OnceCall(FP_String_Format1, STRING_FORMAT1_)(&errCodeStr, "Error code: %i", &errCode);
        errCodeStr.buffer[errCodeStr.length] = '\0';
        Window_ShowDialog_(contextMsg, errCodeStr.buffer);
        return;
    }

    Window_ShowDialog_(contextMsg, errMsgBuf);
    LocalFree(errMsgBuf);
}

#else

static void ShowFileSystemError(const char* contextMsg, cc_result errCode) {
    OnceCall(FP_Chat_Add2, CHAT_ADD2_)("&e%c: %i", contextMsg, &errCode);
}

#endif
