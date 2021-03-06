/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OBJECTMGR_H
#define _OBJECTMGR_H

#include "AreaTrigger.h"
#include "Log.h"
#include "Object.h"
#include "Bag.h"
#include "Creature.h"
#include "Player.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "Corpse.h"
#include "QuestDef.h"
#include "ItemPrototype.h"
#include "NPCHandler.h"
#include "DatabaseEnv.h"
#include "Mail.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include <ace/Singleton.h>
#include "VehicleDefines.h"
#include <string>
#include <map>
#include <limits>
#include "ConditionMgr.h"
#include <functional>
#include "PhaseMgr.h"
#include <LockedMap.h>

class Item;
class PhaseMgr;

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push, N), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

struct GuidDiapason
{
    uint32 m_begin = 0;
    uint32 m_end = 0;

    bool IsActive() { return m_end != 0; }
    void Change(uint32 begin, uint32 end) { m_begin = begin; m_end = end; }
};

struct PageText
{
    std::string Text;
    uint16 NextPage;
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

// Benchmarked: Faster than std::unordered_map (insert/find)
typedef std::map<uint32, PageText> PageTextContainer;

// Benchmarked: Faster than std::map (insert/find)
typedef std::unordered_map<uint16, InstanceTemplate> InstanceTemplateContainer;

struct GameTele
{
    float  position_x;
    float  position_y;
    float  position_z;
    float  orientation;
    uint32 mapId;
    std::string name;
    std::wstring wnameLow;
};

typedef std::unordered_map<uint32, GameTele > GameTeleContainer;

enum ScriptsType
{
    SCRIPTS_FIRST = 1,

    SCRIPTS_QUEST_END = SCRIPTS_FIRST,
    SCRIPTS_QUEST_START,
    SCRIPTS_SPELL,
    SCRIPTS_GAMEOBJECT,
    SCRIPTS_EVENT,
    SCRIPTS_WAYPOINT,

    SCRIPTS_LAST
};

enum eScriptFlags
{
    // Talk Flags
    SF_TALK_USE_PLAYER          = 0x1,

    // Emote flags
    SF_EMOTE_USE_STATE          = 0x1,

    // TeleportTo flags
    SF_TELEPORT_USE_CREATURE    = 0x1,

    // KillCredit flags
    SF_KILLCREDIT_REWARD_GROUP  = 0x1,

    // RemoveAura flags
    SF_REMOVEAURA_REVERSE       = 0x1,

    // CastSpell flags
    SF_CASTSPELL_SOURCE_TO_TARGET = 0,
    SF_CASTSPELL_SOURCE_TO_SOURCE = 1,
    SF_CASTSPELL_TARGET_TO_TARGET = 2,
    SF_CASTSPELL_TARGET_TO_SOURCE = 3,
    SF_CASTSPELL_SEARCH_CREATURE  = 4,
    SF_CASTSPELL_TRIGGERED      = 0x1,

    // PlaySound flags
    SF_PLAYSOUND_TARGET_PLAYER  = 0x1,
    SF_PLAYSOUND_DISTANCE_SOUND = 0x2,

    // Orientation flags
    SF_ORIENTATION_FACE_TARGET  = 0x1,
};

struct ScriptInfo
{
    ScriptsType type;
    uint32 id;
    uint32 delay;
    ScriptCommands command;

    union
    {
        struct
        {
            uint32 nData[3];
            float  fData[4];
        } Raw;

        struct                      // SCRIPT_COMMAND_TALK (0)
        {
            uint32 ChatType;        // datalong
            uint32 Flags;           // datalong2
            int32  TextID;          // dataint
        } Talk;

        struct                      // SCRIPT_COMMAND_EMOTE (1)
        {
            uint32 EmoteID;         // datalong
            uint32 Flags;           // datalong2
        } Emote;

        struct                      // SCRIPT_COMMAND_FIELD_SET (2)
        {
            uint32 FieldID;         // datalong
            uint32 FieldValue;      // datalong2
        } FieldSet;

        struct                      // SCRIPT_COMMAND_MOVE_TO (3)
        {
            uint32 Unused1;         // datalong
            uint32 TravelTime;      // datalong2
            int32  Unused2;         // dataint

            float DestX;
            float DestY;
            float DestZ;
        } MoveTo;

        struct                      // SCRIPT_COMMAND_FLAG_SET (4)
                                    // SCRIPT_COMMAND_FLAG_REMOVE (5)
        {
            uint32 FieldID;         // datalong
            uint32 FieldValue;      // datalong2
        } FlagToggle;

        struct                      // SCRIPT_COMMAND_TELEPORT_TO (6)
        {
            uint32 MapID;           // datalong
            uint32 Flags;           // datalong2
            int32  Unused1;         // dataint

            float DestX;
            float DestY;
            float DestZ;
            float Orientation;
        } TeleportTo;

        struct                      // SCRIPT_COMMAND_QUEST_EXPLORED (7)
        {
            uint32 QuestID;         // datalong
            uint32 Distance;        // datalong2
        } QuestExplored;

        struct                      // SCRIPT_COMMAND_KILL_CREDIT (8)
        {
            uint32 CreatureEntry;   // datalong
            uint32 Flags;           // datalong2
        } KillCredit;

        struct                      // SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (9)
        {
            uint32 GOGuid;          // datalong
            uint32 DespawnDelay;    // datalong2
        } RespawnGameobject;

        struct                      // SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (10)
        {
            uint32 CreatureEntry;   // datalong
            uint32 DespawnDelay;    // datalong2
            int32  Unused1;         // dataint

            float PosX;
            float PosY;
            float PosZ;
            float Orientation;
        } TempSummonCreature;

        struct                      // SCRIPT_COMMAND_CLOSE_DOOR (12)
                                    // SCRIPT_COMMAND_OPEN_DOOR (11)
        {
            uint32 GOGuid;          // datalong
            uint32 ResetDelay;      // datalong2
        } ToggleDoor;

                                    // SCRIPT_COMMAND_ACTIVATE_OBJECT (13)

        struct                      // SCRIPT_COMMAND_REMOVE_AURA (14)
        {
            uint32 SpellID;         // datalong
            uint32 Flags;           // datalong2
        } RemoveAura;

        struct                      // SCRIPT_COMMAND_CAST_SPELL (15)
        {
            uint32 SpellID;         // datalong
            uint32 Flags;           // datalong2
            int32  CreatureEntry;   // dataint

            float SearchRadius;
        } CastSpell;

        struct                      // SCRIPT_COMMAND_PLAY_SOUND (16)
        {
            uint32 SoundID;         // datalong
            uint32 Flags;           // datalong2
        } PlaySound;

        struct                      // SCRIPT_COMMAND_CREATE_ITEM (17)
        {
            uint32 ItemEntry;       // datalong
            uint32 Amount;          // datalong2
        } CreateItem;

        struct                      // SCRIPT_COMMAND_DESPAWN_SELF (18)
        {
            uint32 DespawnDelay;    // datalong
        } DespawnSelf;

        struct                      // SCRIPT_COMMAND_LOAD_PATH (20)
        {
            uint32 PathID;          // datalong
            uint32 IsRepeatable;    // datalong2
        } LoadPath;

        struct                      // SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT (21)
        {
            uint32 CreatureEntry;   // datalong
            uint32 ScriptID;        // datalong2
            uint32 ScriptType;      // dataint
        } CallScript;

        struct                      // SCRIPT_COMMAND_KILL (22)
        {
            uint32 Unused1;         // datalong
            uint32 Unused2;         // datalong2
            int32  RemoveCorpse;    // dataint
        } Kill;

        struct                      // SCRIPT_COMMAND_ORIENTATION (30)
        {
            uint32 Flags;           // datalong
            uint32 Unused1;         // datalong2
            int32  Unused2;         // dataint

            float Unused3;
            float Unused4;
            float Unused5;
            float Orientation;
        } Orientation;

        struct                      // SCRIPT_COMMAND_EQUIP (31)
        {
            uint32 EquipmentID;     // datalong
        } Equip;

        struct                      // SCRIPT_COMMAND_MODEL (32)
        {
            uint32 ModelID;         // datalong
        } Model;

                                    // SCRIPT_COMMAND_CLOSE_GOSSIP (33)

        struct                      // SCRIPT_COMMAND_PLAYMOVIE (34)
        {
            uint32 MovieID;         // datalong
        } PlayMovie;
    };

