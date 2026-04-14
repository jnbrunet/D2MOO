#include <imgui.h>
#include <vector>
#include "D2Debugger.h"
#include <GAME/Game.h>
#include <D2Lang.h>
#include <D2Unicode.h>
#include <D2Dll.h>
#include <D2DataTbls.h>
#include <D2StatList.h>
#include <D2Items.h>

#include "IconsFontAwesome6.h"

#include <Windows.h>

#if defined(D2_VERSION_110F)
#define HAS_SPAWN_FUNCTIONS
HMODULE delayedD2GameDllBaseGet()
{
    static HMODULE DLLBASE_D2Game = LoadLibraryA("D2Game.dll");
    return DLLBASE_D2Game;
}

static const int D2GameImageBase = 0x6FC30000;
D2FUNC(D2Game, SpawnSuperUnique_6FC6F690, D2UnitStrc*, __fastcall, (D2GameStrc* pGame, D2ActiveRoomStrc* pRoom, int32_t nX, int32_t nY, int32_t nSuperUnique), 0x6FC6F690 - D2GameImageBase);
D2FUNC(D2Game, SpawnMonster_6FC69F10, D2UnitStrc*, __fastcall, (D2GameStrc* pGame, D2ActiveRoomStrc* pRoom, int32_t nX, int32_t nY, int32_t nMonsterId, int32_t nAnimMode, int32_t a7, int16_t nFlags), 0x6FC69F10 - D2GameImageBase);
D2FUNC(D2Game, CreateItemUnit_6FC501A0, D2UnitStrc*, __fastcall, (D2UnitStrc* pPlayer, int32_t nItemId, D2GameStrc* pGame, int32_t nSpawnTarget, int32_t nQuality, int32_t bNoSockets, int32_t bNoEthereal, int32_t nItemLevel, int32_t bUseSeed, int32_t dwSeed, int32_t dwItemSeed), 0x6FC501A0 - D2GameImageBase);
D2FUNC(D2Game, DropItem_6FC52260, void, __fastcall, (D2GameStrc* pGame, D2UnitStrc* pUnit, D2UnitStrc* pItem, D2ActiveRoomStrc* pRoom, int32_t nX, int32_t nY), 0x6FC52260 - D2GameImageBase);
#elif defined(D2_VERSION_114D)
#else
#pragma message("Warning: Unsupported D2 version for D2Debugger")
#endif

// Using a define so that we break inline
#define AddDebugBreakButton() do{ if (ImGui::Button(ICON_FA_HAMMER)) { __debugbreak(); }; } while(false)

static bool gbGodModeEnabled = false;

bool D2DebuggerIsGodModeEnabled()
{
    return gbGodModeEnabled;
}

void D2DebuggerSetGodModeEnabled(bool bEnabled)
{
    gbGodModeEnabled = bEnabled;
}

std::vector<char> GetUTF8CharBufferFromStringIndex(uint16_t index)
{
    const Unicode* nameUnicode = (const Unicode*)D2LANG_GetStringFromTblIndex(index);
    std::vector<char> utf8CharBuffer(Unicode::strlen(nameUnicode) * 3, 0);
    Unicode::toUtf(utf8CharBuffer.data(), nameUnicode, utf8CharBuffer.size() - 2 /*See toUtf doc*/);
    return utf8CharBuffer;
}

static const char* gActNames[] = {
    "Act 1",
    "Act 2",
    "Act 3",
    "Act 4",
    "Act 5",
    "Act *", // Used as a sentinel value
};

