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
#include "server_class.h"

#include "bot.h"

#include "in_buttons.h"

#include "bot_mods.h"
#include "bot_globals.h"
#include "bot_weapons.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_synergy.h"
#include "bot_navigator.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_perceptron.h"

#include "logging.h"

void CDystopiaMod::initMod()
{
    //Load weapons
    CWeapons::loadWeapons(m_szWeaponListName == nullptr ? "DYSTOPIA" : m_szWeaponListName, DYSTOPIAWeaps.data());

}

void CDystopiaMod::mapInit()
{
    logger->Log(LogLevel::DEBUG, "[Dystopia Mod] map Init.");
}

int CDystopiaMod::numClassOnTeam(int iTeam, int iClass)
{
	int num = 0;

	for (int i = 1; i <= CBotGlobals::numClients(); i++)
	{
		edict_t* pEdict = INDEXENT(i);

		if (CBotGlobals::entityIsValid(pEdict))
		{
			if (CBotGlobals::getTeam(pEdict) == iTeam)
			{
				if (CClassInterface::getDysPlayerClass(pEdict) == iClass)
					num++;
			}
		}
	}

	return num;
}

int CDystopiaMod::preferredClassOnTeam(int iTeam)
{
	int num = -1;
	int classNum = 1;

	for (int iClass = 1; iClass <= 3; iClass++)
	{
		int tempNum = CDystopiaMod::numClassOnTeam(iTeam, iClass);
		if (tempNum < num || num == -1)
		{
			num = tempNum;
			classNum = iClass;
		}
	}

	return classNum;
}

/**
 * Checks if the given entity is locked. (Door, buttons)
 *
 * @param pEntity   The entity to check
 * @return          true if the entity is locked, false if unlocked or invalid
 **/
bool CDystopiaMod::GetTriggerFilter(edict_t* pEntity)
{
	CBaseEntity* pBaseEntity = pEntity->GetUnknown()->GetBaseEntity();
	datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pBaseEntity);
	const int offset = UTIL_FindInDataMap(pDataMap, "m_iFilterName");
	if (offset == 0)
	{
		const char* szclassname = pEntity->GetClassName();
		logger->Log(LogLevel::ERROR, "Offset 0 for entity \"%s\"", szclassname);
		return false;
	}
	string_t* value = reinterpret_cast<string_t*>(reinterpret_cast<char*>(pBaseEntity) + offset);
	
	/*if (value == 1)
		return true; // Locked
	else
		return false; // Unlocked
	*/

		return false;
}

/**
 * Checks if the given entity is locked. (Door, buttons)
 *
 * @param pEntity   The entity to check
 * @return          true if the entity is locked, false if unlocked or invalid
 **/
int CDystopiaMod::GetTriggerFilterHandle(edict_t* pEntity)
{
	/*CBaseEntity* pBaseEntity = pEntity->GetUnknown()->GetBaseEntity();
	datamap_t* pDataMap = CBaseEntity_GetDataDescMap(pBaseEntity);

	const int offset = UTIL_FindInDataMap(pDataMap, "m_iFilterName");
	if (offset == 0)
	{
		const char* szclassname = pEntity->GetClassName();
		logger->Log(LogLevel::ERROR, "Offset 0 for entity \"%s\"", szclassname);
		return false;
	}
	string_t m_iFilterName = *reinterpret_cast<string_t*>(reinterpret_cast<char*>(pBaseEntity) + offset);
	logger->Log(LogLevel::ERROR, "Found filter name \"%s\"", STRING(m_iFilterName));

	const int offset2 = UTIL_FindInDataMap(pDataMap, "m_hFilter");
	if (offset2 == 0)
	{
		const char* szclassname = pEntity->GetClassName();
		logger->Log(LogLevel::ERROR, "Offset 0 for entity \"%s\"", szclassname);
		return false;
	}
	CBaseHandle value = *reinterpret_cast<CBaseHandle*>(reinterpret_cast<char*>(pBaseEntity) + offset2);
	if (value.IsValid())
	{
		//extern CBaseEntityList* g_pEntityList;

		//auto test = value.Get(); //g_pEntityList->LookupEntity(value);
		edict_t* index = INDEXENT(value.GetEntryIndex());

		//* pBaseEntity2 = value->Get();


		if (index == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Null pointer for entity \"%d\"", value.GetEntryIndex());
		}
		else
		{
			const char* szclassname = index->GetClassName();
			CBaseEntity* pBaseEntity2 = index->GetUnknown()->GetBaseEntity();
		}


	}*/
	return 0;
}