    std::string GetDebugInfo() const;
};

typedef std::multimap<uint32, ScriptInfo> ScriptMap;
typedef std::map<uint32, ScriptMap > ScriptMapMap;
typedef std::multimap<uint32, uint32> SpellScriptsContainer;
typedef std::pair<SpellScriptsContainer::iterator, SpellScriptsContainer::iterator> SpellScriptsBounds;
typedef std::multimap<uint32, uint32> AchievementScriptsContainer;
typedef std::pair<AchievementScriptsContainer::iterator, AchievementScriptsContainer::iterator> AchievementScriptsBounds;
extern ScriptMapMap sQuestEndScripts;
extern ScriptMapMap sQuestStartScripts;
extern ScriptMapMap sSpellScripts;
extern ScriptMapMap sGameObjectScripts;
extern ScriptMapMap sEventScripts;
extern ScriptMapMap sWaypointScripts;

enum VisibleDistanceType
{
    TYPE_VISIBLE_MAP  = 0,
    TYPE_VISIBLE_ZONE = 1,
    TYPE_VISIBLE_AREA = 2,
    TYPE_VISIBLE_MAX  = 3,
};

typedef std::unordered_map<uint32 /*id*/, float /*distance*/> VisibleDistanceMap;
extern VisibleDistanceMap sVisibleDistance[TYPE_VISIBLE_MAX];
float GetVisibleDistance(uint32 type, uint32 id);

std::string GetScriptsTableNameByType(ScriptsType type);
ScriptMapMap* GetScriptsMapByType(ScriptsType type);
std::string GetScriptCommandName(ScriptCommands command);

struct SpellClickInfo
{
    uint32 spellId;
    uint8 castFlags;
    SpellClickUserTypes userType;

    // helpers
    bool IsFitToRequirements(Unit const* clicker, Unit const* clickee) const;
};

typedef std::multimap<uint32, SpellClickInfo> SpellClickInfoContainer;
typedef std::pair<SpellClickInfoContainer::const_iterator, SpellClickInfoContainer::const_iterator> SpellClickInfoMapBounds;

struct AreaTriggerStruct
{
    uint32 target_mapId;
    float  target_X;
    float  target_Y;
    float  target_Z;
    float  target_Orientation;
};

typedef std::set<uint32> CellGuidSet;
typedef std::map<uint32/*player guid*/, uint32/*instance*/> CellCorpseMap;
struct CellObjectGuids
{
    CellGuidSet creatures;
    CellGuidSet gameobjects;
    CellCorpseMap corpses;
};
typedef std::unordered_map<uint32/*cell_id*/, CellObjectGuids> CellObjectGuidsMap;
typedef std::unordered_map<uint32/*(mapid, spawnMode) pair*/, CellObjectGuidsMap> MapObjectGuids;

// Trinity string ranges
#define MIN_TRINITY_STRING_ID           1                    // 'trinity_string'
#define MAX_TRINITY_STRING_ID           2000000000
#define MIN_DB_SCRIPT_STRING_ID        MAX_TRINITY_STRING_ID // 'db_script_string'
#define MAX_DB_SCRIPT_STRING_ID        2000010000
#define MIN_CREATURE_AI_TEXT_STRING_ID (-1)                 // 'creature_ai_texts'
#define MAX_CREATURE_AI_TEXT_STRING_ID (-1000000)

// Trinity Trainer Reference start range
#define TRINITY_TRAINER_START_REF      200000

struct TrinityStringLocale
{
    StringVector Content;
};

struct PersonalLootData
{
    uint32 entry;
    uint8 type;
    uint8 chance;
    uint32 lootspellId;
    uint32 bonusspellId;
    uint32 cooldownid;
    uint8 cooldowntype;
    uint8 respawn;
};

enum PersonalRespawnType
{
    TYPE_NORESPAWN      = 0,
    TYPE_RESPAWN        = 1,
    TYPE_NODESPAWN      = 2
};

typedef std::map<uint64, uint64> LinkedRespawnContainer;
typedef std::unordered_map<uint32, CreatureData> CreatureDataContainer;
typedef std::unordered_map<uint32, GameObjectData> GameObjectDataContainer;
typedef std::unordered_map<uint32, CreatureLocale> CreatureLocaleContainer;
typedef std::unordered_map<uint32, GameObjectLocale> GameObjectLocaleContainer;
typedef std::unordered_map<uint32, ItemLocale> ItemLocaleContainer;
typedef std::unordered_map<uint32, QuestLocale> QuestLocaleContainer;
typedef std::unordered_map<uint32, NpcTextLocale> NpcTextLocaleContainer;
typedef std::unordered_map<uint32, PageTextLocale> PageTextLocaleContainer;
typedef std::unordered_map<int32, TrinityStringLocale> TrinityStringLocaleContainer;
typedef std::unordered_map<uint32, GossipMenuItemsLocale> GossipMenuItemsLocaleContainer;
typedef std::unordered_map<uint32, PointOfInterestLocale> PointOfInterestLocaleContainer;
typedef std::unordered_map<uint32, PersonalLootData> PersonalLootContainer;
typedef std::unordered_map<uint32, std::vector<WorldLocation> > InstanceGraveYardContainer;

typedef std::multiset<uint32> QuestObject;
typedef std::map<uint32, QuestObject> QuestStarter;
typedef std::multimap<uint32, uint32> QuestRelations;
typedef std::pair<QuestRelations::const_iterator, QuestRelations::const_iterator> QuestRelationBounds;

struct PetStats
{
    float hp;
    float ap;
    int32 ap_type;
    float spd;
    int32 school_mask;
    int32 state;
    int32 energy;
    int32 energy_type;
    float armor;
    float damage;
    int32 type;
    int32 maxspdorap;
    int32 haste;
};

struct MailLevelReward
{
    MailLevelReward() : raceMask(0), mailTemplateId(0), senderEntry(0) {}
    MailLevelReward(uint32 _raceMask, uint32 _mailTemplateId, uint32 _senderEntry) : raceMask(_raceMask), mailTemplateId(_mailTemplateId), senderEntry(_senderEntry) {}

    uint32 raceMask;
    uint32 mailTemplateId;
    uint32 senderEntry;
};

typedef std::list<MailLevelReward> MailLevelRewardList;
typedef std::unordered_map<uint8, MailLevelRewardList> MailLevelRewardContainer;

// We assume the rate is in general the same for all three types below, but chose to keep three for scalability and customization
struct RepRewardRate
{
    float quest_rate;                                       // We allow rate = 0.0 in database. For this case, it means that
    float creature_rate;                                    // no reputation are given at all for this faction/rate type.
    float spell_rate;
};

struct ReputationOnKillEntry
{
    uint32 RepFaction1;
    uint32 RepFaction2;
    uint32 ReputationMaxCap1;
    int32 RepValue1;
    uint32 ReputationMaxCap2;
    int32 RepValue2;
    bool IsTeamAward1;
    bool IsTeamAward2;
    bool TeamDependent;
};

struct RepSpilloverTemplate
{
    uint32 faction[MAX_SPILLOVER_FACTIONS];
    float faction_rate[MAX_SPILLOVER_FACTIONS];
    uint32 faction_rank[MAX_SPILLOVER_FACTIONS];
};

struct PointOfInterest
{
    uint32 entry;
    float x;
    float y;
    uint32 icon;
    uint32 flags;
    uint32 data;
    std::string icon_name;
};

struct GossipMenuItems
{
    uint32          MenuId;
    uint32          OptionIndex;
    uint8           OptionIcon;
    std::string     OptionText;
    uint32          OptionType;
    uint32          OptionNpcflag;
    uint32          ActionMenuId;
    uint32          ActionPoiId;
    bool            BoxCoded;
    uint32          BoxMoney;
    std::string     BoxText;
    ConditionList   Conditions;
};

struct GossipMenus
{
    uint32          entry;
    uint32          text_id;
    ConditionList   conditions;
};

typedef std::multimap<uint32, GossipMenus> GossipMenusContainer;
typedef std::pair<GossipMenusContainer::const_iterator, GossipMenusContainer::const_iterator> GossipMenusMapBounds;
typedef std::pair<GossipMenusContainer::iterator, GossipMenusContainer::iterator> GossipMenusMapBoundsNonConst;
typedef std::multimap<uint32, GossipMenuItems> GossipMenuItemsContainer;
typedef std::pair<GossipMenuItemsContainer::const_iterator, GossipMenuItemsContainer::const_iterator> GossipMenuItemsMapBounds;
typedef std::pair<GossipMenuItemsContainer::iterator, GossipMenuItemsContainer::iterator> GossipMenuItemsMapBoundsNonConst;

struct QuestPOIPoint
{
    int32 x;
    int32 y;

    QuestPOIPoint() : x(0), y(0) {}
    QuestPOIPoint(int32 _x, int32 _y) : x(_x), y(_y) {}
};

struct QuestPOI
{
    uint32 Id;
    int32 ObjectiveIndex;
    uint32 MapId;
    uint32 AreaId;
    uint32 Unk2;
    uint32 Unk3;
    uint32 Unk4;
    std::vector<QuestPOIPoint> points;

