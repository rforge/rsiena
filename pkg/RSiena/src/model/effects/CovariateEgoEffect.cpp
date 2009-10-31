/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 * 
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 * 
 * File: CovariateEgoEffect.cpp
 * 
 * Description: This file contains the implementation of the
 * CovariateEgoEffect class.
 *****************************************************************************/

#include "CovariateEgoEffect.h"
#include "data/Network.h"
#include "model/variables/NetworkVariable.h"

namespace siena
{

/**
 * Constructor.
 */
CovariateEgoEffect::CovariateEgoEffect(const EffectInfo * pEffectInfo) :
	CovariateDependentNetworkEffect(pEffectInfo)
{
}


/**
 * Calculates the contribution of a tie flip to the given actor.
 */
double CovariateEgoEffect::calculateTieFlipContribution(int alter) const
{
	double change = this->value(this->pVariable()->ego());
	
	if (this->pVariable()->outTieExists(alter))
	{
		// The ego would loose the tie, so the contribution is negative
		change = -change;
	}
	
	return change;
}


/**
 * Returns the statistic corresponding to this effect as part of
 * the evaluation function with respect to the given network.
 */
double CovariateEgoEffect::evaluationStatistic(Network * pNetwork) const
{
	double statistic = 0;

	for (int i = 0; i < pNetwork->n(); i++)
	{
		if (!this->missing(i))
		{
			statistic +=
				pNetwork->outDegree(i) * this->value(i);
		}
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
double CovariateEgoEffect::endowmentStatistic(Network * pInitialNetwork,
	Network * pLostTieNetwork) const
{
	// This is the same as the evaluation statistic computed with respect
	// to the network of lost ties.
	
	return this->evaluationStatistic(pLostTieNetwork);
}

}
