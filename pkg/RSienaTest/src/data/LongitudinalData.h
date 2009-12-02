/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: LongitudinalData.h
 *
 * Description: This file contains the definition of the
 * LongitudinalData class.
 *****************************************************************************/

#ifndef LONGITUDINALDATA_H_
#define LONGITUDINALDATA_H_

#include "utils/NamedObject.h"

namespace siena
{

// ----------------------------------------------------------------------------
// Section: Forward declarations
// ----------------------------------------------------------------------------

class ActorSet;

// ----------------------------------------------------------------------------
// Section: LogitudinalData class
// ----------------------------------------------------------------------------

/**
 * This is the base class for several classes storing the observed values of
 * some dependent variable for one or more observation moments.
 */
class LongitudinalData : public NamedObject
{
public:
	LongitudinalData(std::string name,
		const ActorSet * pActorSet,
		int observationCount);
	virtual ~LongitudinalData();

	const ActorSet * pActorSet() const;
	int observationCount() const;
	int n() const;

	void upOnly(int period, bool flag);
	bool upOnly(int period) const;
	void downOnly(int period, bool flag);
	bool downOnly(int period) const;

private:
	// The domain of the dependent variable
	const ActorSet * lpActorSet;

	// The number of observations
	int lobservationCount;

	// Stores a flag per each period if only upward changes were observed
	bool * lupOnly;

	// Stores a flag per each period if only downward changes were observed
	bool * ldownOnly;
};

}

#endif /*LONGITUDINALDATA_H_*/