    QuestPOI() : Id(0), ObjectiveIndex(0), MapId(0), AreaId(0), Unk2(0), Unk3(0), Unk4(0) {}
    QuestPOI(uint32 id, int32 objIndex, uint32 mapId, uint32 areaId, uint32 unk2, uint32 unk3, uint32 unk4) : Id(id), ObjectiveIndex(objIndex), MapId(mapId), AreaId(areaId), Unk2(unk2), Unk3(unk3), Unk4(unk4) {}
};

typedef std::vector<QuestPOI> QuestPOIVector;
typedef std::unordered_map<uint32, QuestPOIVector> QuestPOIContainer;

struct ScenarioPOIPoint
{
    int32 x;
    int32 y;

    ScenarioPOIPoint() : x(0), y(0) {}
    ScenarioPOIPoint(int32 _x, int32 _y) : x(_x), y(_y) {}
};

struct ScenarioPOI
{
    uint32 Id;
    uint32 MapId;
    uint32 WorldMapAreaId;
    uint32 Floor;
    uint32 Unk16;
    uint32 Unk20;
    uint32 WorldEffectID;
    uint32 Unk28;
    std::vector<ScenarioPOIPoint> points;

    ScenarioPOI() : Id(0), MapId(0), WorldMapAreaId(0), Floor(0), Unk16(0), Unk20(0), WorldEffectID(0), Unk28(0) {}
    ScenarioPOI(uint32 _Id, uint32 _MapId, uint32 _WorldMapAreaId, uint32 _Floor, uint32 _Unk16, uint32 _Unk20, uint32 _WorldEffectID, uint32 _Unk28) :
        Id(_Id), MapId(_MapId), WorldMapAreaId(_WorldMapAreaId), Floor(_Floor), Unk16(_Unk16), Unk20(_Unk20), WorldEffectID(_WorldEffectID), Unk28(_Unk28) { }
};

typedef std::vector<ScenarioPOI> ScenarioPOIVector;
typedef std::unordered_map<uint32, ScenarioPOIVector> ScenarioPOIContainer;

struct GraveYardData
{
    uint32 safeLocId;
    uint32 team;
};

typedef std::multimap<uint32, GraveYardData> GraveYardContainer;

typedef std::unordered_map<uint32, VendorItemData> CacheVendorItemContainer;
typedef std::unordered_map<uint32, TrainerSpellData> CacheTrainerSpellContainer;

enum SkillRangeType
{
    SKILL_RANGE_LANGUAGE,                                   // 300..300
    SKILL_RANGE_LEVEL,                                      // 1..max skill for level
    SKILL_RANGE_MONO,                                       // 1..1, grey monolite bar
    SKILL_RANGE_RANK,                                       // 1..skill for known rank
    SKILL_RANGE_NONE,                                       // 0..0 always
};

SkillRangeType GetSkillRangeType(SkillLineEntry const* pSkill, bool racial);

#define MAX_PLAYER_NAME          12                         // max allowed by client name length
#define MAX_INTERNAL_PLAYER_NAME 15                         // max server internal player name length (> MAX_PLAYER_NAME for support declined names)
#define MAX_PET_NAME             12                         // max allowed by client name length
#define MAX_CHARTER_NAME         24                         // max allowed by client name length

bool normalizePlayerName(std::string& name);

struct LanguageDesc
{
    Language lang_id;
    uint32   spell_id;
    uint32   skill_id;
};


#define LANGUAGE_DESC_COUNT 24
extern LanguageDesc lang_description[LANGUAGE_DESC_COUNT];
LanguageDesc const* GetLanguageDescByID(uint32 lang);

struct DungeonEncounter
{
    DungeonEncounter(DungeonEncounterEntry const* _dbcEntry, EncounterCreditType _creditType, uint32 _creditEntry, uint32 _lastEncounterDungeon)
        : dbcEntry(_dbcEntry), creditType(_creditType), creditEntry(_creditEntry), lastEncounterDungeon(_lastEncounterDungeon) { }

    DungeonEncounterEntry const* dbcEntry;
    EncounterCreditType creditType;
    uint32 creditEntry;
    uint32 lastEncounterDungeon;
};

typedef std::list<DungeonEncounter const*> DungeonEncounterList;
typedef std::unordered_map<uint32, DungeonEncounterList> DungeonEncounterContainer;

struct HotfixInfo
{
    uint32 Type;
    uint32 Timestamp;
    uint32 Entry;
};

typedef std::vector<HotfixInfo> HotfixData;
typedef std::map<uint32, bool> UpdateSkipData;

typedef std::map<uint32, AreaTriggerInfo > AreaTriggerInfoMap;

class PlayerDumpReader;

struct ItemSpecStats
{
    uint32 ItemType;
    uint32 ItemSpecStatTypes[MAX_ITEM_PROTO_STATS];
    uint32 ItemSpecStatCount;

    ItemSpecStats(ItemEntry const* item, ItemSparseEntry const* sparse) : ItemType(0), ItemSpecStatCount(0)
    {
        memset(ItemSpecStatTypes, -1, sizeof(ItemSpecStatTypes));

        if (item->Class == ITEM_CLASS_WEAPON)
        {
            ItemType = 5;
            switch (item->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:
                    AddStat(ITEM_SPEC_STAT_ONE_HANDED_AXE);
                    break;
                case ITEM_SUBCLASS_WEAPON_AXE2:
                    AddStat(ITEM_SPEC_STAT_TWO_HANDED_AXE);
                    break;
                case ITEM_SUBCLASS_WEAPON_BOW:
                    AddStat(ITEM_SPEC_STAT_BOW);
                    break;
                case ITEM_SUBCLASS_WEAPON_GUN:
                    AddStat(ITEM_SPEC_STAT_GUN);
                    break;
                case ITEM_SUBCLASS_WEAPON_MACE:
                    AddStat(ITEM_SPEC_STAT_ONE_HANDED_MACE);
                    break;
                case ITEM_SUBCLASS_WEAPON_MACE2:
                    AddStat(ITEM_SPEC_STAT_TWO_HANDED_MACE);
                    break;
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                    AddStat(ITEM_SPEC_STAT_POLEARM);
                    break;
                case ITEM_SUBCLASS_WEAPON_SWORD:
                    AddStat(ITEM_SPEC_STAT_ONE_HANDED_SWORD);
                    break;
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                    AddStat(ITEM_SPEC_STAT_TWO_HANDED_SWORD);
                    break;
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    AddStat(ITEM_SPEC_STAT_STAFF);
                    break;
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                    AddStat(ITEM_SPEC_STAT_FIST_WEAPON);
                    break;
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                    AddStat(ITEM_SPEC_STAT_DAGGER);
                    break;
                case ITEM_SUBCLASS_WEAPON_THROWN:
                    AddStat(ITEM_SPEC_STAT_THROWN);
                    break;
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    AddStat(ITEM_SPEC_STAT_CROSSBOW);
                    break;
                case ITEM_SUBCLASS_WEAPON_WAND:
                    AddStat(ITEM_SPEC_STAT_WAND);
                    break;
                default:
                    break;
            }
        }
        else if (item->Class == ITEM_CLASS_ARMOR && item->SubClass > 5 && item->SubClass <= 11)
        {
            switch (item->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_CLOTH:
                    if (sparse->InventoryType != INVTYPE_CLOAK)
                    {
                        ItemType = 1;
                        break;
                    }

                    ItemType = 0;
                    AddStat(ITEM_SPEC_STAT_CLOAK);
                    break;
                case ITEM_SUBCLASS_ARMOR_LEATHER:
                    ItemType = 2;
                    break;
                case ITEM_SUBCLASS_ARMOR_MAIL:
                    ItemType = 3;
                    break;
                case ITEM_SUBCLASS_ARMOR_PLATE:
                    ItemType = 4;
                    break;
                default:
                    ItemType = 6;
                    if (item->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
                        AddStat(ITEM_SPEC_STAT_SHIELD);
                    else if (item->SubClass > ITEM_SUBCLASS_ARMOR_SHIELD && item->SubClass <= ITEM_SUBCLASS_ARMOR_RELIC)
                        AddStat(ITEM_SPEC_STAT_RELIC);
                    break;
            }
        }
        else
            ItemType = 0;

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            if (sparse->ItemStatType[i] != -1)
                AddModStat(sparse->ItemStatType[i]);
    }

    void AddStat(ItemSpecStat statType)
    {
        if (ItemSpecStatCount >= MAX_ITEM_PROTO_STATS)
            return;

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            if (ItemSpecStatTypes[i] == uint32(statType))
                return;

        ItemSpecStatTypes[ItemSpecStatCount++] = statType;
    }

    void AddModStat(int32 itemStatType)
    {
        switch (itemStatType)
        {
            case ITEM_MOD_AGILITY:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                break;
            case ITEM_MOD_STRENGTH:
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                break;
            case ITEM_MOD_INTELLECT:
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
            case ITEM_MOD_SPIRIT:
                AddStat(ITEM_SPEC_STAT_SPIRIT);
                break;
            case ITEM_MOD_DODGE_RATING:
                AddStat(ITEM_SPEC_STAT_DODGE);
                break;
            case ITEM_MOD_PARRY_RATING:
                AddStat(ITEM_SPEC_STAT_PARRY);
                break;
            case ITEM_MOD_CRIT_MELEE_RATING:
            case ITEM_MOD_CRIT_RANGED_RATING:
            case ITEM_MOD_CRIT_SPELL_RATING:
            case ITEM_MOD_CRIT_RATING:
                AddStat(ITEM_SPEC_STAT_CRIT);
                break;
            case ITEM_MOD_HASTE_MELEE_RATING:
            case ITEM_MOD_HASTE_RANGED_RATING:
            case ITEM_MOD_HASTE_SPELL_RATING:
            case ITEM_MOD_HASTE_RATING:
                AddStat(ITEM_SPEC_STAT_HASTE);
                break;
            case ITEM_MOD_HIT_RATING:
                AddStat(ITEM_SPEC_STAT_HIT);
                break;
            case ITEM_MOD_EXTRA_ARMOR:
                AddStat(ITEM_SPEC_STAT_BONUS_ARMOR);
                break;
            case ITEM_MOD_AGI_STR_INT:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
            case ITEM_MOD_AGI_STR:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                break;
            case ITEM_MOD_AGI_INT:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
            case ITEM_MOD_STR_INT:
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
        }
    }
};

class ObjectMgr
{
    friend class PlayerDumpReader;

