/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#include "engine_wrappers.h"

#include "bot.h"
#include "bot_cvars.h"
#include "ndebugoverlay.h"
#include "bot_squads.h"
#include "bot_dystopia.h"
#include "in_buttons.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_mods.h"
#include "bot_task.h"
#include "bot_schedule.h"
#include "bot_weapons.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_plugin_meta.h"
#include "bot_waypoint_visibility.h"

extern IVDebugOverlay *debugoverlay;
extern IServerGameEnts *servergameents; // for accessing the server game entities

void CBotDystopia::init(bool bVarInit)
{
	CBot::init(bVarInit); // call base first
}

void CBotDystopia::spawnInit()
{
    CBot::spawnInit();

	if (m_pWeapons) // reset weapons
		m_pWeapons->clearWeapons();

    m_CurrentUtil = BOT_UTIL_MAX;
	m_pNearbyAmmo = nullptr;
	m_pNearbyBattery = nullptr;
	m_pNearbyCrate = nullptr;
	m_pNearbyHealthKit = nullptr;
	m_pNearbyWeapon = nullptr;
	m_pNearbyMine = nullptr;
	m_pNearbyGrenade = nullptr;
	m_pNearbyItemCrate = nullptr;
	m_pCurrentWeapon = nullptr;
	m_flNextSprintTime = engine->Time();
	m_flSuitPower = 0.0f;
	m_flUseCrateTime = engine->Time();
	m_flPickUpTime = engine->Time();
}

bool CBotDystopia::startGame()
{
	const int team = m_pPlayerInfo->GetTeamIndex();

	if(team <= DYS_TEAM_SPECTATOR)
	{
		debugMsg(BOT_DEBUG_GAME_EVENT, "[MOD EVENT] Selecting team");

		// For some reason bots spawn in some void by default, suicide resets everything
		kill();

		if (CBotGlobals::numPlayersOnTeam(DYS_TEAM_PUNKS, false) <= CBotGlobals::numPlayersOnTeam(DYS_TEAM_CORPS, false))
			m_pPlayerInfo->ChangeTeam(DYS_TEAM_PUNKS);
		else
			m_pPlayerInfo->ChangeTeam(DYS_TEAM_CORPS);

		if (CClassInterface::getDysPlayerClass(m_pEdict) != DYS_CLASS_LIGHT)
		{
			CClassInterface::setDysPlayerClass(m_pEdict, DYS_CLASS_LIGHT);
			selectClass();
		}
		
	}

	return true;
}

void CBotDystopia::selectClass() const
{
	const char* cmd = "setclass 1";
	helpers->ClientCommand(m_pEdict, cmd);
}

void CBotDystopia::died(edict_t *pKiller, const char *pszWeapon)
{
	CBot::died(pKiller, pszWeapon);

	if(pKiller)
	{
		if(CBotGlobals::entityIsValid(pKiller))
		{
			m_pNavigator->belief(CBotGlobals::entityOrigin(pKiller), getEyePosition(), bot_beliefmulti.GetFloat(), distanceFrom(pKiller), BELIEF_DANGER);
		}
	}
}

/**
 * Determines if the bot needs health
 * 
 * @return          True if the bot needs health
 **/
bool CBotDystopia::needHealth()
{
	return getHealthPercent() <= 0.7;
}

/**
 * Determines if the bot needs ammo
 * 
 * @return          True if the bot needs ammo
 **/
bool CBotDystopia::needAmmo()
{
	return false;

	if(m_pCurrentWeapon == nullptr)
	{
		return false;
	}

	const CBotWeapon *weapon = m_pWeapons->getWeapon(CWeapons::getWeapon(m_pCurrentWeapon->GetClassName()));
	if(weapon)
	{
		const int iAmmo = weapon->getAmmo(this); // Current weapon reserve ammo
		
		switch (weapon->getID())
		{
		case SYN_WEAPON_PISTOL:
		{
			return iAmmo < 30;
			break;
		}
		case SYN_WEAPON_SHOTGUN:
		case SYN_WEAPON_DESERTEAGLE:
		case SYN_WEAPON_357:
		{
			return iAmmo < 6;
			break;
		}
		case SYN_WEAPON_SMG1:
		case SYN_WEAPON_MP5K:
		{
			return iAmmo < 75;
			break;
		}
		case SYN_WEAPON_AR2:
		case SYN_WEAPON_MG1:
		{
			return iAmmo < 20;
			break;
		}
		case SYN_WEAPON_CROSSBOW:
		{
			return iAmmo < 2;
			break;			
		}
		case SYN_WEAPON_RPG:
		{
			return iAmmo < 2;
			break;
		}

		default:
		{
			return false;
			break;
		}
		}
	}

	return false;
}

