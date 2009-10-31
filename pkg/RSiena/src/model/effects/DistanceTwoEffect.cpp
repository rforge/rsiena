/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: DistanceTwoEffect.cpp
 *
 * Description: This file contains the implementation of the class
 * DistanceTwoEffect.
 *****************************************************************************/

#include <stdexcept>

#include "DistanceTwoEffect.h"
#include "data/Network.h"
#include "data/IncidentTieIterator.h"
#include "data/OneModeNetworkLongitudinalData.h"
#include "model/variables/NetworkVariable.h"
#include "model/tables/ConfigurationTable.h"

namespace siena
{

/**
 * Constructor.
 */
DistanceTwoEffect::DistanceTwoEffect(const EffectInfo * pEffectInfo,
	int requiredTwoPathCount) : NetworkEffect(pEffectInfo)
{
	this->lrequiredTwoPathCount = requiredTwoPathCount;
}


/**
 * Calculates the contribution of a tie flip to the given actor.
 */
double DistanceTwoEffect::calculateTieFlipContribution(int alter) const
{
	int change = 0;

	// If there are enough two-paths from the ego i to the alter j, then
	// we loose the distance 2 pair (i,j) by introducing the tie between
	// them.

	if (this->pVariable()->pTwoPathTable()->get(alter) >=
		this->lrequiredTwoPathCount)
	{
		change--;
	}

	// This variable is to simplify the later tests if a two-path through
	// the given alter makes a difference.

	int criticalTwoPathCount = this->lrequiredTwoPathCount;

	if (!this->pVariable()->outTieExists(alter))
	{
		criticalTwoPathCount--;
	}

	// Consider each outgoing tie of the alter j.

	for (IncidentTieIterator iter =
			this->pVariable()->pNetwork()->outTies(alter);
		iter.valid();
		iter.next())
	{
		int h = iter.actor();

		// If h is not the ego i, there's no tie from i to h, and the
		// introduction or withdrawal of the tie (i,j) makes a difference
		// for the pair <i,h> to be a valid distance two pair,
		// then increment the contribution.

		if (h != this->pVariable()->ego() &&
			!this->pVariable()->outTieExists(h) &&
			this->pVariable()->pTwoPathTable()->get(h) == criticalTwoPathCount)
		{
			change++;
		}
	}

	// For dissolutions of ties the contribution works in the opposite way.

	if (this->pVariable()->outTieExists(alter))
	{
		change = -change;
	}

	return change;
}


/**
 * Returns if the given configuration table is used by this effect
 * during the calculation of tie flip contributions.
 */
bool DistanceTwoEffect::usesTable(const ConfigurationTable * pTable) const
{
	return pTable == this->pVariable()->pTwoPathTable();
}


/**
 * Returns the statistic corresponding to this effect as part of
 * the evaluation function with respect to the given network.
 */
double DistanceTwoEffect::evaluationStatistic(Network * pNetwork) const
{
	int statistic = 0;
	int n = pNetwork->n();
	const Network * pStartMissingNetwork =
		this->pData()->pMissingTieNetwork(this->period());
	const Network * pEndMissingNetwork =
		this->pData()->pMissingTieNetwork(this->period() + 1);

	// A helper array of marks

	int * mark = new int[n];

	for (int i = 0; i < n; i++)
	{
		mark[i] = 0;
	}

	// Count the number of distance two pairs <i,h> for each i.

	for (int i = 0; i < n; i++)
	{
		// Invariant: mark[h] <= baseMark for all h.
		int baseMark = n * i;

		// Count the number of two-paths from i to each h by setting
		// mark[h] = baseMark + <number of two-paths from i to h>.
		// If there are no two-paths from i to h, we leave mark[h] <= baseMark.

		for (IncidentTieIterator iterI = pNetwork->outTies(i);
			iterI.valid();
			iterI.next())
		{
			int j = iterI.actor();

			for (IncidentTieIterator iterJ = pNetwork->outTies(j);
				iterJ.valid();
				iterJ.next())
			{
				int h = iterJ.actor();

				if (mark[h] <= baseMark)
				{
					// We have encountered this actor for the first time.
					mark[h] = baseMark + 1;
				}
				else
				{
					// We've found yet another two-path from i to h.
					mark[h]++;
				}

				if (mark[h] == baseMark + this->lrequiredTwoPathCount)
				{
					// We've reached the necessary minimum of two-paths, hence
					// a new candidate for a distance-two pair is found.

					statistic++;
				}
			}
		}

		// Okay, if there's a tie (i,h) then <i,h> cannot possibly be a
		// distance two pair. Hence we iterate over outgoing ties (i,h) of i,
		// and if the actor h has enough two-paths, we unmark it and decrement
		// the statistic.

		for (IncidentTieIterator iter = pNetwork->outTies(i);
			iter.valid();
			iter.next())
		{
			int h = iter.actor();

			if (mark[h] >= baseMark + this->lrequiredTwoPathCount)
			{
				mark[h] = 0;
				statistic--;
			}
		}

		// We do a similar fix for missing ties (i,h) at either end of
		// the period.

		for (IncidentTieIterator iter = pStartMissingNetwork->outTies(i);
			iter.valid();
			iter.next())
		{
			int h = iter.actor();

			if (mark[h] >= baseMark + this->lrequiredTwoPathCount)
			{
				mark[h] = 0;
				statistic--;
			}
		}

		for (IncidentTieIterator iter = pEndMissingNetwork->outTies(i);
			iter.valid();
			iter.next())
		{
			int h = iter.actor();

			if (mark[h] >= baseMark + this->lrequiredTwoPathCount)
			{
				mark[h] = 0;
				statistic--;
			}
		}

		// Ignore the trivial pair <i,i>.

		if (mark[i] >= baseMark + this->lrequiredTwoPathCount)
		{
			statistic--;
		}
	}

	delete[] mark;

	// For symmetric networks, we don't want to count each distance 2
	// pair twice.

	const OneModeNetworkLongitudinalData * pData =
		dynamic_cast<const OneModeNetworkLongitudinalData *>(this->pData());

	if (!pData)
	{
		throw logic_error(
			"One-mode network data expected in distance 2 effect.");
	}

	if (pData->symmetric())
	{
		statistic /= 2;
	}

	return statistic;
}


/**
 * Returns the statistic corresponding to this effect as part of
 * the endowment function with respect to an initial network
 * and a network of lost ties. The current network is implicit as
 * the introduced ties are not relevant for calculating
 * endowment statistics.
 */
double DistanceTwoEffect::endowmentStatistic(Network * pInitialNetwork,
	Network * pLostTieNetwork) const
{
	throw logic_error(
		"Endowment effect not supported for distance 2 effects.");
}

}