    private:
        ObjectMgr();
        ~ObjectMgr();

    public:
        static ObjectMgr* Instance()
        {
            static ObjectMgr instance;
            return &instance;
        }

        typedef std::unordered_map<uint32, Item*> ItemMap;

        typedef std::unordered_map<uint32, Quest*> QuestMap;

        typedef std::unordered_map<uint32, AreaTriggerStruct> AreaTriggerContainer;

        typedef std::unordered_map<uint32, uint32> AreaTriggerScriptContainer;

        typedef std::map<AccessRequirementKey, AccessRequirement> AccessRequirementContainer;

        typedef std::unordered_map<uint32, RepRewardRate > RepRewardRateContainer;
        typedef std::unordered_map<uint32, ReputationOnKillEntry> RepOnKillContainer;
        typedef std::unordered_map<uint32, RepSpilloverTemplate> RepSpilloverTemplateContainer;

        typedef std::unordered_map<uint32, PointOfInterest> PointOfInterestContainer;

        typedef std::vector<std::string> ScriptNameContainer;

        typedef std::map<uint32, uint32> CharacterConversionMap;

        typedef std::set<uint64> DupeLogMap;
        DupeLogMap m_dupeLogMap;

        void AddCharToDupeLog(uint64 guid);
        bool IsPlayerInLogList(Player *player);
        void RemoveCharFromDupeList(uint64 guid);
        void DumpDupeConstant(Player *player);

        typedef std::list<CurrencyLoot> CurrencysLoot;
        std::list<CurrencyLoot> GetCurrencyLoot(uint32 entry, uint8 type, uint8 spawnMode);

        Player* GetPlayerByLowGUID(uint32 lowguid) const;

        GameObjectTemplate const* GetGameObjectTemplate(uint32 entry);
        GameObjectTemplateContainer const* GetGameObjectTemplates() const { return &_gameObjectTemplateStore; }
        int LoadReferenceVendor(int32 vendor, int32 item, uint8 type, std::set<uint32> *skip_vendors);

        void LoadGameObjectTemplate();
        void AddGameobjectInfo(GameObjectTemplate* goinfo);

        const std::vector<CreatureDifficultyStat>* GetDifficultyStat(uint32 entry) const;
        CreatureDifficultyStat const* GetCreatureDifficultyStat(uint32 entry, uint8 diff) const;

        CreatureTemplate const* GetCreatureTemplate(uint32 entry);
        CreatureTemplateContainer const* GetCreatureTemplates() const { return &_creatureTemplateStore; }
        CreatureModelInfo const* GetCreatureModelInfo(uint32 modelId);
        CreatureModelInfo const* GetCreatureModelRandomGender(uint32* displayID);
        static uint32 ChooseDisplayId(uint32 team, const CreatureTemplate* cinfo, const CreatureData* data = NULL);
        static void ChooseCreatureFlags(const CreatureTemplate* cinfo, uint32& npcflag, uint32& npcflag2, uint32& unit_flags, uint32& dynamicflags, const CreatureData* data = NULL);
        EquipmentInfo const* GetEquipmentInfo(uint32 entry);
        CreatureAddon const* GetCreatureAddon(uint32 lowguid);
        CreatureAddon const* GetCreatureTemplateAddon(uint32 entry);
        ItemTemplate const* GetItemTemplate(uint32 entry);
        ItemTemplateContainer const* GetItemTemplateStore() const { return &_itemTemplateStore; }

        InstanceTemplate const* GetInstanceTemplate(uint32 mapId);

        PetStats const* GetPetStats(uint32 creature_id) const;

        void GetPlayerClassLevelInfo(uint32 class_, uint8 level, uint32& baseHP, uint32& baseMana) const;

        PlayerInfo const* GetPlayerInfo(uint32 race, uint32 class_) const
        {
            if (race >= MAX_RACES)
                return NULL;
            if (class_ >= MAX_CLASSES)
                return NULL;
            PlayerInfo const* info = &_playerInfo[race][class_];
            if (info->displayId_m == 0 || info->displayId_f == 0)
                return NULL;
            return info;
        }

        void GetPlayerLevelInfo(uint32 race, uint32 class_, uint8 level, PlayerLevelInfo* info) const;

        static uint64 GetPlayerGUIDByName(std::string name);
        static bool GetPlayerNameByGUID(uint64 guid, std::string &name);
        uint32 GetPlayerTeamByGUID(uint64 guid) const;
        static uint32 GetPlayerAccountIdByGUID(uint64 guid);
        static uint32 GetPlayerAccountIdByPlayerName(const std::string& name);

        uint32 GetNearestTaxiNode(float x, float y, float z, uint32 mapid, uint32 team);
        void GetTaxiPath(uint32 source, uint32 destination, uint32 &path, uint32 &cost);
        uint32 GetTaxiMountDisplayId(uint32 id, uint32 team, bool allowed_alt_team = false);

        Quest const* GetQuestTemplate(uint32 quest_id) const
        {
            QuestMap::const_iterator itr = _questTemplates.find(quest_id);
            return itr != _questTemplates.end() ? itr->second : NULL;
        }

        QuestMap const& GetQuestTemplates() const { return _questTemplates; }

        uint32 GetQuestForAreaTrigger(uint32 Trigger_ID) const
        {
            QuestAreaTriggerContainer::const_iterator itr = _questAreaTriggerStore.find(Trigger_ID);
            if (itr != _questAreaTriggerStore.end())
                return itr->second;
            return 0;
        }

        bool IsTavernAreaTrigger(uint32 Trigger_ID) const
        {
            return _tavernAreaTriggerStore.find(Trigger_ID) != _tavernAreaTriggerStore.end();
        }

        bool IsGameObjectForQuests(uint32 entry) const
        {
            return _gameObjectForQuestStore.find(entry) != _gameObjectForQuestStore.end();
        }

        GossipText const* GetGossipText(uint32 Text_ID) const;

        WorldSafeLocsEntry const* GetDefaultGraveYard(uint32 team);
        WorldSafeLocsEntry const* GetClosestGraveYard(float x, float y, float z, uint32 MapId, uint32 team);
        bool AddGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool persist = true);
        void RemoveGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool persist = false);
        void LoadGraveyardZones();
        GraveYardData const* FindGraveYardData(uint32 id, uint32 zone);

        AreaTriggerStruct const* GetAreaTrigger(uint32 trigger) const
        {
            AreaTriggerContainer::const_iterator itr = _areaTriggerStore.find(trigger);
            if (itr != _areaTriggerStore.end())
                return &itr->second;
            return NULL;
        }

        std::vector<WorldLocation> const* GetInstanceGraveYard(uint32 mapId) const
        {
            InstanceGraveYardContainer::const_iterator itr = _instanceGraveYardStore.find(mapId);
            if (itr != _instanceGraveYardStore.end())
                return &itr->second;
            return NULL;
        }