template<typename ItFunction>
void DebutIterateUnitList(D2GameStrc* pGame, const D2C_UnitTypes nUnitType, ItFunction&& itFunc)
{
    ImGui::PushID(nUnitType);

    ImGui::Text("Filters:");

    static const int dwordInputSize = ImGui::CalcTextSize("-00000000").x;
    static int nClassIdFilter = -1;
    static D2UnitGUID nUnitGUIDFilter = D2UnitInvalidGUID;
    static int nActFilter = D2C_Acts::NUM_ACTS;

    ImGui::PushItemWidth(dwordInputSize);
    ImGui::SameLine(); ImGui::InputInt("ClassId", &nClassIdFilter, 0, 0);
    ImGui::SameLine(); ImGui::InputInt("GUID", (int*)&nUnitGUIDFilter, 0,0);
    ImGui::PopItemWidth();

    static const int ActComboWidth = ImGui::CalcTextSize("Act X").x + /*arrow*/ ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetNextItemWidth(ActComboWidth);
    ImGui::SameLine(); ImGui::Combo("##Act", (int*)&nActFilter, gActNames, ARRAY_SIZE(gActNames));

    // This is pretty crappy, we are forced to iterate all units to get the total number of units to display
    // This is more or less required as soon as we filter the list anyway...
    static std::vector<D2UnitStrc*> filteredUnits;
    filteredUnits.clear();
    filteredUnits.reserve(1024);

    D2UnitStrc** pUnitList = pGame->pUnitList[GAME_RemapUnitTypeToListIndex(nUnitType)];
    const int32_t nFirstHashIdx = (nUnitGUIDFilter != D2UnitInvalidGUID) ? (nUnitGUIDFilter & 0x7F) : 0;
    const int32_t nLastHashIdx = (nUnitGUIDFilter != D2UnitInvalidGUID) ? (nUnitGUIDFilter & 0x7F) : 0x7F;
    bool stopIteration = false;
    for (int32_t i = nFirstHashIdx; i <= nLastHashIdx && !stopIteration; ++i)
    {
        for (D2UnitStrc* pUnit = pUnitList[i];
            pUnit != nullptr && !stopIteration;
            pUnit = pUnit->pListNext)
        {
            if ((nUnitGUIDFilter != D2UnitInvalidGUID) && (pUnit->dwUnitId != nUnitGUIDFilter))
                continue;
            if ((nClassIdFilter >= 0) && (pUnit->dwClassId != nClassIdFilter))
                continue;
            if ((nActFilter != D2C_Acts::NUM_ACTS) && (pUnit->nAct != nActFilter))
                continue;

            if (nUnitGUIDFilter != D2UnitInvalidGUID)
            {
                // This is the only case where we can early out
                itFunc(pUnit);
                stopIteration = true;
            }
            else
            {
                filteredUnits.push_back(pUnit);
            }
        }
    }

    ImGui::Text("Count: %d", filteredUnits.size());
    if (ImGui::BeginChild("##ScrollingRegion", ImVec2(0, 500), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
    {
        ImGuiListClipper clipper;
        clipper.Begin(filteredUnits.size());
        while (clipper.Step() && !stopIteration)
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd && !stopIteration; i++)
            {
                if (!itFunc(filteredUnits[i]))
                {
                    stopIteration = true;
                }
            }
        }
        clipper.End();

    }
    ImGui::EndChild();
    ImGui::PopID();

}

void D2DebugUnitAnim(D2UnitStrc * pUnit)
{
    if (pUnit->pAnimData)
    {
        ImGui::BulletText("Name           %8s", pUnit->pAnimData->szAnimDataName);
        ImGui::BulletText("Frames         %d", pUnit->pAnimData->dwFrames);
        ImGui::BulletText("Base Speed     %d", pUnit->pAnimData->dwAnimSpeed);
    }
    ImGui::BulletText("Mode           %d", pUnit->dwAnimMode);
    ImGui::BulletText("Speed          %d", pUnit->wAnimSpeed);
    ImGui::BulletText("FrameCount     %d", pUnit->dwFrameCountPrecise);
    ImGui::BulletText("nActionFrame   %d", pUnit->nActionFrame);
    ImGui::BeginDisabled(pUnit->pAnimSeq == nullptr);
    ImGui::BulletText("Sequence Mode  %d", pUnit->dwSeqMode);
    ImGui::BulletText("Sequence Speed %d", pUnit->dwSeqSpeed);
    ImGui::BulletText("Sequence Frame %f", pUnit->nSeqCurrentFramePrecise / 256.f);
    ImGui::BulletText("Seq next frame %f", pUnit->dwSeqFrame / 256.f);
    ImGui::EndDisabled();
}


