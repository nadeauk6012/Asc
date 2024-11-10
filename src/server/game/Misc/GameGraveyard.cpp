/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "GameGraveyard.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "MapMgr.h"
#include "ScriptMgr.h"

Graveyard* Graveyard::instance()
{
    static Graveyard instance;
    return &instance;
}

void Graveyard::LoadGraveyardFromDB()
{
    uint32 oldMSTime = getMSTime();

    _graveyardStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT ID, Map, x, y, z, Comment FROM game_graveyard");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 graveyard. Table `game_graveyard` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    int32 Count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 ID = fields[0].Get<uint32>();

        GraveyardStruct Graveyard;

        Graveyard.Map = fields[1].Get<uint32>();
        Graveyard.x = fields[2].Get<float>();
        Graveyard.y = fields[3].Get<float>();
        Graveyard.z = fields[4].Get<float>();
        Graveyard.name = fields[5].Get<std::string>();

        if (!Utf8toWStr(Graveyard.name, Graveyard.wnameLow))
        {
            LOG_ERROR("sql.sql", "Wrong UTF8 name for id {} in `game_graveyard` table, ignoring.", ID);
            continue;
        }

        wstrToLower(Graveyard.wnameLow);

        _graveyardStore[ID] = Graveyard;

        ++Count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Graveyard in {} ms", Count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

GraveyardStruct const* Graveyard::GetGraveyard(uint32 ID) const
{
    GraveyardContainer::const_iterator itr = _graveyardStore.find(ID);
    if (itr != _graveyardStore.end())
        return &itr->second;

    return nullptr;
}

GraveyardStruct const* Graveyard::GetDefaultGraveyard(TeamId teamId)
{
    enum DefaultGraveyard
    {
        HORDE_GRAVEYARD = 10, // Crossroads
        ALLIANCE_GRAVEYARD = 4, // Westfall
    };

    return sGraveyard->GetGraveyard(teamId == TEAM_HORDE ? HORDE_GRAVEYARD : ALLIANCE_GRAVEYARD);
}

GraveyardStruct const* Graveyard::CreateTemporaryGraveyard(Player* player, TeamId teamId)
{
    static GraveyardStruct tempGraveyard;

    // Set the temporary graveyard location to the player's current position
    tempGraveyard.Map = player->GetMapId();
    tempGraveyard.x = player->GetPositionX();
    tempGraveyard.y = player->GetPositionY();
    tempGraveyard.z = player->GetPositionZ();
   // tempGraveyard.o = player->GetOrientation();

    // Log the creation of a temporary graveyard
    LOG_DEBUG("server.loading", "Creating temporary graveyard at player's location: Map {} coords ({}, {}, {})", tempGraveyard.Map, tempGraveyard.x, tempGraveyard.y, tempGraveyard.z);

    return &tempGraveyard;
}

// Skip Archerus graveyards if the player isn't a Death Knight.
enum DeathKnightGraveyards
{
    GRAVEYARD_EBON_HOLD = 1369,
    GRAVEYARD_ARCHERUS = 1405
};

GraveyardStruct const* Graveyard::GetClosestGraveyard(Player* player, TeamId teamId, bool nearCorpse)
{
    uint32 graveyardOverride = 0;
    sScriptMgr->OnBeforeChooseGraveyard(player, teamId, nearCorpse, graveyardOverride);
    if (graveyardOverride)
    {
        return sGraveyard->GetGraveyard(graveyardOverride);
    }

    WorldLocation loc = nearCorpse ? player->GetCorpseLocation() : player->GetWorldLocation();
    uint32 mapId = loc.GetMapId();
    float x = loc.GetPositionX();
    float y = loc.GetPositionY();
    float z = loc.GetPositionZ();

    // Exception: Send player to their default graveyard if Z is less than or equal to -500
    if (z <= -500)
    {
        return GetDefaultGraveyard(teamId);
    }

    // Variables for nearest graveyard on the same map
    bool foundNear = false;
    float nearestDist = std::numeric_limits<float>::max();
    GraveyardStruct const* nearestGraveyard = nullptr;

    // Variables for nearest graveyard on entrance map for corpse map
    bool foundEntr = false;
    float distEntr = std::numeric_limits<float>::max();
    GraveyardStruct const* entryEntr = nullptr;

    // Variable for graveyard on a different map
    GraveyardStruct const* entryFar = nullptr;

    // Loop through all graveyards
    for (auto it = GraveyardStore.begin(); it != GraveyardStore.end(); ++it)
    {
        GraveyardData const& graveyardData = it->second;
        GraveyardStruct const* graveyard = sGraveyard->GetGraveyard(graveyardData.safeLocId);

        if (!graveyard || !graveyardData.IsNeutralOrFriendlyToTeam(teamId))
            continue;

        // Skip Archerus graveyards if the player isn't a Death Knight.
        if (player->getClass() != CLASS_DEATH_KNIGHT &&
            (graveyardData.safeLocId == GRAVEYARD_EBON_HOLD || graveyardData.safeLocId == GRAVEYARD_ARCHERUS))
        {
            continue;
        }
        

        // Check for map compatibility (e.g., dungeons, battlegrounds, raids)
        if (mapId != graveyard->Map)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);

            if (!mapEntry || mapEntry->entrance_map < 0 || uint32(mapEntry->entrance_map) != graveyard->Map || (mapEntry->entrance_x == 0 && mapEntry->entrance_y == 0))
            {
                entryFar = graveyard;
                continue;
            }

            // Calculate distance (2D) at entrance map
            float dist2 = (graveyard->x - mapEntry->entrance_x) * (graveyard->x - mapEntry->entrance_x) + (graveyard->y - mapEntry->entrance_y) * (graveyard->y - mapEntry->entrance_y);
            if (foundEntr)
            {
                if (dist2 < distEntr)
                {
                    distEntr = dist2;
                    entryEntr = graveyard;
                }
            }
            else
            {
                foundEntr = true;
                distEntr = dist2;
                entryEntr = graveyard;
            }
        }
        else
        {
            // Logic for nearest graveyard on the same map
            float dist2 = (graveyard->x - x) * (graveyard->x - x) + (graveyard->y - y) * (graveyard->y - y) + (graveyard->z - z) * (graveyard->z - z);
            if (dist2 < nearestDist)
            {
                nearestDist = dist2;
                nearestGraveyard = graveyard;
            }
        }
    }

    // Final selection logic
    if (nearestGraveyard)
        return nearestGraveyard;
    else if (entryEntr)
        return entryEntr;
    else if (entryFar)
        return entryFar;

    // Fallback to creating a temporary graveyard if no specific graveyard found
    return CreateTemporaryGraveyard(player, teamId);
}