        AccessRequirement const* GetAccessRequirement(int32 mapid, Difficulty difficulty, uint16 dungeonId = 0) const
        {
            AccessRequirementKey key(mapid, uint8(difficulty), dungeonId);
            AccessRequirementContainer::const_iterator itr = _accessRequirementStore.find(key);
            if (itr != _accessRequirementStore.end())
                return &itr->second;
            return NULL;
        }

        AreaTriggerStruct const* GetGoBackTrigger(uint32 Map) const;
        AreaTriggerStruct const* GetMapEntranceTrigger(uint32 Map) const;

        uint32 GetAreaTriggerScriptId(uint32 trigger_id);
        SpellScriptsBounds GetSpellScriptsBounds(uint32 spell_id);

        AchievementScriptsBounds GetAchievementScriptsBounds(uint32 id);

        RepRewardRate const* GetRepRewardRate(uint32 factionId) const
        {
            RepRewardRateContainer::const_iterator itr = _repRewardRateStore.find(factionId);
            if (itr != _repRewardRateStore.end())
                return &itr->second;

            return NULL;
        }

        ReputationOnKillEntry const* GetReputationOnKilEntry(uint32 id) const
        {
            RepOnKillContainer::const_iterator itr = _repOnKillStore.find(id);
            if (itr != _repOnKillStore.end())
                return &itr->second;
            return NULL;
        }

        RepSpilloverTemplate const* GetRepSpilloverTemplate(uint32 factionId) const
        {
            RepSpilloverTemplateContainer::const_iterator itr = _repSpilloverTemplateStore.find(factionId);
            if (itr != _repSpilloverTemplateStore.end())
                return &itr->second;

            return NULL;
        }

        PointOfInterest const* GetPointOfInterest(uint32 id) const
        {
            PointOfInterestContainer::const_iterator itr = _pointsOfInterestStore.find(id);
            if (itr != _pointsOfInterestStore.end())
                return &itr->second;
            return NULL;
        }

        QuestPOIVector const* GetQuestPOIVector(uint32 questId)
        {
            QuestPOIContainer::const_iterator itr = _questPOIStore.find(questId);
            if (itr != _questPOIStore.end())
                return &itr->second;
            return NULL;
        }

        ScenarioPOIVector const* GetScenarioPOIVector(uint32 criteriaTreeId)
        {
            ScenarioPOIContainer::const_iterator itr = _scenarioPOIStore.find(criteriaTreeId);
            if (itr != _scenarioPOIStore.end())
                return &itr->second;
            return NULL;
        }

        VehicleAccessoryList const* GetVehicleAccessoryList(Vehicle* veh) const;

        DungeonEncounterList const* GetDungeonEncounterList(uint32 mapId, Difficulty difficulty)
        {
            std::unordered_map<uint32, DungeonEncounterList>::const_iterator itr = _dungeonEncounterStore.find(MAKE_PAIR32(mapId, difficulty));
            if (itr != _dungeonEncounterStore.end())
                return &itr->second;
            return NULL;
        }

        void LoadCurrencysLoot();
        void LoadQuests();
        void LoadQuestRelations()
        {
            TC_LOG_INFO("server", "Loading GO Start Quest Data...");
            LoadGameobjectQuestRelations();
            TC_LOG_INFO("server", "Loading GO End Quest Data...");
            LoadGameobjectInvolvedRelations();
            TC_LOG_INFO("server", "Loading Creature Start Quest Data...");
            LoadCreatureQuestRelations();
            TC_LOG_INFO("server", "Loading Creature End Quest Data...");
            LoadAreaQuestRelations();
            TC_LOG_INFO("server", "Loading Area Quest Data...");
            LoadCreatureInvolvedRelations();
        }
        void LoadGameobjectQuestRelations();
        void LoadGameobjectInvolvedRelations();
        void LoadCreatureQuestRelations();
        void LoadCreatureInvolvedRelations();
        void LoadAreaQuestRelations();

        QuestRelations* GetGOQuestRelationMap()
        {
            return &_goQuestRelations;
        }

        QuestRelationBounds GetGOQuestRelationBounds(uint32 go_entry)
        {
            return _goQuestRelations.equal_range(go_entry);
        }

        QuestRelationBounds GetGOQuestInvolvedRelationBounds(uint32 go_entry)
        {
            return _goQuestInvolvedRelations.equal_range(go_entry);
        }

        QuestRelations* GetCreatureQuestRelationMap()
        {
            return &_creatureQuestRelations;
        }
        QuestStarter* GetCreatureQuestStarterMap()
        {
            return &_creatureQuestStarter;
        }

        QuestStarter* GetGOQuestStarterMap()
        {
            return &_goQuestStarter;
        }

        QuestRelationBounds GetCreatureQuestRelationBounds(uint32 creature_entry)
        {
            return _creatureQuestRelations.equal_range(creature_entry);
        }

        QuestRelationBounds GetCreatureQuestInvolvedRelationBounds(uint32 creature_entry)
        {
            return _creatureQuestInvolvedRelations.equal_range(creature_entry);
        }

        QuestRelationBounds GetAreaQuestRelationBounds(uint32 area)
        {
            return _areaQuestRelations.equal_range(area);
        }

        void LoadGameObjectScripts();
        void LoadQuestEndScripts();
        void LoadQuestStartScripts();
        void LoadEventScripts();
        void LoadSpellScripts();
        void LoadWaypointScripts();

        void LoadSpellScriptNames();
        void ValidateSpellScripts();

        void LoadAchievementScriptNames();

        bool LoadTrinityStrings(char const* table, int32 min_value, int32 max_value);
        bool LoadTrinityStrings() { return LoadTrinityStrings("trinity_string", MIN_TRINITY_STRING_ID, MAX_TRINITY_STRING_ID); }
        void LoadDbScriptStrings();
        void LoadCreatureClassLevelStats();
        void LoadWorldVisibleDistance();
        void LoadCreatureLocales();
        void LoadCreatureDifficultyStat();
        void LoadCreatureTemplates();
        void LoadCreatureTemplateAddons();
        void CheckCreatureTemplate(CreatureTemplate const* cInfo);
        void RestructCreatureGUID(uint32 nbLigneToRestruct);
        void RestructGameObjectGUID(uint32 nbLigneToRestruct);
        void LoadCreatures();
        void LoadCreatureAIInstance();
        void LoadLinkedRespawn();
        bool SetCreatureLinkedRespawn(uint32 guid, uint32 linkedGuid);
        void LoadCreatureAddons();
        void LoadCreatureModelInfo();
        void LoadEquipmentTemplates();
        void LoadGameObjectLocales();
        void LoadGameobjects();
        void LoadItemTemplates();
        void LoadItemTemplateAddon();
        void LoadItemScriptNames();
        void LoadItemLocales();
        void LoadQuestLocales();
        void LoadNpcTextLocales();
        void LoadPageTextLocales();
        void LoadGossipMenuItemsLocales();
        void LoadPointOfInterestLocales();
        void LoadInstanceTemplate();
        void LoadInstanceEncounters();
        void LoadMailLevelRewards();
        void LoadVehicleTemplateAccessories();
        void LoadVehicleAccessories();
        void LoadPersonalLootTemplate();

        void LoadGossipText();

        void LoadAreaTriggerTeleports();
        void LoadAccessRequirements();
        void LoadQuestAreaTriggers();
        void LoadAreaTriggerScripts();
        void LoadTavernAreaTriggers();
        void LoadGameObjectForQuests();

        void LoadPageTexts();
        PageText const* GetPageText(uint32 pageEntry);

        void LoadPlayerInfo();
        void LoadPetStats();
        void LoadExplorationBaseXP();
        void LoadPetNames();
        void LoadPetNumber();
        void LoadBattlePetGuid();
        void LoadCorpses();
        void LoadFishingBaseSkillLevel();

        void LoadReputationRewardRate();
        void LoadReputationOnKill();
        void LoadReputationSpilloverTemplate();

        void LoadPointsOfInterest();
        void LoadQuestPOI();
        void LoadScenarioPOI();

        void LoadNPCSpellClickSpells();

        void LoadGameTele();

        void LoadGossipMenu();
        void LoadGossipMenuItems();

        void LoadVendors();
        void LoadTrainerSpell();
        void AddSpellToTrainer(uint32 entry, uint32 spell, uint32 spellCost, uint32 reqSkill, uint32 reqSkillValue, uint32 reqLevel);

        void LoadPhaseDefinitions();
        void LoadSpellPhaseInfo();

        void LoadResearchSiteToZoneData();
        void LoadDigSitePositions();

        void LoadAreaTriggerActionsAndData();
        AreaTriggerInfo const* GetAreaTriggerInfo(uint32 entry);

        void LoadBannedAddons();

        // Battle Pet System
        void LoadBattlePetXPForLevel();
        void LoadBattlePetBreedsToSpecies();
        void LoadBattlePetNpcTeamMember();