void D2DebugPath(D2StaticPathStrc* pStaticPath)
{

}


void D2DebugPath(D2DynamicPathStrc* pDynamicPath)
{
    ImGui::BulletText("Type=%d Flags=0x%x", pDynamicPath->dwPathType, pDynamicPath->dwFlags);
    ImGui::BulletText("Game  (X,Y)=(%5d,%5d)", pDynamicPath->tGameCoords.wPosX, pDynamicPath->tGameCoords.wPosY);
    ImGui::SameLine(); ImGui::Text("Client(X,Y)=(%5d,%5d)", pDynamicPath->dwClientCoordX, pDynamicPath->dwClientCoordY);
    ImGui::BulletText("Target(X,Y)=(%5d,%5d)", pDynamicPath->tTargetCoord.X, pDynamicPath->tTargetCoord.Y);
    if (pDynamicPath->tPrevTargetCoord != D2PathPointStrc{0,0})
        ImGui::BulletText("   SP2(X,Y)=(%5d,%5d)", pDynamicPath->tPrevTargetCoord.X, pDynamicPath->tPrevTargetCoord.Y);
    if (pDynamicPath->tFinalTargetCoord != D2PathPointStrc{0,0})
        ImGui::BulletText("   SP3(X,Y)=(%5d,%5d)", pDynamicPath->tFinalTargetCoord.X, pDynamicPath->tFinalTargetCoord.Y);
    ImGui::BulletText   ("Current point %d/%d", pDynamicPath->dwCurrentPointIdx, pDynamicPath->dwPathPoints);

}


void D2DebugUnitPath(D2UnitStrc* pUnit)
{
    switch (pUnit->dwUnitType)
    {
    case UNIT_OBJECT:
    case UNIT_ITEM:
    case UNIT_TILE:
        D2DebugPath(pUnit->pStaticPath);
        break;

    default:
        D2DebugPath(pUnit->pDynamicPath);
        break;
    }
}

const char* GetAlignmentString(D2C_UnitAlignment nAlignment)
{
    switch (nAlignment)
    {
    case UNIT_ALIGNMENT_EVIL: return "Evil";
    case UNIT_ALIGNMENT_NEUTRAL: return "Neutral";
    case UNIT_ALIGNMENT_GOOD: return "Good";
    case UNIT_NUM_ALIGNMENT: 
    case UNIT_ALIGNMENT_UNASSIGNED:
    default:
        return "Invalid alignment";
    }
}

void D2DebugUnitCommon(D2UnitStrc* pUnit)
{

    ImGui::Text("ClassId=%d", pUnit->dwClassId);
    ImGui::SameLine(); ImGui::Text("GUID=%d", pUnit->dwUnitId);
    ImGui::SameLine(); ImGui::Text("%s", gActNames[pUnit->nAct]);
    D2CoordStrc tCoords;
    UNITS_GetCoords(pUnit, &tCoords);
    ImGui::SameLine(); ImGui::Text("(X,Y)=(%5d,%5d)", tCoords.nX, tCoords.nY);
    if (pUnit->dwUnitType == UNIT_PLAYER || pUnit->dwUnitType == UNIT_MONSTER)
    {
        ImGui::SameLine(); ImGui::Text("| %s", GetAlignmentString((D2C_UnitAlignment)STATLIST_GetUnitAlignment(pUnit)));
    }
    ImGui::SeparatorText("Animation");
    D2DebugUnitAnim(pUnit);
    ImGui::SeparatorText("Path");
    D2DebugUnitPath(pUnit);
}

D2UnitStrc* GetFirstPlayerInList(D2GameStrc* pGame)
{
    for (D2UnitStrc* pUnitList : pGame->pUnitList[GAME_RemapUnitTypeToListIndex(UNIT_PLAYER)])
    {
        for (D2UnitStrc* pUnit = pUnitList;
            pUnit != nullptr;
            pUnit = pUnit->pListNext)
        {
            return pUnit;
        }
    }
    return nullptr;
}