GraveyardData const* Graveyard::FindGraveyardData(uint32 id, uint32 zoneId)
{
    GraveyardMapBounds range = GraveyardStore.equal_range(zoneId);
    for (; range.first != range.second; ++range.first)
    {
        GraveyardData const& data = range.first->second;
        if (data.safeLocId == id)
            return &data;
    }

    return nullptr;
}

bool Graveyard::AddGraveyardLink(uint32 id, uint32 zoneId, TeamId teamId, bool persist /*= true*/)
{
    if (FindGraveyardData(id, zoneId))
        return false;

    // add link to loaded data
    GraveyardData data;
    data.safeLocId = id;
    data.teamId = teamId;

    GraveyardStore.insert(WGGraveyardContainer::value_type(zoneId, data));

    // add link to DB
    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_GRAVEYARD_ZONE);

        stmt->SetData(0, id);
        stmt->SetData(1, zoneId);
        // Xinef: DB Data compatibility...
        stmt->SetData(2, uint16(teamId == TEAM_NEUTRAL ? 0 : (teamId == TEAM_ALLIANCE ? ALLIANCE : HORDE)));

        WorldDatabase.Execute(stmt);
    }

    return true;
}

void Graveyard::RemoveGraveyardLink(uint32 id, uint32 zoneId, TeamId teamId, bool persist /*= false*/)
{
    GraveyardMapBoundsNonConst range = GraveyardStore.equal_range(zoneId);
    if (range.first == range.second)
    {
        LOG_ERROR("sql.sql", "Table `graveyard_zone` incomplete: Zone {} Team {} does not have a linked graveyard.", zoneId, teamId);
        return;
    }

    bool found = false;

    for (; range.first != range.second; ++range.first)
    {
        GraveyardData& data = range.first->second;

        // skip not matching safezone id
        if (data.safeLocId != id)
            continue;

        // skip enemy faction graveyard at same map (normal area, city, or battleground)
        // team == 0 case can be at call from .neargrave
        if (data.teamId != TEAM_NEUTRAL && teamId != TEAM_NEUTRAL && data.teamId != teamId)
            continue;

        found = true;
        break;
    }

    // no match, return
    if (!found)
        return;

    // remove from links
    GraveyardStore.erase(range.first);

    // remove link from DB
    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_GRAVEYARD_ZONE);

        stmt->SetData(0, id);
        stmt->SetData(1, zoneId);
        // Xinef: DB Data compatibility...
        stmt->SetData(2, uint16(teamId == TEAM_NEUTRAL ? 0 : (teamId == TEAM_ALLIANCE ? ALLIANCE : HORDE)));

        WorldDatabase.Execute(stmt);
    }
}

