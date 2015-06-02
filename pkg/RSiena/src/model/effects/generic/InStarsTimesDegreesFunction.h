/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: InStarsTimesDegreesFunction.h
 *
 * Description: This file contains the definition of the
 * ThreeCyclesFunction class.
 *****************************************************************************/

#ifndef INSTARSTIMESDEGREESFUNCTION_H_
#define INSTARSTIMESDEGREESFUNCTION_H_

#include "MixedNetworkAlterFunction.h"

namespace siena
{

// ----------------------------------------------------------------------------
// Section: Forward declarations
// ----------------------------------------------------------------------------

class SqrtTable;

// ----------------------------------------------------------------------------
// Section: InStarsTimesDegreesFunction class
// ----------------------------------------------------------------------------

/**
 * For from.x.ind effect (see manual).
 */
class InStarsTimesDegreesFunction : public MixedNetworkAlterFunction
{
public:
	InStarsTimesDegreesFunction(string firstNetworkName, string secondNetworkName,
									double parameter);
	virtual void initialize(const Data * pData,
		State * pState,
		int period,
		Cache * pCache);
	virtual double value(int alter);

private:
	bool lroot; // should the square root be taken?
	string lvariableName; // name of second network
	// Lookup table for fast square root calculations:
	SqrtTable * lsqrtTable;
};

}

#endif /*INSTARSTIMESDEGREESFUNCTION_H_*/