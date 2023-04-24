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

	m_fFov = 90.0f;
}

void CBotDystopia::spawnInit()
{
    CBot::spawnInit();

	if (m_pWeapons) // reset weapons
		m_pWeapons->clearWeapons();

	m_CurrentUtil = BOT_UTIL_MAX;
	m_pNearbyTrigger = nullptr; // trigger_multiple
	m_pNearbyCrackable = nullptr; // trigger_crackable
	m_pNearbyAmmo = nullptr; // Ammo dispensers

	m_pCurrentWeapon = nullptr;
	m_flNextSprintTime = engine->Time();
	m_flSuitPower = 0.0f;
	m_flUseCrateTime = engine->Time();
	m_flPickUpTime = engine->Time();
	m_iSelectedClass = 0;
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
			selectTeam(DYS_TEAM_PUNKS);
		else
			selectTeam(DYS_TEAM_CORPS);

		int iClass = CDystopiaMod::preferredClassOnTeam(getTeam());
		if (CClassInterface::getDysPlayerClass(m_pEdict) != iClass)
		{
			CClassInterface::setDysPlayerClass(m_pEdict, iClass);
			selectClass(iClass);
		}
		
	}

	return true;
}

void CBotDystopia::selectTeam(int iTeam) const
{
	char cmd[32];

	sprintf(cmd, "jointeam %d\n", iTeam);

	helpers->ClientCommand(m_pEdict, cmd);

	m_pPlayerInfo->ChangeTeam(iTeam);
}

void CBotDystopia::selectClass(int iClass) const
{
	char cmd[32];

	sprintf(cmd, "setclass %d\n", iClass);

	helpers->ClientCommand(m_pEdict, cmd);

	int iSelectedWpn = rand() % 4 + 1;
	sprintf(cmd, "setweapon %d\n", iSelectedWpn);

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

void CBotDystopia::selectedClass(int iClass)
{
	m_iSelectedClass = iClass;
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

	const char* szclassname = pEdict->GetClassName();
	if (strncmp(szclassname, "npc_", 4) == 0)
	{
		if (strcmp(szclassname, "npc_turret_ceiling") == 0)
		{
			//char msg[256];
			//sprintf(msg, "[DYS] Found turret! E: %d A: %d T: %d INV: %d HP: %d", CClassInterface::getDysCeilingTurretEnabled(pEdict), CClassInterface::getDysCeilingTurretActive(pEdict), CClassInterface::getDysCeilingTurretTeam(pEdict), CClassInterface::getDysCeilingTurretInvincible(pEdict), CClassInterface::getDysCeilingTurretHealth(pEdict));
			//CClients::clientDebugMsg(BOT_DEBUG_THINK, msg, this);

			if (CClassInterface::getDysCeilingTurretEnabled(pEdict) && !CClassInterface::getDysCeilingTurretInvincible(pEdict) && CClassInterface::getDysCeilingTurretTeam(pEdict) != getTeam() && CClassInterface::getDysCeilingTurretActive(pEdict))
			{
				CClients::clientDebugMsg(BOT_DEBUG_THINK, "[DYS] Found active enemy turret!", this);
				return true;
			}
		}
	}

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

	float fDist = distanceFrom(pEntity);
	const Vector entityorigin = CBotGlobals::entityOrigin(pEntity);
	const char* szclassname = pEntity->GetClassName();

	// Is valid and NOT invisible
	if (bValid && bVisible && !(CClassInterface::getEffects(pEntity) & EF_NODRAW))
	{

	}
	// Is valid and invisible
	else if (bValid && bVisible && (CClassInterface::getEffects(pEntity) & EF_NODRAW))
	{
		// Save nearby trigger_multiple and trigger_crackable and check their teams
		// These are often used with outputs on func_door
		if (strncmp(szclassname, "trigger_multiple", 16) == 0)
		{
			if (!m_pNearbyTrigger.get() || fDist < distanceFrom(m_pNearbyTrigger.get()))
			{
				//CDystopiaMod::GetTriggerFilter(pEntity);
				//CDystopiaMod::GetTriggerFilterHandle(pEntity);

				CClients::clientDebugMsg(BOT_DEBUG_THINK, "Found trigger_multiple", this);
				m_pNearbyTrigger = pEntity;

				// TODO: Access filter_activator_team or just guess and save value for later
			}
		}
		else if (strncmp(szclassname, "trigger_crackable", 17) == 0)
		{
			if (!m_pNearbyCrackable.get() || fDist < distanceFrom(m_pNearbyCrackable.get()))
			{
				//CDystopiaMod::GetTriggerFilter(pEntity);
				//CDystopiaMod::GetTriggerFilterHandle(pEntity);

				CClients::clientDebugMsg(BOT_DEBUG_THINK, "Found trigger_crackable", this);
				m_pNearbyCrackable = pEntity;

				// TODO: Access filter_activator_team or just guess and save value for later
			}
		}
	}
	else
	{
		if(pEntity == m_pNearbyAmmo.get_old())
			m_pNearbyAmmo = nullptr;
		else if (pEntity == m_pNearbyTrigger.get_old())
			m_pNearbyTrigger = nullptr;
		else if (pEntity == m_pNearbyCrackable.get_old())
			m_pNearbyCrackable = nullptr;
	}

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
	ADD_UTILITY(BOT_UTIL_FIND_NEAREST_AMMO, hasSomeConditions(CONDITION_NEED_AMMO), 0.15f); // Search for ammo*/
	ADD_UTILITY(BOT_UTIL_ATTACK_POINT, true, 0.01f); // Go to waypoints with 'goal' flag
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
		static float fDistance;

		fDistance = distanceFrom(pEnemy);

		clearFailedWeaponSelect();

		if (pWeapon->isMelee())
		{
			setMoveTo(CBotGlobals::entityOrigin(pEnemy));
			setLookAtTask(LOOK_ENEMY);
			m_fAvoidTime = engine->Time() + 1.0f;
		}

		if (pWeapon->canUseSecondary() && pWeapon->getAmmo(this, 2) && pWeapon->secondaryInRange(fDistance))
		{
			if (randomInt(0, 1))
			{
				secondaryAttack();
				return true;
			}
		}

		// can use primary
		if (pWeapon->canAttack() && pWeapon->primaryInRange(fDistance))
		{
			if (pWeapon->mustHoldAttack())
				primaryAttack(true);
			else
				primaryAttack();

			return true;
		}
	}
	else
	{
		primaryAttack();
		return true;
	}

	return false;
}

