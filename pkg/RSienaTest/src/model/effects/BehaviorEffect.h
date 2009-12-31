/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: BehaviorEffect.h
 *
 * Description: This file contains the definition of the
 * BehaviorEffect class.
 *****************************************************************************/

#ifndef BEHAVIOREFFECT_H_
#define BEHAVIOREFFECT_H_

#include "Effect.h"

namespace siena
{

// ----------------------------------------------------------------------------
// Section: Forward declarations
// ----------------------------------------------------------------------------

class BehaviorLongitudinalData;


// ----------------------------------------------------------------------------
// Section: BehaviorEffect class
// ----------------------------------------------------------------------------

/**
 * The base class for all behavior effects.
 */
class BehaviorEffect : public Effect
{
public:
	BehaviorEffect(const EffectInfo * pEffectInfo);

	virtual void initialize(const Data * pData,
		State * pState,
		int period,
		Cache * pCache);

	/**
	 * Calculates the change in the statistic corresponding to this effect if
	 * the given actor would change his behavior by the given amount.
	 */
	virtual double calculateChangeContribution(int actor,
		int difference) = 0;

	virtual double evaluationStatistic(double * currentValues);
	virtual double endowmentStatistic(const int * difference,
		double * currentValues);
	virtual double egoStatistic(int ego, double * currentValues);

protected:
	int n() const;
	int value(int actor) const;
	double centeredValue(int actor) const;
	bool missing(int observation, int actor) const;
	double range() const;
	double similarity(double a, double b) const;
	double similarityMean() const;

private:
	BehaviorLongitudinalData * lpBehaviorData;
	const int * lvalues;
};

}

#endif /*BEHAVIOREFFECT_H_*/