const std::vector<char>& GetNOTFOUNDCharBuffer()
{
    static const char notFound[] = "NOT-FOUND";
    static std::vector<char> notFoundBuffer{notFound, notFound + sizeof(notFound)};
    return notFoundBuffer;
}

template<typename NAMEGETTER>
void D2ComboBox(const char* Title, int& selectedID, size_t count,NAMEGETTER&& NameGetter)
{

    static bool bWasComboOpen = false;
    if (ImGui::BeginCombo(Title, NameGetter(selectedID).data()))
    {
        ImGuiListClipper clipper;
        clipper.Begin(count);

        if (!bWasComboOpen)
        {
            bWasComboOpen = true;
            //This is needed on first frame for SetItemDefaultFocus to work
            clipper.ForceDisplayRangeByIndices(0, clipper.ItemsCount);
        }
        while (clipper.Step())
        {
            for (int id = clipper.DisplayStart; id < clipper.DisplayEnd; id++)
            {
                const bool bSelected = selectedID == id;
                if (ImGui::Selectable(NameGetter(id).data(), bSelected))
                    selectedID = id;
                if (bSelected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        clipper.End();
        ImGui::EndCombo();
    }
    else
    {
        bWasComboOpen = false;
    }
}

void D2DebugUnitSpawner(D2GameStrc* pGame)
{
#ifdef HAS_SPAWN_FUNCTIONS
    if (ImGui::CollapsingHeader("UnitSpawner"))
    {
        D2CoordStrc tCoords{};
        if (D2UnitStrc* pPlayer = GetFirstPlayerInList(pGame))
        {
            UNITS_GetCoords(pPlayer, &tCoords);
            ImGui::Text("Spawn location (player) = (%d,%d)", tCoords.nX, tCoords.nY);


            auto GetSuperUniqueUTF8Name = [](int id)
            {
                if (D2SuperUniquesTxt* pSuperUniqueRecord = DATATBLS_GetSuperUniquesTxtRecord(id))
                {
                    return GetUTF8CharBufferFromStringIndex(pSuperUniqueRecord->wNameStr);
                }
                return GetNOTFOUNDCharBuffer();
            };

            static int currentSuperUniqueSelectionId = 0;
            D2ComboBox("SuperUnique", currentSuperUniqueSelectionId, DATATBLS_GetSuperUniquesTxtRecordCount(), GetSuperUniqueUTF8Name);
			ImGui::SameLine();
            if (ImGui::Button("Spawn##SuperUnique"))
            {
                if (D2UnitStrc* pSpawned = D2Game_SpawnSuperUnique_6FC6F690(pGame, pPlayer->pDynamicPath->pRoom, tCoords.nX, tCoords.nY, currentSuperUniqueSelectionId))
                {
                    // Register for debug view ?
                }
                else
                {
                    ImGui::OpenPopup("Spawn failed");
                }
            }

            static int currentNormalSelectionId = 0;
            auto GetNormalMonsterUTF8Name = [](int id)
            {
                if (D2MonStatsTxt* pMonStatsTxtRecord = DATATBLS_GetMonStatsTxtRecord(id))
                {
                    return GetUTF8CharBufferFromStringIndex(pMonStatsTxtRecord->wNameStr);
                }
                return GetNOTFOUNDCharBuffer();
            };
            D2ComboBox("Normal", currentNormalSelectionId, DATATBLS_GetMonStatsTxtRecordCount(), GetNormalMonsterUTF8Name);

			ImGui::SameLine();
            if (ImGui::Button("Spawn##Normal"))
            {
                
                if (D2UnitStrc* pSpawned = D2Game_SpawnMonster_6FC69F10(pGame, pPlayer->pDynamicPath->pRoom, tCoords.nX, tCoords.nY, currentNormalSelectionId, MONMODE_NEUTRAL, 5, 0))
                {
                    // Register for debug view ?
                }
                else
                {
                    ImGui::OpenPopup("Spawn failed");
                }
            }



            bool bOpened_Unused = true;
            if (ImGui::BeginPopupModal("Spawn failed", &bOpened_Unused))
            {
                ImGui::Text("Spawning the monster failed. Possible issues:");
                ImGui::BulletText("Can only be spawned once per game.");
                ImGui::BulletText("Not engouh space left.");
                ImGui::BulletText("...");
                if (ImGui::Button("Close"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

        }
    }
#endif
}

static bool bFreezeGame = false;
static bool ContainsInsensitive(const char* pStr, const char* pSearch)
{
    if (!pSearch || pSearch[0] == '\0') return true;
    if (!pStr) return false;
    const size_t nSearchLen = strlen(pSearch);
    for (; *pStr; ++pStr)
    {
        if (_strnicmp(pStr, pSearch, nSearchLen) == 0)
            return true;
    }
    return false;
}

void D2DebugItemDropper(D2GameStrc* pGame)
{
#ifdef HAS_SPAWN_FUNCTIONS
    if (!ImGui::CollapsingHeader("Item Dropper"))
        return;

    D2UnitStrc* pPlayer = GetFirstPlayerInList(pGame);
    if (!pPlayer)
    {
        ImGui::TextDisabled("No player in game");
        return;
    }

    D2CoordStrc tCoords{};
    UNITS_GetCoords(pPlayer, &tCoords);
    ImGui::Text("Drop location (player): (%d, %d)", tCoords.nX, tCoords.nY);

    // --- Item type filter ---
    struct ItemTypeEntry { const char* szName; int nTypeId; };
    static const ItemTypeEntry gItemTypeFilters[] = {
        { "All",              -1                     },
        { "Any Armor",        ITEMTYPE_ANY_ARMOR      },
        { "Body Armor",       ITEMTYPE_ARMOR          },
        { "Helm",             ITEMTYPE_HELM           },
        { "Boots",            ITEMTYPE_BOOTS          },
        { "Gloves",           ITEMTYPE_GLOVES         },
        { "Belt",             ITEMTYPE_BELT           },
        { "Any Shield",       ITEMTYPE_ANY_SHIELD     },
        { "Shield",           ITEMTYPE_SHIELD         },
        { "Auric Shield",     ITEMTYPE_AURIC_SHIELDS  },
        { "Any Weapon",       ITEMTYPE_WEAPON         },
        { "Melee Weapon",     ITEMTYPE_MELEE_WEAPON   },
        { "Axe",              ITEMTYPE_AXE            },
        { "Sword",            ITEMTYPE_SWORD          },
        { "Club",             ITEMTYPE_CLUB           },
        { "Hammer",           ITEMTYPE_HAMMER         },
        { "Mace",             ITEMTYPE_MACE           },
        { "Knife",            ITEMTYPE_KNIFE          },
        { "Spear",            ITEMTYPE_SPEAR          },
        { "Polearm",          ITEMTYPE_POLEARM        },
        { "Scepter",          ITEMTYPE_SCEPTER        },
        { "Wand",             ITEMTYPE_WAND           },
        { "Staff",            ITEMTYPE_STAFF          },
        { "Orb",              ITEMTYPE_ORB            },
        { "Voodoo Head",      ITEMTYPE_VOODOO_HEADS   },
        { "Primal Helm",      ITEMTYPE_PRIMAL_HELM    },
        { "Pelt",             ITEMTYPE_PELT           },
        { "Cloak",            ITEMTYPE_CLOAK          },
        { "Circlet",          ITEMTYPE_CIRCLET        },
        { "Missile Weapon",   ITEMTYPE_MISSILE_WEAPON },
        { "Bow",              ITEMTYPE_BOW            },
        { "Crossbow",         ITEMTYPE_CROSSBOW       },
        { "Ring",             ITEMTYPE_RING           },
        { "Amulet",           ITEMTYPE_AMULET         },
        { "Charm",            ITEMTYPE_CHARM          },
        { "Rune",             ITEMTYPE_RUNE           },
        { "Gem",              ITEMTYPE_GEM            },
        { "Jewel",            ITEMTYPE_JEWEL          },
        { "Potion",           ITEMTYPE_POTION         },
        { "Miscellaneous",    ITEMTYPE_MISCELLANEOUS  },
    };

    // --- Quality ---
    struct QualityEntry { const char* szName; D2C_ItemQualities eQuality; };
    static const QualityEntry gQualities[] = {
        { "Normal",   ITEMQUAL_NORMAL   },
        { "Superior", ITEMQUAL_SUPERIOR },
        { "Magic",    ITEMQUAL_MAGIC    },
        { "Rare",     ITEMQUAL_RARE     },
        { "Unique",   ITEMQUAL_UNIQUE   },
        { "Set",      ITEMQUAL_SET      },
    };

    static int nTypeFilterIdx   = 0;
    static int nQualityIdx      = 0;
    static int nItemLevel       = 99;
    static int nSelectedItemId  = 0;
    static char szSearch[64]    = {};

    // Row 1: type, quality, item level
    ImGui::SetNextItemWidth(150.f);
    if (ImGui::BeginCombo("Type##ItemDrop", gItemTypeFilters[nTypeFilterIdx].szName))
    {
        for (int i = 0; i < ARRAY_SIZE(gItemTypeFilters); ++i)
        {
            const bool bSel = (nTypeFilterIdx == i);
            if (ImGui::Selectable(gItemTypeFilters[i].szName, bSel))
                nTypeFilterIdx = i;
            if (bSel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::BeginCombo("Quality##ItemDrop", gQualities[nQualityIdx].szName))
    {
        for (int i = 0; i < ARRAY_SIZE(gQualities); ++i)
        {
            const bool bSel = (nQualityIdx == i);
            if (ImGui::Selectable(gQualities[i].szName, bSel))
                nQualityIdx = i;
            if (bSel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.f);
    ImGui::InputInt("ilvl##ItemDrop", &nItemLevel, 0, 0);
    if (nItemLevel < 1)  nItemLevel = 1;
    if (nItemLevel > 99) nItemLevel = 99;

    // Row 2: text search
    ImGui::SetNextItemWidth(300.f);
    ImGui::InputText("Search##ItemDrop", szSearch, sizeof(szSearch));

    // Build filtered item list every frame (debug tool, perf not critical)
    static std::vector<int> s_FilteredItems;
    s_FilteredItems.clear();

    const D2ItemDataTbl* pItemTbl = DATATBLS_GetItemDataTables();
    const int nTypeId = gItemTypeFilters[nTypeFilterIdx].nTypeId;

    for (int i = 0; i < pItemTbl->nItemsTxtRecordCount; ++i)
    {
        D2ItemsTxt* pRec = DATATBLS_GetItemsTxtRecord(i);
        if (!pRec)
            continue;

        // Type filter
        if (nTypeId >= 0 && !ITEMS_CheckItemTypeIdByItemId(i, nTypeId))
            continue;

        // Name search filter
        if (szSearch[0] != '\0')
        {
            auto nameBuf = GetUTF8CharBufferFromStringIndex(pRec->wNameStr);
            if (!ContainsInsensitive(nameBuf.data(), szSearch))
                continue;
        }

        s_FilteredItems.push_back(i);
    }

    // Row 3: item combo + drop button
    // Build preview string for currently selected item
    static std::vector<char> s_PreviewBuf;
    {
        D2ItemsTxt* pRec = DATATBLS_GetItemsTxtRecord(nSelectedItemId);
        if (pRec)
        {
            auto tmp = GetUTF8CharBufferFromStringIndex(pRec->wNameStr);
            // Format as "Name (CODE)"
            char szCode[5] = { pRec->szCode[0], pRec->szCode[1], pRec->szCode[2], pRec->szCode[3], '\0' };
            // Trim trailing spaces from code
            for (int k = 3; k >= 0 && szCode[k] == ' '; --k) szCode[k] = '\0';
            const int nNeeded = (int)(tmp.size() + 8);
            s_PreviewBuf.resize(nNeeded);
            _snprintf_s(s_PreviewBuf.data(), nNeeded, _TRUNCATE, "%s (%s)", tmp.data(), szCode);
        }
        else
        {
            static const char szNone[] = "(select an item)";
            s_PreviewBuf.assign(szNone, szNone + sizeof(szNone));
        }
    }

    ImGui::Text("Matches: %d", (int)s_FilteredItems.size());
    ImGui::SetNextItemWidth(350.f);
    if (ImGui::BeginCombo("Base Item##ItemDrop", s_PreviewBuf.data()))
    {
        ImGuiListClipper clipper;
        clipper.Begin((int)s_FilteredItems.size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                const int nItemId = s_FilteredItems[i];
                D2ItemsTxt* pRec = DATATBLS_GetItemsTxtRecord(nItemId);
                if (!pRec) continue;

                auto nameBuf = GetUTF8CharBufferFromStringIndex(pRec->wNameStr);
                char szCode[5] = { pRec->szCode[0], pRec->szCode[1], pRec->szCode[2], pRec->szCode[3], '\0' };
                for (int k = 3; k >= 0 && szCode[k] == ' '; --k) szCode[k] = '\0';

                char szLabel[256];
                _snprintf_s(szLabel, sizeof(szLabel), _TRUNCATE, "%s (%s)##%d", nameBuf.data(), szCode, nItemId);

                const bool bSel = (nSelectedItemId == nItemId);
                if (ImGui::Selectable(szLabel, bSel))
                    nSelectedItemId = nItemId;
                if (bSel)
                    ImGui::SetItemDefaultFocus();
            }
        }
        clipper.End();
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Drop##ItemDrop"))
    {
        if (!pPlayer->pDynamicPath)
        {
            ImGui::OpenPopup("ItemDropFailed##ItemDrop");
        }
        else
        {
            D2UnitStrc* pItem = D2Game_CreateItemUnit_6FC501A0(
                pPlayer, nSelectedItemId, pGame,
                /*nSpawnTarget=*/3,
                (int32_t)gQualities[nQualityIdx].eQuality,
                /*bNoSockets=*/0, /*bNoEthereal=*/0,
                nItemLevel,
                /*bUseSeed=*/0, /*dwSeed=*/0, /*dwItemSeed=*/0);
            if (pItem)
            {
                D2Game_DropItem_6FC52260(pGame, pPlayer, pItem,
                    pPlayer->pDynamicPath->pRoom, tCoords.nX, tCoords.nY);
            }
            else
            {
                ImGui::OpenPopup("ItemDropFailed##ItemDrop");
            }
        }
    }

    bool bPopupOpen = true;
    if (ImGui::BeginPopupModal("ItemDropFailed##ItemDrop", &bPopupOpen))
    {
        ImGui::Text("Item creation failed.");
        ImGui::BulletText("The item/quality combination may be invalid.");
        ImGui::BulletText("The item may not support the selected quality.");
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
#endif
}

bool D2DebugGame(D2GameStrc* pGame)
{
    if(ImGui::Begin("Game"))
    {
        ImGui::Checkbox("God mode", &gbGodModeEnabled);

        if (*pGame->szGameName)
            ImGui::Text("Game: '%s'", pGame->szGameName);
        if (*pGame->szGameDesc)
            ImGui::Text("Desc: '%s'", pGame->szGameDesc);
        if (*pGame->szGamePassword)
            ImGui::Text("Password: '%s'", pGame->szGamePassword);

        ImGui::Text(pGame->bExpansion ? "Expansion" : "Classic");
        ImGui::SameLine();
        ImGui::Text(pGame->dwGameType ? "(Ladder)" : "(Non-Ladder)");
        ImGui::Text("Difficulty:"); ImGui::SameLine();
        switch (pGame->nDifficulty)
        {
        case DIFFMODE_NORMAL:ImGui::Text("Normal"); break;
        case DIFFMODE_NIGHTMARE:ImGui::Text("Nightmare"); break;
        case DIFFMODE_HELL:ImGui::Text("Hell"); break;
		default: D2_UNREACHABLE;
        }
        ImGui::Text("Init seed: 0x%x", pGame->dwInitSeed);
        ImGui::Text("Frame %d", pGame->dwGameFrame);
        ImGui::SameLine();
        if(ImGui::SmallButton(bFreezeGame ? ICON_FA_PLAY : ICON_FA_PAUSE)) bFreezeGame = !bFreezeGame;

        ImGui::Separator();
        ImGui::Text("Last used GUID");
        ImGui::BulletText("Players: %d", pGame->dwLastUsedUnitGUID[UNIT_PLAYER]);
        ImGui::BulletText("Monsters: %d", pGame->dwLastUsedUnitGUID[UNIT_MONSTER]);
        ImGui::BulletText("Objects: %d", pGame->dwLastUsedUnitGUID[UNIT_OBJECT]);
        ImGui::BulletText("Missiles: %d", pGame->dwLastUsedUnitGUID[UNIT_MISSILE]);
        ImGui::BulletText("Items: %d", pGame->dwLastUsedUnitGUID[UNIT_ITEM]);
        ImGui::BulletText("Tiles: %d", pGame->dwLastUsedUnitGUID[UNIT_TILE]);

        D2DebugUnitSpawner(pGame);
        D2DebugItemDropper(pGame);

        if (ImGui::CollapsingHeader("Players units"))
        {
            ImGui::Indent();
            DebutIterateUnitList(pGame, UNIT_PLAYER, [&](D2UnitStrc* pPlayer)
                {
                    AddDebugBreakButton();
                    ImGui::SameLine();
                    ImGui::SeparatorText(pPlayer->pPlayerData->szName);

                    D2DebugUnitCommon(pPlayer);
                    return true;
                }
            );
            ImGui::Unindent();
        }
        if (ImGui::CollapsingHeader("Monster units"))
        {
            ImGui::Indent();
            DebutIterateUnitList(pGame, UNIT_MONSTER, [](D2UnitStrc* pMonster)
                {
                    ImGui::Separator();
                    AddDebugBreakButton();
                    ImGui::SameLine();
                    D2DebugUnitCommon(pMonster);
                    return true;
                }
            );
            ImGui::Unindent();
        }
        if (ImGui::CollapsingHeader("Object units"))
        {
            ImGui::Indent();
            DebutIterateUnitList(pGame, UNIT_OBJECT, [](D2UnitStrc* pObject)
                {
                    ImGui::Separator();
                    AddDebugBreakButton();
                    ImGui::SameLine();
                    D2DebugUnitCommon(pObject);
                    return true;
                }
            );
            ImGui::Unindent();
        }
        if (ImGui::CollapsingHeader("Missile units"))
        {
            ImGui::Indent();
            DebutIterateUnitList(pGame, UNIT_MISSILE, [](D2UnitStrc* pMissile)
                {
                    ImGui::Separator();
                    AddDebugBreakButton();
                    ImGui::SameLine();
                    D2DebugUnitCommon(pMissile);
                    return true;
                }
            );
            ImGui::Unindent();
        }
        if (ImGui::CollapsingHeader("Item units"))
        {
            ImGui::Indent();
            DebutIterateUnitList(pGame, UNIT_ITEM, [](D2UnitStrc* pItem)
                {
                    ImGui::Separator();
                    AddDebugBreakButton();
                    ImGui::SameLine();
                    D2DebugUnitCommon(pItem);
                    return true;
                }
            );
            ImGui::Unindent();
        }

#if 0
        // No debugging for clients until we access the gClientListLock_6FD447D0 properly
        // This means knowing wether we use the one from the original game or D2Moo
        ImGui::Text("Clients");
        D2_LOCK(&gClientListLock_6FD447D0);

        for (D2ClientStrc* pCurrentClient = pGame->pClientList;
            pCurrentClient != nullptr;
            pCurrentClient = pCurrentClient->pNextByName)
        {
            ImGui::Text("Name:%16s", pCurrentClient->szName);
            ImGui::Text("Account:%16s", pCurrentClient->szAccount);
        }
        D2_UNLOCK(&gClientListLock_6FD447D0);
#endif
    }
    ImGui::End();

    return bFreezeGame;
}
