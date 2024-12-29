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

#include "bot_mtrand.h"

#include "bot.h"
#include "bot_ga.h"
#include "bot_globals.h"

#include <algorithm>
#include <random>

//caxanga334: SDK 2013 contains macros for std::min and std::max which causes errors when compiling
#if SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
#include "valve_minmax_off.h"
#endif

#include "logging.h"

const int CGA::g_iDefaultMaxPopSize = 16;
const float CGA::g_fCrossOverRate = 0.7f;
const float CGA::g_fMutateRate = 0.1f;
const float CGA::g_fMaxPerturbation = 0.3f;

////////////////////
// POPULATION
////////////////////

IIndividual* CPopulation::get(const int iIndex) const
{
	return m_theIndividuals[iIndex];
}

void CPopulation::add(IIndividual* individual)
{
	m_theIndividuals.emplace_back(individual);
}

void CPopulation::freeMemory()
{
	m_theIndividuals.clear();
}

void CPopulation::clear()
{
	m_theIndividuals.clear();
}

ga_nn_value CPopulation::totalFitness() const
{
	float fTotalFitness = 0.0f;

	for (unsigned i = 0; i < size(); i++)
	{
		fTotalFitness += m_theIndividuals[i]->getFitness();
	}

	return fTotalFitness;
}

ga_nn_value CPopulation::bestFitness() const
{
    float fBestFitness = std::numeric_limits<float>::lowest();

    for (IIndividual* const& individual : m_theIndividuals)
	{
        fBestFitness = std::max(fBestFitness, individual->getFitness());
	}

	return fBestFitness;
}

ga_nn_value CPopulation::averageFitness() const
{
	return totalFitness() / static_cast<float>(m_theIndividuals.size());
}

std::unique_ptr<IIndividual> CPopulation::pick()
{
	IIndividual* to_return = m_theIndividuals.back();
	m_theIndividuals.pop_back();

    return std::unique_ptr<IIndividual>(to_return);
}

////////////////////
// GENETIC ALGORITHM
////////////////////

CGA::CGA(ISelection* selectFunction) : m_theSelectFunction(selectFunction)
{
	init();
	m_thePopulation.clear();
	m_theNewPopulation.clear();
}

void CGA::addToPopulation(IIndividual* individual)
{
	m_thePopulation.add(individual);

	if (m_thePopulation.size() >= m_iMaxPopSize)
	{
		epoch();
		m_thePopulation.freeMemory();
	}
}

void CGA::epoch()
{
	m_theNewPopulation.freeMemory();

	while (m_theNewPopulation.size() < m_iMaxPopSize)
	{
		IIndividual* mum = m_theSelectFunction->select(&m_thePopulation);
		IIndividual* dad = m_theSelectFunction->select(&m_thePopulation);

		IIndividual* baby1 = mum->copy();
		IIndividual* baby2 = dad->copy();

		if (randomFloat(0, 1) < g_fCrossOverRate)
			baby1->crossOver(baby2);

		baby1->mutate();
		baby2->mutate();

		m_theNewPopulation.add(baby1);
		m_theNewPopulation.add(baby2);
	}

	m_iNumGenerations++;

	const float fCurAvgFitness = m_thePopulation.averageFitness();

	/*CBotGlobals::botMessage(NULL,0,"------Generation %d------",m_iNumGenerations);
	CBotGlobals::botMessage(NULL,0,"best fitness = %0.5f",m_thePopulation.bestFitness());
	CBotGlobals::botMessage(NULL,0,"avg fitness = %0.5f",fCurAvgFitness);
	CBotGlobals::botMessage(NULL,0,"prev. avg fitness = %0.5f",m_fPrevAvgFitness);
	if ( fCurAvgFitness>=m_fPrevAvgFitness )
		CBotGlobals::botMessage(NULL,0,"Getting Better :)");
	else
		CBotGlobals::botMessage(NULL,0,"Getting Worse! :(");*/

	m_fPrevAvgFitness = fCurAvgFitness;
}

void CGA::freeLocalMemory()
{
	m_thePopulation.freeMemory();
	m_theNewPopulation.freeMemory();
	m_iNumGenerations = 0;
}

void CGA::freeGlobalMemory()
{
	freeLocalMemory();
	delete m_theSelectFunction;
	m_theSelectFunction = nullptr;
}

bool CGA::canPick() const
{
	return m_theNewPopulation.size() > 0;
}

std::unique_ptr<IIndividual> CGA::pick()
{
	return m_theNewPopulation.pick();
}

///////////////
// SELECTION
///////////////

IIndividual* CRouletteSelection::select(CPopulation* population)
{
    if (population->size() == 0)
        logger->Log(LogLevel::WARN, "GA Error: Population is empty. Selection cannot proceed.");

    const ga_nn_value totalFitness = population->totalFitness();

    if (totalFitness <= 0.0f)
        logger->Log(LogLevel::WARN, "GA Error: Total fitness is zero or negative. Selection cannot proceed.");

    const ga_nn_value fFitnessSlice = randomFloat(0, totalFitness);

	ga_nn_value fFitnessSoFar = 0.0f;

	for (unsigned i = 0; i < population->size(); i++)
	{
        IIndividual* individual = population->get(static_cast<int>(i));

		fFitnessSoFar += individual->getFitness();

		if (fFitnessSoFar >= fFitnessSlice)
			return individual;
	}

	return population->get(0);
}