void CBotDystopia::modAim(edict_t* pEntity, Vector& v_origin, Vector* v_desired_offset, Vector& v_size, float fDist, float fDist2D)
{
	static CBotWeapon* pWp;

	CBot::modAim(pEntity, v_origin, v_desired_offset, v_size, fDist, fDist2D);

	pWp = getCurrentWeapon();

	/*switch (pWp->getID())
	{
	}

	if (pEntity == INDEXENT(m_hNearestBreakable.GetEntryIndex()))
	{
		v_desired_offset->x = 0;
		v_desired_offset->y = 0;
		v_desired_offset->z = 0;
	}*/
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

void CBotDystopia::touchedWpt(CWaypoint* pWaypoint, int iNextWaypoint, int iPrevWaypoint)
{
	if (iNextWaypoint != -1 && pWaypoint->hasFlag(CWaypointTypes::W_FL_DOOR)) // Use waypoint: Check for door
	{
		CWaypoint* pNext = CWaypoints::getWaypoint(iNextWaypoint);
		if (pNext)
		{
			/**
			 * Traces a line between the current waypoint and the next waypoint. If a door is blocking the path, try to open it.
			 **/
			CTraceFilterHitAll filter;
			const trace_t* tr = CBotGlobals::getTraceResult();
			CBotGlobals::traceLine(pWaypoint->getOrigin() + Vector(0, 0, CWaypoint::WAYPOINT_HEIGHT / 2),
				pNext->getOrigin() + Vector(0, 0, CWaypoint::WAYPOINT_HEIGHT / 2), MASK_PLAYERSOLID,
				&filter);
			if (tr->fraction < 1.0f)
			{
				if (tr->m_pEnt)
				{
					edict_t* pDoor = servergameents->BaseEntityToEdict(tr->m_pEnt);
					const char* szclassname = pDoor->GetClassName();
					if (strncmp(szclassname, "prop_door_rotating", 18) == 0 || strncmp(szclassname, "func_door", 9) == 0 || strncmp(szclassname, "func_door_rotating", 18) == 0)
					{
						int team = CClassInterface::getTeam(pDoor);

						char msg[256];
						sprintf(msg, "[DYS] Found door! DOOR TEAM: %d PLAYER TEAM: %d CLASSNAME: %s", team, getTeam(), szclassname);
						CClients::clientDebugMsg(BOT_DEBUG_THINK, msg, this);


						// TODO: Get nearby trigger_multiple and target filter team
						
						//m_pSchedules->addFront(new CSynOpenDoorSched(pDoor));
					}
					else
					{

					}
				}
			}
			
		}
	}

	CBot::touchedWpt(pWaypoint, iNextWaypoint, iPrevWaypoint);
}

bool CBotDystopia::canGotoWaypoint(Vector vPrevWaypoint, CWaypoint* pWaypoint, CWaypoint* pPrev)
{
	if (pWaypoint->hasFlag(CWaypointTypes::W_FL_DOOR))
	{
		// TODO: Check for doors Open? Team? Can crack?
		// func_door -> trigger_multiple -> filter_activator_team

		return false;
	}

	return CBot::canGotoWaypoint(vPrevWaypoint, pWaypoint, pPrev);
}