        BannedAddon const* GetBannedAddon(uint32 Id)
        {
            BannedAddonDataMap::const_iterator itr = sBannedAddonDataMap.find(Id);
            if (itr != sBannedAddonDataMap.end())
                return &itr->second;
            return NULL;
        }
        BannedAddonDataMap const* GetBannedAddons() { return &sBannedAddonDataMap; }

        PhaseDefinitionStore const* GetPhaseDefinitionStore() { return &_PhaseDefinitionStore; }
        SpellPhaseStore const* GetSpellPhaseStore() { return &_SpellPhaseStore; }

        std::string GeneratePetName(uint32 entry);
        uint32 GetBaseXP(uint8 level);
        uint32 GetXPForLevel(uint8 level) const;

        int32 GetFishingBaseSkillLevel(uint32 entry) const
        {
            FishingBaseSkillContainer::const_iterator itr = _fishingBaseForAreaStore.find(entry);
            return itr != _fishingBaseForAreaStore.end() ? itr->second : 0;
        }

        void ReturnOrDeleteOldMails(bool serverUp);

        CreatureBaseStats const* GetCreatureBaseStats(uint8 level, uint8 unitClass);

        void SetHighestGuids();
        uint32 GenerateLowGuid(HighGuid guidhigh);
        GuidDiapason GenerateNewDiapasonFor(uint32 type, uint32 count = 0);
        uint32 GenerateAuctionID();
        uint64 GenerateEquipmentSetGuid();
        uint32 GenerateMailID();
        uint32 GeneratePetNumber();
        uint64 GenerateVoidStorageItemId();

        uint64 GenerateBattlePetGuid();

        typedef std::multimap<int32, uint32> ExclusiveQuestGroups;
        ExclusiveQuestGroups mExclusiveQuestGroups;

        MailLevelReward const* GetMailLevelReward(uint32 level, uint32 raceMask)
        {
            MailLevelRewardContainer::const_iterator map_itr = _mailLevelRewardStore.find(level);
            if (map_itr == _mailLevelRewardStore.end())
                return NULL;

            for (MailLevelRewardList::const_iterator set_itr = map_itr->second.begin(); set_itr != map_itr->second.end(); ++set_itr)
                if (set_itr->raceMask & raceMask)
                    return &*set_itr;

            return NULL;
        }

        CellObjectGuids const * GetCellObjectGuids(uint16 mapid, uint8 spawnMode, uint32 cell_id) const
        {
            MapObjectReadGuard guard(_mapObjectGuidsStoreLock);

            MapObjectGuids::const_iterator i = _mapObjectGuidsStore.find(MAKE_PAIR32(mapid, spawnMode));
            if (i == _mapObjectGuidsStore.end())
                return nullptr;

            CellObjectGuidsMap::const_iterator j = i->second.find(cell_id);
            if (j == i->second.end())
                return nullptr;

            return &j->second;
        }

        CreatureData const* GetCreatureData(uint32 guid) const
        {
            CreatureDataContainer::const_iterator itr = _creatureDataStore.find(guid);
            if (itr == _creatureDataStore.end()) return NULL;
            return &itr->second;
        }

        CreatureAIInstance const* GetCreatureAIInstaceData(uint32 entry) const
        {
            CreatureAIInstanceContainer::const_iterator itr = _creatureAIInstance.find(entry);
            if (itr == _creatureAIInstance.end()) return NULL;
            return &itr->second;
        }

        const std::vector<DoorData>* GetCreatureAIInstaceDoorData(int32 instanceId) const
        {
            CreatureAIInstanceDoorContainer::const_iterator itr = _creatureAIInstanceDoor.find(instanceId);
            return itr != _creatureAIInstanceDoor.end() ? &(itr->second) : NULL;
        }

        const std::vector<DoorGoData>* GetCreatureAIInstaceGoData(int32 entry) const
        {
            CreatureAIInstanceGoContainer::const_iterator itr = _creatureAIInstanceGo.find(entry);
            return itr != _creatureAIInstanceGo.end() ? &(itr->second) : NULL;
        }

        PersonalLootData const* GetPersonalLootData(uint32 id, uint32 type = 0) const
        {
            PersonalLootContainer::const_iterator itr = _PersonalLootStore[type].find(id);
            if (itr == _PersonalLootStore[type].end()) return NULL;
            return &itr->second;
        }

        PersonalLootData const* GetPersonalLootDataBySpell(uint32 spellId) const
        {
            PersonalLootContainer::const_iterator itr = _PersonalLootBySpellStore.find(spellId);
            if (itr == _PersonalLootBySpellStore.end()) return NULL;
            return &itr->second;
        }

        CreatureData& NewOrExistCreatureData(uint32 guid) { return _creatureDataStore[guid]; }
        void DeleteCreatureData(uint32 guid);
        uint64 GetLinkedRespawnGuid(uint64 guid) const
        {
            LinkedRespawnContainer::const_iterator itr = _linkedRespawnStore.find(guid);
            if (itr == _linkedRespawnStore.end()) return 0;
            return itr->second;
        }
        CreatureLocale const* GetCreatureLocale(uint32 entry) const
        {
            CreatureLocaleContainer::const_iterator itr = _creatureLocaleStore.find(entry);
            if (itr == _creatureLocaleStore.end()) return NULL;
            return &itr->second;
        }
        GameObjectLocale const* GetGameObjectLocale(uint32 entry) const
        {
            GameObjectLocaleContainer::const_iterator itr = _gameObjectLocaleStore.find(entry);
            if (itr == _gameObjectLocaleStore.end()) return NULL;
            return &itr->second;
        }
        ItemLocale const* GetItemLocale(uint32 entry) const
        {
            ItemLocaleContainer::const_iterator itr = _itemLocaleStore.find(entry);
            if (itr == _itemLocaleStore.end()) return NULL;
            return &itr->second;
        }
        QuestLocale const* GetQuestLocale(uint32 entry) const
        {
            QuestLocaleContainer::const_iterator itr = _questLocaleStore.find(entry);
            if (itr == _questLocaleStore.end()) return NULL;
            return &itr->second;
        }
        NpcTextLocale const* GetNpcTextLocale(uint32 entry) const
        {
            NpcTextLocaleContainer::const_iterator itr = _npcTextLocaleStore.find(entry);
            if (itr == _npcTextLocaleStore.end()) return NULL;
            return &itr->second;
        }
        PageTextLocale const* GetPageTextLocale(uint32 entry) const
        {
            PageTextLocaleContainer::const_iterator itr = _pageTextLocaleStore.find(entry);
            if (itr == _pageTextLocaleStore.end()) return NULL;
            return &itr->second;
        }
        GossipMenuItemsLocale const* GetGossipMenuItemsLocale(uint32 entry) const
        {
            GossipMenuItemsLocaleContainer::const_iterator itr = _gossipMenuItemsLocaleStore.find(entry);
            if (itr == _gossipMenuItemsLocaleStore.end()) return NULL;
            return &itr->second;
        }
        PointOfInterestLocale const* GetPointOfInterestLocale(uint32 poi_id) const
        {
            PointOfInterestLocaleContainer::const_iterator itr = _pointOfInterestLocaleStore.find(poi_id);
            if (itr == _pointOfInterestLocaleStore.end()) return NULL;
            return &itr->second;
        }

        GameObjectData const* GetGOData(uint32 guid) const
        {
            GameObjectDataContainer::const_iterator itr = _gameObjectDataStore.find(guid);
            if (itr == _gameObjectDataStore.end()) return NULL;
            return &itr->second;
        }
        GameObjectData& NewGOData(uint32 guid) { return _gameObjectDataStore[guid]; }
        void DeleteGOData(uint32 guid);

        TrinityStringLocale const* GetTrinityStringLocale(int32 entry) const
        {
            TrinityStringLocaleContainer::const_iterator itr = _trinityStringLocaleStore.find(entry);
            if (itr == _trinityStringLocaleStore.end()) return NULL;
            return &itr->second;
        }
        const char *GetTrinityString(int32 entry, LocaleConstant locale_idx) const;
        const char *GetTrinityStringForDBCLocale(int32 entry) const { return GetTrinityString(entry, DBCLocaleIndex); }
        LocaleConstant GetDBCLocaleIndex() const { return DBCLocaleIndex; }
        void SetDBCLocaleIndex(LocaleConstant locale) { DBCLocaleIndex = locale; }