void CBotDystopia::modThink()
{
	if (needHealth())
		updateCondition(CONDITION_NEED_HEALTH);
	else
		removeCondition(CONDITION_NEED_HEALTH);

	if(needAmmo())
		updateCondition(CONDITION_NEED_AMMO);
	else
		removeCondition(CONDITION_NEED_AMMO);
		
	if(onLadder())
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD,0,1,0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}
}

void CBotDystopia::updateConditions()
{
	CBot::updateConditions();
}

bool CBotDystopia::isEnemy(edict_t *pEdict, bool bCheckWeapons)
{
	// TODO: Probably needs fixing in firing methods

	const int entity_index = ENTINDEX(pEdict);
	if (entity_index == 0)
		return false; // worldspawn

	if (!pEdict)
		return false;

	if (m_pEdict == pEdict) // self
		return false;

	if (!CBotGlobals::isAlivePlayer(pEdict))
		return false;

	if (CBotGlobals::getTeam(pEdict) >= DYS_TEAM_PUNKS && getTeam() >= DYS_TEAM_PUNKS)
	{
		if (CBotGlobals::getTeam(pEdict) != getTeam())
		{
			return true;
		}
		else if (rcbot_ffa.GetBool())
		{
			return true;
		}
	}
	
    return false;
}

bool CBotDystopia::setVisible ( edict_t *pEntity, bool bVisible )
{
	const bool bValid = CBot::setVisible(pEntity, bVisible);

	/* float fDist = distanceFrom(pEntity);
	const Vector entityorigin = CBotGlobals::entityOrigin(pEntity);
	const char* szclassname = pEntity->GetClassName();

	// Is valid and NOT invisible
	if (bValid && bVisible && !(CClassInterface::getEffects(pEntity) & EF_NODRAW))
	{

	}
	else
	{
		if(pEntity == m_pNearbyAmmo.get_old())
			m_pNearbyAmmo = nullptr;
		else if(pEntity == m_pNearbyCrate.get_old())
			m_pNearbyCrate = nullptr;
		else if(pEntity == m_pNearbyHealthKit.get_old())
			m_pNearbyHealthKit = nullptr;
		else if(pEntity == m_pNearbyBattery.get_old())
			m_pNearbyBattery = nullptr;
		else if(pEntity == m_pNearbyWeapon.get_old())
			m_pNearbyWeapon = nullptr;
		else if(pEntity == m_pNearbyMine.get_old())
			m_pNearbyMine = nullptr;
		else if(pEntity == m_pNearbyGrenade.get_old())
			m_pNearbyGrenade = nullptr;
		else if(pEntity == m_pNearbyItemCrate.get_old())
			m_pNearbyItemCrate = nullptr;
		else if(pEntity == m_pNearbyHealthCharger.get_old())
			m_pNearbyHealthCharger = nullptr;
		else if(pEntity == m_pNearbyArmorCharger.get_old())
			m_pNearbyArmorCharger = nullptr;
	}*/

	return bValid;
}

void CBotDystopia::getTasks (unsigned int iIgnore)
{
    static CBotUtilities utils;
    static CBotUtility* next;
    static bool bCheckCurrent;

	if (!hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty())
		return;

    removeCondition(CONDITION_CHANGED);
    bCheckCurrent = true; // important for checking current schedule

	// Utilities
	/*ADD_UTILITY(BOT_UTIL_GETHEALTHKIT, m_pNearbyHealthKit.get() != NULL, 1.0f - getHealthPercent()); // Pick up health kits
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_HEALTH, hasSomeConditions(CONDITION_NEED_HEALTH), 1.0f - getHealthPercent()); // Search for health kits
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO, hasSomeConditions(CONDITION_NEED_AMMO), 0.15f); // Search for ammo
	ADD_UTILITY(BOT_UTIL_ATTACK_POINT, true, 0.01f); // Go to waypoints with 'goal' flag*/
	ADD_UTILITY(BOT_UTIL_ROAM, true, 0.0001f); // Roam around

	utils.execute();

	while ((next = utils.nextBest()) != nullptr)
	{
		if (!m_pSchedules->isEmpty() && bCheckCurrent)
		{
			if (m_CurrentUtil != next->getId())
				m_pSchedules->freeMemory();
			else
				break;
		}

		bCheckCurrent = false;

		if (executeAction(next->getId()))
		{
			m_CurrentUtil = next->getId();
			m_flInterruptTime = engine->Time() + randomFloat(30.0f, 45.0f);

			if (m_fUtilTimes[next->getId()] < engine->Time())
				m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f, 2.0f); // saves problems with consistent failing

			if (CClients::clientsDebugging(BOT_DEBUG_UTIL))
			{
				CClients::clientDebugMsg(BOT_DEBUG_UTIL, g_szUtils[next->getId()], this);
			}
			break;
		}
	}

	utils.freeMemory();
}

