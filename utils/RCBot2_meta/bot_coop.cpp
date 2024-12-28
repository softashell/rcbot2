#include "bot.h"
#include "bot_coop.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"

#include "vstdlib/random.h" // for random functions

#include <cstring>
#include <array>

void CBotCoop::modThink()
{
    // find enemies and health stations / objectives etc
}

bool CBotCoop::isEnemy(edict_t* pEdict, bool bCheckWeapons)
{
    if (pEdict == nullptr || ENTINDEX(pEdict) == 0)
        return false;

    // no shooting players
    if (ENTINDEX(pEdict) <= CBotGlobals::maxClients())
    {
        return false;
    }

    const char* classname = pEdict->GetClassName();

    // List of friendly NPCs
    constexpr std::array<const char*, 5> friendlyNPCs = {
        "npc_antlionguard",
        "npc_citizen",
        "npc_barney",
        "npc_kliener",
        "npc_alyx"
    };

    if (std::strncmp(classname, "npc_", 4) == 0)
    {
        for (const auto& friendly : friendlyNPCs)
        {
            if (std::strcmp(classname, friendly) == 0)
            {
                return false; // ally
            }
        }
        return true;
    }

    return false;
}

bool CBotCoop::startGame()
{
    return true;
}