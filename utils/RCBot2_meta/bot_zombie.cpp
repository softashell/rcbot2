#include "engine_wrappers.h"

#include "bot.h"
#include "bot_zombie.h"
#include "bot_globals.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_schedule.h"

#pragma push_macro("clamp") // Fix for C++17 [APG]RoboCop[CL]
#undef clamp
#include <algorithm>
#pragma pop_macro("clamp")

bool CBotZombie :: isEnemy ( edict_t *pEdict,bool bCheckWeapons )
{
    if (pEdict == nullptr || pEdict == m_pEdict)
        return false;

    int edictIndex = ENTINDEX(pEdict);
    if (edictIndex == 0 || edictIndex > CBotGlobals::maxClients())
        return false;

    if (CBotGlobals::getTeamplayOn())
    {
        if (CBotGlobals::getTeam(pEdict) == getTeam())
            return false;
    }

    return true;
}

void CBotZombie::modThink()
{
    // Implement zombie-specific thinking logic here
}

void CBotZombie::getTasks(unsigned iIgnore)
{
    if (m_pEnemy)
    {
        m_pSchedules->add(new CBotGotoOriginSched(m_pEnemy));
    }
    else
    {
        CBot::getTasks();
    }
}