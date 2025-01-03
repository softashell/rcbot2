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

#pragma push_macro("clamp") //Fix for C++17 [APG]RoboCop[CL]
#undef clamp
#include <algorithm>
#pragma pop_macro("clamp")

//caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
#endif

constexpr int INVALID_BOT_COUNT = -1;

CBotCommandInline GameEventVersion("event_version", CMD_ACCESS_CONFIG, [](CClient* pClient, const BotCommandArgs& args)
{
	if (!args[0] || !*args[0])
		return COMMAND_ERROR;

	CBotGlobals::setEventVersion(std::atoi(args[0]));

	return COMMAND_ACCESSED;
});

CBotCommandInline MaxBotsCommand("max_bots", CMD_ACCESS_CONFIG | CMD_ACCESS_DEDICATED, [](const CClient* pClient, const BotCommandArgs& args)
{
	edict_t* pEntity = nullptr;

	if (pClient)
		pEntity = pClient->getPlayer();

	if (args[0] && *args[0])
	{
		int maxBots = std::atoi(args[0]);
		bool hasError = false;
		const int minBots = CBots::getMinBots();

		if (maxBots <= INVALID_BOT_COUNT) // skip check for disabling max bots (require <=)
		{
			maxBots = INVALID_BOT_COUNT;
		}
		else if ((minBots >= 0) && (maxBots <= minBots))
		{
			CBotGlobals::botMessage(pEntity, 0, "max_bots must be greater than min_bots (min_bots is currently: %d)", minBots);
			hasError = true;
		}
		maxBots = std::min(maxBots, CBotGlobals::maxClients());

		if (!hasError)
		{
			CBots::setMaxBots(maxBots);
			CBotGlobals::botMessage(pEntity, 0, "max_bots set to %d", maxBots);
		}
	}
	else
	{
		CBotGlobals::botMessage(pEntity, 0, "max_bots is currently %d", CBots::getMaxBots());
	}

	return COMMAND_ACCESSED;
});

CBotCommandInline MinBotsCommand("min_bots", CMD_ACCESS_CONFIG | CMD_ACCESS_DEDICATED, [](const CClient* pClient, const BotCommandArgs& args)
{
	edict_t* pEntity = nullptr;

	if (pClient)
		pEntity = pClient->getPlayer();

	if (args[0] && *args[0])
	{
		int minBots = std::atoi(args[0]);
		const int maxBots = CBots::getMaxBots();
		bool hasError = false;

		minBots = std::min(minBots, CBotGlobals::maxClients());

		if (minBots <= INVALID_BOT_COUNT) // skip check for disabling min bots (require <=)
		{
			minBots = INVALID_BOT_COUNT;
		}
		else if ((maxBots >= 0) && (minBots >= maxBots))
		{
			CBotGlobals::botMessage(pEntity, 0, "min_bots must be less than max_bots (max_bots is currently: %d)", maxBots);
			hasError = true;
		}

		if (!hasError)
		{
			CBots::setMinBots(minBots);
			CBotGlobals::botMessage(pEntity, 0, "min_bots set to %d", minBots);
		}
	}
	else
	{
		CBotGlobals::botMessage(pEntity, 0, "min_bots is currently %d", CBots::getMinBots());
	}

	return COMMAND_ACCESSED;
});

CBotSubcommands ConfigSubcommands("config", CMD_ACCESS_DEDICATED, {
&GameEventVersion,
&MaxBotsCommand,
&MinBotsCommand
});
