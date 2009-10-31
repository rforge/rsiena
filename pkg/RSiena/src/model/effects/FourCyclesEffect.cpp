/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: FourCyclesEffect.cpp
 *
 * Description: This file contains the implementation of the
 * FourCyclesEffect class.
 *****************************************************************************/

#include <stdexcept>
#include "FourCyclesEffect.h"
#include "utils/SqrtTable.h"
#include "data/Network.h"
#include "data/IncidentTieIterator.h"
#include "model/EffectInfo.h"
#include "model/variables/NetworkVariable.h"

namespace siena
{

/**
 * Constructor.
 */
FourCyclesEffect::FourCyclesEffect(const EffectInfo * pEffectInfo) :
	NetworkEffect(pEffectInfo)
{
	this->lcounters = 0;

	if (pEffectInfo->internalEffectParameter() != 1 &&
		pEffectInfo->internalEffectParameter() != 2)
	{
		throw invalid_argument(
			"FourCyclesEffect: Parameter value 1 or 2 expected");
	}

	this->lroot = pEffectInfo->internalEffectParameter() == 2;
	this->lpSqrtTable = SqrtTable::instance();
}


/**
 * Destructor.
 */
FourCyclesEffect::~FourCyclesEffect()
{
	delete[] this->lcounters;
	this->lcounters = 0;
}


/**
 * Initializes this effect for the use with the given epoch simulation.
 */
void FourCyclesEffect::initialize(EpochSimulation * pSimulation)
{
	NetworkEffect::initialize(pSimulation);

	delete[] this->lcounters;
	this->lcounters = new int[this->pVariable()->m()];
}


/**
 * Initializes this effect for calculating the corresponding statistics.
 * @param[in] pData the observed data
 * @param[in] pState the current state of the dependent variables
 * @param[in] period the period of interest
 */
void FourCyclesEffect::initialize(const Data * pData,
	State * pState,
	int period)
{
	NetworkEffect::initialize(pData, pState, period);
}


/**
 * Does the necessary preprocessing work for calculating the tie flip
 * contributions for a specific ego. This method must be invoked before
 * calling NetworkEffect::calculateTieFlipContribution(...).
 */
void FourCyclesEffect::preprocessEgo()
{
	int ego = this->pVariable()->ego();
	Network * pNetwork = this->pVariable()->pNetwork();

	// Count the number of three paths i -> h <- k -> j from i to each j
	this->countThreePaths(ego, pNetwork, this->lcounters);

	if (this->lroot)
	{
		// Count the number of 4-cycles the ego i is currently involved in.
		// This count is required for the sqrt case only.

		this->lcurrentCycleCount = 0;

		for (IncidentTieIterator iter = pNetwork->outTies(ego);
			iter.valid();
			iter.next())
		{
			int j = iter.actor();
			this->lcurrentCycleCount += this->lcounters[j];
		}

		// The above loop counted each 4-cycle twice
		this->lcurrentCycleCount /= 2;
	}
}


/**
 * For each j and the given i, this method calculates the number of three-paths
 * i -> h <- k -> j.
 */
void FourCyclesEffect::countThreePaths(int i,
	Network * pNetwork,
	int * counters) const
{
	int m = pNetwork->m();

	// Initialize

	for (int j = 0; j < m; j++)
	{
		counters[j] = 0;
	}

	// Enumerate all three-paths i -> h <- k -> j and update the counters.
	// The (average) time complexity is obviously O(d^3), where d is the
	// average degree.

	for (IncidentTieIterator iterI = pNetwork->outTies(i);
		iterI.valid();
		iterI.next())
	{
		int h = iterI.actor();

		for (IncidentTieIterator iterH = pNetwork->inTies(h);
			iterH.valid();
			iterH.next())
		{
			int k = iterH.actor();

			if (i != k)
			{
				for (IncidentTieIterator iterK = pNetwork->outTies(k);
					iterK.valid();
					iterK.next())
				{
					int j = iterK.actor();

					if (j != h)
					{
						counters[j]++;
					}
				}
			}
		}
	}
}


/**
 * Calculates the contribution of a tie flip to the given actor.
 */
double FourCyclesEffect::calculateTieFlipContribution(int alter) const
{
	double change;

	if (this->lroot)
	{
		int newCycleCount = this->lcurrentCycleCount;

		if (this->pVariable()->outTieExists(alter))
		{
			newCycleCount -= this->lcounters[alter];
		}
		else
		{
			newCycleCount += this->lcounters[alter];
		}

		change = this->lpSqrtTable->sqrt(newCycleCount) -
			this->lpSqrtTable->sqrt(this->lcurrentCycleCount);
	}
	else
	{
		change = this->lcounters[alter];

		if (this->pVariable()->outTieExists(alter))
		{
			change = -change;
		}
	}

	return change;
}


/**
 * Detailed comment in the base class.
 */
double FourCyclesEffect::statistic(Network * pNetwork,
	Network * pSummationTieNetwork) const
{
	double statistic = 0;
	int n = pNetwork->n();
	int m = pNetwork->m();
	int * counters = new int[m];

	for (int i = 0; i < n; i++)
	{
		// Count the number of three paths i -> h <- k -> j from i to each j
		this->countThreePaths(i, pNetwork, counters);

		for (IncidentTieIterator iter = pSummationTieNetwork->outTies(i);
			iter.valid();
			iter.next())
		{
			statistic += counters[iter.actor()];
		}
	}

	delete[] counters;

	// In case of the evaluation statistic, we counted each 4-cycle four times.
	// TODO: Is it okay to divide by 4 for endowment statistic as well?

	return statistic * 0.25;
}

}