bool CBotDystopia::executeAction(eBotAction iAction)
{
	switch (iAction)
	{
	case BOT_UTIL_ATTACK_POINT:
	{
		// roam
		CWaypoint* pWaypoint = nullptr;
		CWaypoint* pRoute = nullptr;
		CBotSchedule* pSched = new CBotSchedule();
		CBotTask* pFindPath;
		m_fUtilTimes[BOT_UTIL_ATTACK_POINT] = engine->Time() + randomFloat(60.0f, 180.0f);

		pSched->setID(SCHED_ATTACKPOINT);

		// Make the bot more likely to use alternate paths based on their braveness and current health
		if (getHealthPercent() + m_pProfile->m_fBraveness <= 1.0f)
			updateCondition(CONDITION_COVERT);
		else
			removeCondition(CONDITION_COVERT);

		pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_GOAL);

		if (pWaypoint)
		{
			pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), 0, 0);
			if ((m_fUseRouteTime <= engine->Time()))
			{
				if (pRoute)
				{
					const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
					pFindPath = new CFindPathTask(iRoute, LOOK_WAYPOINT);
					pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
					pSched->addTask(pFindPath);
					pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
					m_pSchedules->add(pSched);
					m_fUseRouteTime = engine->Time() + 30.0f;
				}
			}

			const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
			pFindPath = new CFindPathTask(iWaypoint, LOOK_WAYPOINT);
			pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
			pSched->addTask(pFindPath);
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);

			return true;
		}

		break;
	}
	case BOT_UTIL_ROAM:
	{
		// roam
		CWaypoint* pWaypoint = nullptr;
		CWaypoint* pRoute = nullptr;
		CBotSchedule* pSched = new CBotSchedule();
		CBotTask* pFindPath;

		pSched->setID(SCHED_GOTO_ORIGIN);

		// Make the bot more likely to use alternate paths based on their braveness and current health
		if (getHealthPercent() + m_pProfile->m_fBraveness <= 1.0f)
			updateCondition(CONDITION_COVERT);
		else
			removeCondition(CONDITION_COVERT);

		pWaypoint = CWaypoints::randomWaypointGoal(-1);

		if (pWaypoint)
		{
			pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), 0, 0);
			if ((m_fUseRouteTime <= engine->Time()))
			{
				if (pRoute)
				{
					const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
					pFindPath = new CFindPathTask(iRoute, LOOK_WAYPOINT);
					pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
					pSched->addTask(pFindPath);
					pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
					m_pSchedules->add(pSched);
					m_fUseRouteTime = engine->Time() + 30.0f;
				}
			}

			const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
			pFindPath = new CFindPathTask(iWaypoint, LOOK_WAYPOINT);
			pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
			pSched->addTask(pFindPath);
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);

			return true;
		}

		break;
	}
	}

	return false;
}

bool CBotDystopia::walkingTowardsWaypoint(CWaypoint *pWaypoint, bool *bOffsetApplied, Vector &vOffset)
{
	return CBot::walkingTowardsWaypoint(pWaypoint, bOffsetApplied, vOffset);
}

void CBotDystopia::reachedCoverSpot(int flags)
{
	removeCondition(CONDITION_RUN); // Remove when in cover
}

void CBotDystopia::handleWeapons()
{
	CBot::handleWeapons();
}

bool CBotDystopia::handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy)
{
	if (pWeapon)
	{
		clearFailedWeaponSelect();

		if (pWeapon->isMelee())
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));

		if (pWeapon->mustHoldAttack())
			primaryAttack(true);
		else
			primaryAttack();
	}
	else
		primaryAttack();

	return true;
}

/**
 * This functions is called by task interruptions check to see if the bot should change it's current task
 * 
 * @return 		TRUE if the bot should interrupt it's current task
 **/
bool CBotDystopia::wantsToChangeCourseOfAction()
{
	return false; // TO-DO
}