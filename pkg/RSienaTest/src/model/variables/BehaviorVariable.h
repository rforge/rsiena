/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: BehaviorVariable.h
 *
 * Description: This file contains the definition of the
 * BehaviorVariable class.
 *****************************************************************************/

#ifndef BEHAVIORVARIABLE_H_
#define BEHAVIORVARIABLE_H_

#include "DependentVariable.h"

namespace siena
{

// ----------------------------------------------------------------------------
// Section: Forward declarations
// ----------------------------------------------------------------------------

class BehaviorLongitudinalData;
class SimulationActorSet;


// ----------------------------------------------------------------------------
// Section: BehaviorVariable class
// ----------------------------------------------------------------------------

/**
 * This class represents the state of a behavioral dependent variable.
 * @see DependentVariable
 */
class BehaviorVariable : public DependentVariable
{
public:
	BehaviorVariable(BehaviorLongitudinalData * pData,
		EpochSimulation * pSimulation);
	virtual ~BehaviorVariable();

	virtual int m() const;
	virtual LongitudinalData * pData() const;
	virtual void initialize(int period);
	virtual void setLeaverBack(const SimulationActorSet * pActorSet,
		int actor);

	virtual void makeChange(int actor);

	bool missingStartValue(int actor) const;
	int value(int actor) const;
	void value(int actor, int newValue);
	double centeredValue(int actor) const;
	bool structural(int actor) const;
	double similarity(int i, int j) const;
	const int * values() const;
	int range() const;
	double similarityMean() const;

	virtual double probability(MiniStep * pMiniStep);
	virtual bool validMiniStep(const MiniStep * pMiniStep) const;

private:
	double totalEvaluationContribution(int actor,
		int difference) const;
	double totalEndowmentContribution(int actor,
		int difference) const;
	void accumulateScores(int difference, bool UpPossible,
		bool downPossible) const;
	void calculateProbabilities(int actor);

	// The observed data for this behavioral variable
	BehaviorLongitudinalData * lpData;

	// The current value of the variable per each actor
	int * lvalues;

	// A two-dimensional array of change contributions to effects, where
	// rows correspond to differences and columns correspond to effects in the
	// evaluation function.

	double ** levaluationEffectContribution;

	// A two-dimensional array of change contributions to effects, where
	// rows correspond to differences and columns correspond to effects in the
	// endowment function.

	double ** lendowmentEffectContribution;

	// Selection probability per each difference:
	// lprobabilities[0] - probability for a downward change
	// lprobabilities[1] - probability of no change
	// lprobabilities[2] - probability of an upward change

	double * lprobabilities;

	// Indicates if upward change is possible in the current situation
	bool lupPossible;

	// Indicates if downward change is possible in the current situation
	bool ldownPossible;
};

}

#endif /*BEHAVIORVARIABLE_H_*/