        void AddCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid, uint32 instance);
        void DeleteCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid);

        // grid objects
        void AddCreatureToGrid(uint32 guid, CreatureData const* data);
        void RemoveCreatureFromGrid(uint32 guid, CreatureData const* data);
        void AddGameobjectToGrid(uint32 guid, GameObjectData const* data);
        void RemoveGameobjectFromGrid(uint32 guid, GameObjectData const* data);
        uint32 AddGOData(uint32 entry, uint32 map, float x, float y, float z, float o, uint32 spawntimedelay = 0, float rotation0 = 0, float rotation1 = 0, float rotation2 = 0, float rotation3 = 0);
        uint32 AddCreData(uint32 entry, uint32 team, uint32 map, float x, float y, float z, float o, uint32 spawntimedelay = 0);
        bool MoveCreData(uint32 guid, uint32 map, Position pos);

        // reserved names
        void LoadReservedPlayersNames();
        bool IsReservedName(const std::string& name) const;

        // name with valid structure and symbols
        static uint8 CheckPlayerName(const std::string& name, bool create = false);
        static PetNameInvalidReason CheckPetName(const std::string& name);
        static bool IsValidCharterName(const std::string& name);

        static bool CheckDeclinedNames(std::wstring w_ownname, DeclinedName const& names);

        GameTele const* GetGameTele(uint32 id) const
        {
            GameTeleContainer::const_iterator itr = _gameTeleStore.find(id);
            if (itr == _gameTeleStore.end()) return NULL;
            return &itr->second;
        }
        GameTele const* GetGameTele(const std::string& name) const;
        GameTeleContainer const& GetGameTeleMap() const { return _gameTeleStore; }
        bool AddGameTele(GameTele& data);
        bool DeleteGameTele(const std::string& name);

        TrainerSpellData const* GetNpcTrainerSpells(uint32 entry) const
        {
            CacheTrainerSpellContainer::const_iterator  iter = _cacheTrainerSpellStore.find(entry);
            if (iter == _cacheTrainerSpellStore.end())
                return NULL;

            return &iter->second;
        }

        VendorItemData const* GetNpcVendorItemList(uint32 entry) const
        {
            CacheVendorItemContainer::const_iterator iter = _cacheVendorItemStore.find(entry);
            if (iter == _cacheVendorItemStore.end())
                return NULL;

            return &iter->second;
        }
        void AddVendorItem(uint32 entry, uint32 item, int32 maxcount, uint32 incrtime, uint32 extendedCost, uint8 type, bool persist = true); // for event
        bool RemoveVendorItem(uint32 entry, uint32 item, uint8 type, bool persist = true); // for event
        bool IsVendorItemValid(uint32 vendor_entry, uint32 id, int32 maxcount, uint32 ptime, uint32 ExtendedCost, uint8 type, Player* player = NULL, std::set<uint32>* skip_vendors = NULL, uint32 ORnpcflag = 0) const;

        void LoadScriptNames();
        ScriptNameContainer &GetScriptNames() { return _scriptNamesStore; }
        const char * GetScriptName(uint32 id) const { return id < _scriptNamesStore.size() ? _scriptNamesStore[id].c_str() : ""; }
        uint32 GetScriptId(const char *name);

        SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 creature_id) const
        {
            return SpellClickInfoMapBounds(_spellClickInfoStore.lower_bound(creature_id), _spellClickInfoStore.upper_bound(creature_id));
        }

        GossipMenusMapBounds GetGossipMenusMapBounds(uint32 uiMenuId) const
        {
            return GossipMenusMapBounds(_gossipMenusStore.lower_bound(uiMenuId), _gossipMenusStore.upper_bound(uiMenuId));
        }

        GossipMenusMapBoundsNonConst GetGossipMenusMapBoundsNonConst(uint32 uiMenuId)
        {
            return GossipMenusMapBoundsNonConst(_gossipMenusStore.lower_bound(uiMenuId), _gossipMenusStore.upper_bound(uiMenuId));
        }

        GossipMenuItemsMapBounds GetGossipMenuItemsMapBounds(uint32 uiMenuId) const
        {
            return GossipMenuItemsMapBounds(_gossipMenuItemsStore.lower_bound(uiMenuId), _gossipMenuItemsStore.upper_bound(uiMenuId));
        }
        GossipMenuItemsMapBoundsNonConst GetGossipMenuItemsMapBoundsNonConst(uint32 uiMenuId)
        {
            return GossipMenuItemsMapBoundsNonConst(_gossipMenuItemsStore.lower_bound(uiMenuId), _gossipMenuItemsStore.upper_bound(uiMenuId));
        }

        // for wintergrasp only
        GraveYardContainer GraveYardStore;

        static void AddLocaleString(const std::string& s, LocaleConstant locale, StringVector& data);
        static inline void GetLocaleString(const StringVector& data, int loc_idx, std::string& value)
        {
            if (data.size() > size_t(loc_idx) && !data[loc_idx].empty())
                value = data[loc_idx];
        }

        CharacterConversionMap FactionChange_Achievements;
        CharacterConversionMap FactionChange_Items;
        CharacterConversionMap FactionChange_Spells;
        CharacterConversionMap FactionChange_Reputation;
        CharacterConversionMap FactionChange_Titles;

        void LoadFactionChangeAchievements();
        void LoadFactionChangeItems();
        void LoadFactionChangeSpells();
        void LoadFactionChangeReputations();
        void LoadFactionChangeTitles();

        void LoadHotfixData();
        HotfixData const& GetHotfixData() const { return _hotfixData; }
        time_t GetHotfixDate(uint32 entry, uint32 type) const
        {
            time_t ret = 0;
            for (HotfixData::const_iterator itr = _hotfixData.begin(); itr != _hotfixData.end(); ++itr)
                if (itr->Entry == entry && itr->Type == type)
                    if (itr->Timestamp > ret)
                        ret = itr->Timestamp;

            return ret ? ret : time(NULL);
        }

        ///Temporaire pour la cr�ation des Z, a remettre en private apr�s
        GameObjectDataContainer _gameObjectDataStore;

        //Get difficulty from spawnmode
        uint8 GetDiffFromSpawn(uint8 spawnmode)
        {
            switch (spawnmode)
            {
                case NONE_DIFFICULTY:
                case REGULAR_DIFFICULTY:
                case MAN10_DIFFICULTY:
                case MAN40_DIFFICULTY:
                case NORMAL_SCENARIO_DIFFICULTY:
                    return 0;
                case CHALLENGE_MODE_DIFFICULTY:
                case HEROIC_DIFFICULTY:
                case MAN25_DIFFICULTY:
                case HEROIC_SCENARIO_DIFFICULTY:
                    return 1;
                case MAN10_HEROIC_DIFFICULTY:
                    return 2;
                case MAN25_HEROIC_DIFFICULTY:
                    return 3;
                case RAID_TOOL_DIFFICULTY:
                    return 4;
                case FLEXIBLE_DIFFICULTY:
                    return 5;
                default:
                    break;
            }

            return 0;
        }

        //Get item count from spawnmode
        uint8 GetCountFromSpawn(uint8 spawnmode, uint32 size)
        {
            switch (spawnmode)
            {
                case NONE_DIFFICULTY:
                case REGULAR_DIFFICULTY:
                case HEROIC_DIFFICULTY:
                    return 1;
                case MAN10_DIFFICULTY:
                case MAN10_HEROIC_DIFFICULTY:
                    return 2;
                case MAN25_DIFFICULTY:
                case MAN25_HEROIC_DIFFICULTY:
                case MAN40_DIFFICULTY:
                    return 4;
                case RAID_TOOL_DIFFICULTY:
                case CHALLENGE_MODE_DIFFICULTY:
                    return 3;
                default:
                    break;
            }

            return 0;
        }

        // Battle Pets
        const std::vector<uint32>* GetPossibleBreedsForSpecies(uint32 speciesID) const;
        const std::vector<uint32>* GetBattlePetTeamMembers(uint32 creatureEntry) const;

        //Get count from difficulty
        uint8 GetCountFromDifficulty(uint8 spawnmode)
        {
            switch (spawnmode)
            {
                case NONE_DIFFICULTY:
                case REGULAR_DIFFICULTY:
                case HEROIC_DIFFICULTY:
                case CHALLENGE_MODE_DIFFICULTY:
                case HEROIC_SCENARIO_DIFFICULTY:
                case NORMAL_SCENARIO_DIFFICULTY:
                    return 3;
                case MAN10_DIFFICULTY:
                case MAN10_HEROIC_DIFFICULTY:
                    return 8;
                case MAN25_DIFFICULTY:
                case MAN25_HEROIC_DIFFICULTY:
                case FLEXIBLE_DIFFICULTY:
                case RAID_TOOL_DIFFICULTY:
                    return 20;
                case MAN40_DIFFICULTY:
                    return 32;
                default:
                    break;
            }

            return 5;
        }

        //Get boundType from difficulty
        uint8 GetboundTypeFromDifficulty(uint8 difficulty)
        {
            switch (difficulty)
            {
                case MAN25_HEROIC_DIFFICULTY:
                    return 3;
                case MAN25_DIFFICULTY:
                    return 2;
                case HEROIC_DIFFICULTY:
                case MAN10_HEROIC_DIFFICULTY:
                    return 1;
                default:
                    return 0;
            }

            return 0;
        }

    private:
        // first free id for selected id type
        uint32 _auctionId;
        uint64 _equipmentSetGuid;
        uint32 _itemTextId;
        uint32 _mailId;
        uint32 _hiPetNumber;
        uint64 _voidItemId;

        // first free low guid for selected guid type
        uint32 _hiCharGuid;
        uint32 _hiCreatureGuid;
        uint32 _hiPetGuid;
        uint32 _hiVehicleGuid;
        uint32 _hiItemGuid;
        uint32 _hiGoGuid;
        uint32 _hiDoGuid;
        uint32 _hiCorpseGuid;
        uint32 _hiAreaTriggerGuid;
        uint32 _hiMoTransGuid;
        uint32 _hiLootGuid;

        uint64 _hiBattlePetGuid;

        QuestMap _questTemplates;

        typedef std::unordered_map<uint32, GossipText> GossipTextContainer;
        typedef std::unordered_map<uint32, uint32> QuestAreaTriggerContainer;
        typedef std::set<uint32> TavernAreaTriggerContainer;
        typedef std::set<uint32> GameObjectForQuestContainer;

        QuestAreaTriggerContainer _questAreaTriggerStore;
        TavernAreaTriggerContainer _tavernAreaTriggerStore;
        GameObjectForQuestContainer _gameObjectForQuestStore;
        GossipTextContainer _gossipTextStore;
        AreaTriggerContainer _areaTriggerStore;
        AreaTriggerScriptContainer _areaTriggerScriptStore;
        AccessRequirementContainer _accessRequirementStore;
        DungeonEncounterContainer _dungeonEncounterStore;
        InstanceGraveYardContainer _instanceGraveYardStore;

        RepRewardRateContainer _repRewardRateStore;
        RepOnKillContainer _repOnKillStore;
        RepSpilloverTemplateContainer _repSpilloverTemplateStore;

        GossipMenusContainer _gossipMenusStore;
        GossipMenuItemsContainer _gossipMenuItemsStore;
        PointOfInterestContainer _pointsOfInterestStore;

        QuestPOIContainer _questPOIStore;
        ScenarioPOIContainer _scenarioPOIStore;

        QuestRelations _goQuestRelations;
        QuestRelations _goQuestInvolvedRelations;
        QuestRelations _creatureQuestRelations;
        QuestRelations _creatureQuestInvolvedRelations;
        QuestRelations _areaQuestRelations;
        CurrencysLoot  _currencysLoot;

        QuestStarter _goQuestStarter;
        QuestStarter _creatureQuestStarter;
        QuestStarter _areaQuestStarter;

        //character reserved names
        typedef std::set<std::wstring> ReservedNamesContainer;
        ReservedNamesContainer _reservedNamesStore;

        GameTeleContainer _gameTeleStore;

        ScriptNameContainer _scriptNamesStore;

        SpellClickInfoContainer _spellClickInfoStore;

        SpellScriptsContainer _spellScriptsStore;

        AchievementScriptsContainer _achievementScriptsStore;

        VehicleAccessoryContainer _vehicleTemplateAccessoryStore;
        VehicleAccessoryContainer _vehicleAccessoryStore;

        LocaleConstant DBCLocaleIndex;

        PageTextContainer _pageTextStore;
        InstanceTemplateContainer _instanceTemplateStore;

        PhaseDefinitionStore _PhaseDefinitionStore;
        SpellPhaseStore _SpellPhaseStore;

        // Battle Pet System
        typedef std::vector<uint32> BattlePetXPForLevel;
        BattlePetXPForLevel _battlePetXPperLevel;

        typedef std::map<uint32, std::vector<uint32> > BattlePetPossibleBreedsToSpecies;
        BattlePetPossibleBreedsToSpecies _battlePetPossibleBreedsToSpecies;

        typedef std::map<uint32, std::vector<uint32> > BattlePetNpcTeamMembers;
        BattlePetNpcTeamMembers _battlePetNpcTeamMembers;

        uint32 _skipUpdateCount;

    private:
        void LoadScripts(ScriptsType type);
        void CheckScripts(ScriptsType type, std::set<int32>& ids);
        void LoadQuestRelationsHelper(QuestRelations& map, QuestStarter& _map, std::string const& table, bool starter, bool go);
        void PlayerCreateInfoAddItemHelper(uint32 race_, uint32 class_, uint32 itemId, int32 count);

        MailLevelRewardContainer _mailLevelRewardStore;

        CreatureBaseStatsContainer _creatureBaseStatsStore;

        typedef std::map<uint32, PetStats> PetStatsContainer;
        PetStatsContainer _petStatsStore;                            // [creature_id][level]

        void BuildPlayerLevelInfo(uint8 race, uint8 class_, uint8 level, PlayerLevelInfo* plinfo) const;

        PlayerInfo _playerInfo[MAX_RACES][MAX_CLASSES];

        typedef std::vector<uint32> PlayerXPperLevel;       // [level]
        PlayerXPperLevel _playerXPperLevel;

        typedef std::map<uint32, uint32> BaseXPContainer;          // [area level][base xp]
        BaseXPContainer _baseXPTable;

        typedef std::map<uint32, int32> FishingBaseSkillContainer; // [areaId][base skill level]
        FishingBaseSkillContainer _fishingBaseForAreaStore;

        typedef std::map<uint32, StringVector> HalfNameContainer;
        HalfNameContainer _petHalfName0;
        HalfNameContainer _petHalfName1;

        MapObjectGuids _mapObjectGuidsStore;

        typedef ting::shared_mutex MapObjectLock;
        typedef ting::shared_lock<MapObjectLock> MapObjectReadGuard;
        typedef std::lock_guard<MapObjectLock> MapObjectWriteGuard;

        mutable MapObjectLock _mapObjectGuidsStoreLock;

        CreatureDataContainer _creatureDataStore;
        CreatureTemplateContainer _creatureTemplateStore;
        CreatureDifficultyStatContainer _creatureDifficultyStatStore;
        CreatureModelContainer _creatureModelStore;
        CreatureAddonContainer _creatureAddonStore;
        CreatureAddonContainer _creatureTemplateAddonStore;
        CreatureAIInstanceContainer _creatureAIInstance;
        CreatureAIInstanceDoorContainer _creatureAIInstanceDoor;
        CreatureAIInstanceGoContainer _creatureAIInstanceGo;
        EquipmentInfoContainer _equipmentInfoStore;
        LinkedRespawnContainer _linkedRespawnStore;
        CreatureLocaleContainer _creatureLocaleStore;
        //GameObjectDataContainer _gameObjectDataStore;
        GameObjectLocaleContainer _gameObjectLocaleStore;
        GameObjectTemplateContainer _gameObjectTemplateStore;
        PersonalLootContainer _PersonalLootStore[MAX_LOOT_COOLDOWN_TYPE];
        PersonalLootContainer _PersonalLootBySpellStore;

        ItemTemplateContainer _itemTemplateStore;
        ItemLocaleContainer _itemLocaleStore;
        QuestLocaleContainer _questLocaleStore;
        NpcTextLocaleContainer _npcTextLocaleStore;
        PageTextLocaleContainer _pageTextLocaleStore;
        TrinityStringLocaleContainer _trinityStringLocaleStore;
        GossipMenuItemsLocaleContainer _gossipMenuItemsLocaleStore;
        PointOfInterestLocaleContainer _pointOfInterestLocaleStore;

        CacheVendorItemContainer _cacheVendorItemStore;
        CacheTrainerSpellContainer _cacheTrainerSpellStore;

        enum CreatureLinkedRespawnType
        {
            CREATURE_TO_CREATURE,
            CREATURE_TO_GO,         // Creature is dependant on GO
            GO_TO_GO,
            GO_TO_CREATURE,         // GO is dependant on creature
        };
        HotfixData _hotfixData;

        AreaTriggerInfoMap _areaTriggerData;
};

uint32 GetItemArmor(uint32 itemlevel, uint32 itemClass, uint32 itemSubclass, uint32 quality, uint32 inventoryType);
void FillItemDamageFields(float* minDamage, float* maxDamage, float* dps, uint32 itemLevel, uint32 itemClass, uint32 itemSubClass, uint32 quality, uint32 delay, float statScalingFactor, uint32 inventoryType, uint32 flags2);

#define sObjectMgr ObjectMgr::Instance()

// scripting access functions
bool LoadTrinityStrings(char const* table, int32 start_value = MAX_CREATURE_AI_TEXT_STRING_ID, int32 end_value = std::numeric_limits<int32>::min());

#endif
