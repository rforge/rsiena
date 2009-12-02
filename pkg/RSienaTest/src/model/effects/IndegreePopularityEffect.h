/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: IndegreePopularityEffect.h
 *
 * Description: This file contains the definition of the
 * IndegreePopularityEffect class.
 *****************************************************************************/

#ifndef INDEGREEPOPULARITYEFFECT_H_
#define INDEGREEPOPULARITYEFFECT_H_

#include "NetworkEffect.h"

namespace siena
{

// ----------------------------------------------------------------------------
// Section: Forward declarations
// ----------------------------------------------------------------------------

class SqrtTable;


// ----------------------------------------------------------------------------
// Section: Class definition
// ----------------------------------------------------------------------------

/**
 * This class defines the indegree popularity effect defined by
 * s_i(x)=sum_j x_{ij} x_{+j} or s_i(x)=sum_j x_{ij} sqrt(x_{+j}),
 * depending on whether the square root should be taken from
 * the indegrees.
 * The corresponding statistic is
 * the sum of squared indegrees (or indegrees^1.5) over all actors.
 */
class IndegreePopularityEffect : public NetworkEffect
{
public:
	IndegreePopularityEffect(const EffectInfo * pEffectInfo, bool root);

	virtual double calculateContribution(int alter) const;

protected:
	virtual double tieStatistic(int alter);

private:
	// Indicates if the square root of indegrees must be used
	bool lroot;

	// Lookup table for fast square root calculations
	SqrtTable * lsqrtTable;
};

}

#endif /*INDEGREEPOPULARITYEFFECT_H_*/