void Graveyard::LoadGraveyardZones()
{
    uint32 oldMSTime = getMSTime();

    GraveyardStore.clear(); // need for reload case

    //                                                0       1         2
    QueryResult result = WorldDatabase.Query("SELECT ID, GhostZone, Faction FROM graveyard_zone");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Graveyard-Zone Links. DB Table `graveyard_zone` Is Empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 safeLocId = fields[0].Get<uint32>();
        uint32 zoneId = fields[1].Get<uint32>();
        uint32 team = fields[2].Get<uint16>();
        TeamId teamId = team == 0 ? TEAM_NEUTRAL : (team == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE);

        GraveyardStruct const* entry = sGraveyard->GetGraveyard(safeLocId);
        if (!entry)
        {
            LOG_ERROR("sql.sql", "Table `graveyard_zone` has a record for not existing `game_graveyard` table {}, skipped.", safeLocId);
            continue;
        }

        AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
        if (!areaEntry)
        {
            LOG_ERROR("sql.sql", "Table `graveyard_zone` has a record for not existing zone id ({}), skipped.", zoneId);
            continue;
        }

        if (team != 0 && team != HORDE && team != ALLIANCE)
        {
            LOG_ERROR("sql.sql", "Table `graveyard_zone` has a record for non player faction ({}), skipped.", team);
            continue;
        }

        if (!AddGraveyardLink(safeLocId, zoneId, teamId, false))
            LOG_ERROR("sql.sql", "Table `graveyard_zone` has a duplicate record for Graveyard (ID: {}) and Zone (ID: {}), skipped.", safeLocId, zoneId);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Graveyard-Zone Links in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

GraveyardStruct const* Graveyard::GetGraveyard(const std::string& name) const
{
    // explicit name case
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return nullptr;

    // converting string that we try to find to lower case
    wstrToLower(wname);

    // Alternative first GameTele what contains wnameLow as substring in case no GameTele location found
    const GraveyardStruct* alt = nullptr;
    for (GraveyardContainer::const_iterator itr = _graveyardStore.begin(); itr != _graveyardStore.end(); ++itr)
    {
        if (itr->second.wnameLow == wname)
            return &itr->second;
        else if (!alt && itr->second.wnameLow.find(wname) != std::wstring::npos)
            alt = &itr->second;
    }

    return alt;
}
