/******************************************************************************
 * SIENA: Simulation Investigation for Empirical Network Analysis
 *
 * Web: http://www.stats.ox.ac.uk/~snijders/siena/
 *
 * File: NetworkVariable.cpp
 *
 * Description: This file contains the implementation of the
 * NetworkVariable class.
 *****************************************************************************/
#include <R_ext/Print.h>
#include <R_ext/Arith.h>
#include <Rinternals.h>
#include <algorithm>
#include <cmath>
#include "NetworkVariable.h"
#include "utils/Utils.h"
#include "utils/Random.h"
#include "data/ActorSet.h"
#include "network/Network.h"
#include "network/OneModeNetwork.h"
#include "network/IncidentTieIterator.h"
#include "data/NetworkLongitudinalData.h"
#include "data/ConstantDyadicCovariate.h"
#include "data/OneModeNetworkLongitudinalData.h"
#include "model/EpochSimulation.h"
#include "model/SimulationActorSet.h"
#include "model/Model.h"
#include "model/EffectInfo.h"
#include "model/effects/NetworkEffect.h"
#include "model/tables/Cache.h"
#include "model/tables/NetworkCache.h"
#include "model/ml/NetworkChange.h"
#include "model/filters/PermittedChangeFilter.h"
#include "model/ml/Chain.h"

namespace siena
{
SEXP getMiniStepDF(const MiniStep& miniStep);

// ----------------------------------------------------------------------------
// Section: Construction
// ----------------------------------------------------------------------------

/**
 * Creates a new network variable for the given observed data.
 * @param pSimulation the owner of this variable
 */
NetworkVariable::NetworkVariable(NetworkLongitudinalData * pData,
	EpochSimulation * pSimulation) :
		DependentVariable(pData->name(),
			pData->pActorSet(),
			pSimulation)
{
	this->lpData = pData;
	this->lpSenders = pSimulation->pSimulationActorSet(pData->pSenders());
	this->lpReceivers = pSimulation->pSimulationActorSet(pData->pReceivers());
	this->lpNetwork = 0;
	this->lactiveStructuralTieCount = new int[this->n()];

	this->lpermitted = new bool[this->m()];

	int numberOfAlters;
	if (this->oneModeNetwork())
	{
		this->lpNetwork = new OneModeNetwork(this->n(), false);
		numberOfAlters = this->m();
	}
	else
	{
		this->lpNetwork = new Network(this->n(), this->m());
		numberOfAlters = this->m() + 1;
	}

	this->lprobabilities = new double[numberOfAlters];
	this->levaluationEffectContribution = new double * [numberOfAlters];
	this->lendowmentEffectContribution = new double * [numberOfAlters];
	this->lcreationEffectContribution = new double * [numberOfAlters];
	this->lsymmetricEvaluationEffectContribution = new double * [2];
	this->lsymmetricEndowmentEffectContribution = new double * [2];
	this->lsymmetricCreationEffectContribution = new double * [2];

	for (int i = 0; i < numberOfAlters; i++)
	{
		this->levaluationEffectContribution[i] =
			new double[pSimulation->pModel()->
				rEvaluationEffects(pData->name()).size()];
		this->lendowmentEffectContribution[i] =
			new double[pSimulation->pModel()->
				rEndowmentEffects(pData->name()).size()];
		this->lcreationEffectContribution[i] =
			new double[pSimulation->pModel()->
				rCreationEffects(pData->name()).size()];
	}
	for (int i = 0; i < 2; i++)
	{
		this->lsymmetricEvaluationEffectContribution[i] =
			new double[pSimulation->pModel()->
				rEvaluationEffects(pData->name()).size()];
		this->lsymmetricEndowmentEffectContribution[i] =
			new double[pSimulation->pModel()->
				rEndowmentEffects(pData->name()).size()];
		this->lsymmetricCreationEffectContribution[i] =
			new double[pSimulation->pModel()->
				rCreationEffects(pData->name()).size()];
	}

	this->lpNetworkCache =
		pSimulation->pCache()->pNetworkCache(this->lpNetwork);

	this->lalter = 0;
}


/**
 * Deallocates this variable object.
 */
NetworkVariable::~NetworkVariable()
{
	delete this->lpNetwork;

	delete[] this->lactiveStructuralTieCount;
	delete[] this->lpermitted;
	delete[] this->lprobabilities;


	// Delete arrays of contributions
	int numberOfAlters;
	if (this->oneModeNetwork())
	{
		numberOfAlters = this->m();
	}
	else
	{
		numberOfAlters = this->m() + 1;
	}
	for (int i = 0; i < numberOfAlters; i++)
	{
		delete[] this->levaluationEffectContribution[i];
		delete[] this->lendowmentEffectContribution[i];
		delete[] this->lcreationEffectContribution[i];
	}

	for (int i = 0; i < 2; i++)
	{
		delete[] this->lsymmetricEvaluationEffectContribution[i];
		delete[] this->lsymmetricEndowmentEffectContribution[i];
		delete[] this->lsymmetricCreationEffectContribution[i];
	}

	delete[] this->levaluationEffectContribution;
	delete[] this->lendowmentEffectContribution;
	delete[] this->lcreationEffectContribution;

	delete[] this->lsymmetricEvaluationEffectContribution;
	delete[] this->lsymmetricEndowmentEffectContribution;
	delete[] this->lsymmetricCreationEffectContribution;

	this->lsymmetricEvaluationEffectContribution = 0;
	this->lsymmetricEndowmentEffectContribution = 0;
	this->lsymmetricCreationEffectContribution = 0;
	this->levaluationEffectContribution = 0;
	this->lendowmentEffectContribution = 0;
	this->lcreationEffectContribution = 0;
	this->lpData = 0;
	this->lpNetwork = 0;
	this->lactiveStructuralTieCount = 0;
	this->lpermitted = 0;
	this->lprobabilities = 0;

	deallocateVector(this->lpermittedChangeFilters);
}


/**
 * Adds the given filter of permissible changes to the filters of this
 * variable. This variable becomes the owner of the filter, which means that
 * the filter will be deleted as soon as this variable is deleted.
 */
void NetworkVariable::addPermittedChangeFilter(PermittedChangeFilter * pFilter)
{
	this->lpermittedChangeFilters.push_back(pFilter);
}



// ----------------------------------------------------------------------------
// Section: Accessors
// ----------------------------------------------------------------------------

/**
 * Returns the set of actors acting as tie senders.
 */
const SimulationActorSet * NetworkVariable::pSenders() const
{
	return this->lpSenders;
}


/**
 * Returns the set of actors acting as tie receivers.
 */
const SimulationActorSet * NetworkVariable::pReceivers() const
{
	return this->lpReceivers;
}


/**
 * Returns the second dimension of this variable, namely, how many values
 * correspond to each actor. This number equals the number of receivers for
 * network variables.
 */
int NetworkVariable::m() const
{
	return this->lpReceivers->n();
}


/**
 * Indicates if this is a one-mode network, namely, if the senders and
 * receivers are the same set of actors.
 */
bool NetworkVariable::oneModeNetwork() const
{
	return this->pSenders() == this->pReceivers();
}


/**
 * Returns the longitudinal data object this variable is based on.
 */
LongitudinalData * NetworkVariable::pData() const
{
	return this->lpData;
}


// ----------------------------------------------------------------------------
// Section: Initialization
// ----------------------------------------------------------------------------

/**
 * Initializes this variable as of the beginning of the given period.
 */
void NetworkVariable::initialize(int period)
{
	DependentVariable::initialize(period);

	// Copy the respective observation

	if (this->oneModeNetwork())
	{
		OneModeNetwork * pNetwork =
			(OneModeNetwork *) this->lpNetwork;
		const OneModeNetwork * pObservedNetwork =
			(const OneModeNetwork *) this->lpData->pNetwork(period);

		// Use the copy assignment operator
		(*pNetwork) = (*pObservedNetwork);
	}
	else
	{
		// Use the copy assignment operator
		(*this->lpNetwork) = (*this->lpData->pNetwork(period));
	}

	// Initialize the counters with all structural ties, including those to
	// inactive actors.

	for (int i = 0; i < this->n(); i++)
	{
		this->lactiveStructuralTieCount[i] =
			this->lpData->structuralTieCount(i, period);
	}

	// Now subtract the structural ties to initially inactive actors.

	for (int i = 0; i < this->m(); i++)
	{
		if (!this->pReceivers()->active(i))
		{
			for (IncidentTieIterator iter =
					this->lpData->pStructuralTieNetwork(period)->inTies(i);
				iter.valid();
				iter.next())
			{
				this->lactiveStructuralTieCount[iter.actor()]--;
			}
		}
	}

}


/**
 * Returns the current state of the network.
 */
Network * NetworkVariable::pNetwork() const
{
	return this->lpNetwork;
}


// ----------------------------------------------------------------------------
// Section: Composition change
// ----------------------------------------------------------------------------

/**
 * Updates the current network and other variables when an actor becomes
 * active.
 */
void NetworkVariable::actOnJoiner(const SimulationActorSet * pActorSet,
	int actor)
{
	DependentVariable::actOnJoiner(pActorSet, actor);

	const Network * pStartNetwork = this->lpData->pNetwork(this->period());

	if (pActorSet == this->pSenders())
	{
		// Activate the ties to active receivers according to the
		// initial observation of the period.

		for (IncidentTieIterator iter = pStartNetwork->outTies(actor);
			iter.valid();
			iter.next())
		{
			if (this->pReceivers()->active(iter.actor()))
			{
				this->lpNetwork->setTieValue(actor,
					iter.actor(),
					iter.value());
			}
		}

		// The rates need to be recalculated.
		this->invalidateRates();
	}

	if (pActorSet == this->pReceivers())
	{
		// Activate the ties from active senders according to the
		// initial observation of the period.

		for (IncidentTieIterator iter = pStartNetwork->inTies(actor);
			iter.valid();
			iter.next())
		{
			if (this->pSenders()->active(iter.actor()))
			{
				this->lpNetwork->setTieValue(iter.actor(),
					actor,
					iter.value());
			}
		}

		// Update the numbers of structural ties to active actors, as one actor
		// becomes active now.

		for (IncidentTieIterator iter =
				this->lpData->pStructuralTieNetwork(this->period())->inTies(
					actor);
			iter.valid();
			iter.next())
		{
			this->lactiveStructuralTieCount[iter.actor()]++;
		}

		// The rates need to be recalculated.
		this->invalidateRates();
	}
}


/**
 * Updates the current network and other variables when an actor becomes
 * inactive.
 */
void NetworkVariable::actOnLeaver(const SimulationActorSet * pActorSet,
	int actor)
{
	DependentVariable::actOnLeaver(pActorSet, actor);

	if (pActorSet == this->pSenders())
	{
		// Remove the ties from the given actor.
		this->lpNetwork->clearOutTies(actor);

		// The rates need to be recalculated.
		this->invalidateRates();
	}

	if (pActorSet == this->pReceivers())
	{
		// Remove the ties to the given actor.
		this->lpNetwork->clearInTies(actor);

		// Update the numbers of structural ties to active actors, as one actor
		// becomes inactive now.

		for (IncidentTieIterator iter =
				this->lpData->pStructuralTieNetwork(this->period())->inTies(
					actor);
			iter.valid();
			iter.next())
		{
			this->lactiveStructuralTieCount[iter.actor()]--;
		}

		// The rates need to be recalculated.
		this->invalidateRates();
	}
}


/**
 * Sets leavers values back to the value at the start of the simulation.
 *
 */
void NetworkVariable::setLeaverBack(const SimulationActorSet * pActorSet,
	int actor)
{
	if (pActorSet == this->pSenders())
	{
		// Reset ties from the given actor to values at start

		for (int i = 0; i < this->m(); i++)
		{
			if (i != actor)
			{
				this->lpNetwork->setTieValue(actor,
					i,
					this->lpData->tieValue(actor, i, this->period()));
			}
		}
	}

	if (pActorSet == this->pReceivers())
	{
		for (int i = 0; i < this->n(); i++)
		{
			if (i != actor)
			{
				this->lpNetwork->setTieValue(i,
					actor,
					this->lpData->tieValue(i, actor, this->period()));
			}
		}
	}
}


// ----------------------------------------------------------------------------
// Section: Changing the network
// ----------------------------------------------------------------------------

/**
 * Returns if the given actor can change the current state of this variable,
 * namely, it is active and at least one of the tie variables to active
 * receivers is not structurally determined.
 */
bool NetworkVariable::canMakeChange(int actor) const
{
	bool rc = DependentVariable::canMakeChange(actor);

	if (rc)
	{
		int activeAlterCount =
			this->lpReceivers->activeActorCount();

		if (this->oneModeNetwork())
		{
			// No loops are possible in one mode networks
			activeAlterCount--;
		}

		rc &= this->lpSenders->active(actor) &&
			this->lactiveStructuralTieCount[actor] < activeAlterCount;
	}

	return rc;
}


/**
 * Simulates a change of the network according to the choice of the given
 * actor. First, the actor chooses the alter based on the evaluation and
 * endowment functions, and then the tie to the selected alter is flipped.
 */
void NetworkVariable::makeChange(int actor)
{
	int m;
	this->lego = actor;
	bool accept = true;
	int alter;

	this->successfulChange(true);

	if (this->pSimulation()->pModel()->modelTypeB())
	{
		if (this->calculateModelTypeBProbabilities())
		{
			accept = nextDouble() < this->lsymmetricProbability;

			if (this->pSimulation()->pModel()->needScores())
			{
				this->accumulateSymmetricModelScores(this->lalter, accept);
			}
		}
		else
		{
			this->successfulChange(false);
			return;
		}
		alter = this->lalter;
	}
	else
	{
		this->calculateTieFlipProbabilities();

		if (this->oneModeNetwork())
		{
			m = this->m();
		}
		else
		{
			m = this->m() + 1;
		}

		//	int alteri = nextIntWithProbabilities(this->lalterSet.size(),
		//	this->lprobabilities);
		//alter = this->lalterSet[alteri];
			alter = nextIntWithProbabilities(m,	this->lprobabilities);

		// Siena 3 checks in the diagonal case, so I do too temporarily.
		//if (alter != actor && !this->lpNetworkCache->outTieExists(alter) &&
		//	this->pSimulation()->pModel()->modelType() == AAGREE)
		if (this->pSimulation()->pModel()->modelType() == AAGREE &&
			!this->lpNetworkCache->outTieExists(alter))
		{
			 this->checkAlterAgreement(alter);
			 double value = nextDouble();
			 accept = value < this->lsymmetricProbability;

			 if (this->pSimulation()->pModel()->needScores())
			 {
				 this->addAlterAgreementScores(accept);
			 }
		}

		if (this->pSimulation()->pModel()->needScores())
		{
			this->accumulateScores(alter);
		}
	}
//	 NB  the probabilities are probably wrong for !accept
	if (this->pSimulation()->pModel()->needChain())
	{
		// add ministep to chain
		MiniStep * pMiniStep;
		if (accept)
		{
			pMiniStep =
				new NetworkChange(this->lpData, actor, alter,
					this->diagonalMiniStep(actor, alter));
		}
		else
		{
			pMiniStep =
				new NetworkChange(this->lpData, actor, alter, true);
		}
		this->pSimulation()->pChain()->insertBefore(pMiniStep,
			this->pSimulation()->pChain()->pLast());
		if (!this->pSimulation()->pModel()->modelTypeB())
		{
			pMiniStep->logChoiceProbability(log(this->lprobabilities[alter]));
			if (this->pSimulation()->pModel()->modelType() == AAGREE)
			{
				pMiniStep->logChoiceProbability(pMiniStep->
					logChoiceProbability() + log(this->lsymmetricProbability));
			}
			// TODO: no change contributions copied for the alter agreement.
			if (this->pSimulation()->pModel()->needChangeContributions())
			{
				this->copyChangeContributions(pMiniStep);
			}
		}
		else
		{
			// TODO: no change contributions stored for symmetric models.
			double probability = this->lsymmetricProbability;
			if (!accept)
			{
				probability = 1 - probability;
			}
			pMiniStep->logChoiceProbability(log(this->lalterProbability) +
				log(probability));
		}
	}
	// Make a change if we have a real alter (other than the ego or
	// the dummy for bipartite networks)

	if (accept && ((!this->oneModeNetwork() && alter < this->m()) ||
			(this->oneModeNetwork() && this->lego != alter)))
	{
		int currentValue = this->lpNetwork->tieValue(this->lego, alter);

		// Update the distance from the observed data at the beginning of the
		// period. Ties missing at any of the endpoints of the period
		// don't contribute to the distance

		// If network is symmetric, changes are in steps of two, otherwise 1
		int change = 1;
		if (this->oneModeNetwork())
		{
			if ((dynamic_cast<const OneModeNetworkLongitudinalData *>
					(this->lpData))->symmetric())
			{
				change = 2;
			}
		}
		if (!this->lpData->missing(this->lego, alter, this->period()) &&
			!this->lpData->missing(this->lego, alter, this->period() + 1))
		{
			if (this->lpData->tieValue(this->lego, alter, this->period()) ==
				currentValue)
			{
				this->simulatedDistance(this->simulatedDistance() + change);
			}
			else
			{
				this->simulatedDistance(this->simulatedDistance() - change);
			}
		}

		this->lpNetwork->setTieValue(this->lego, alter, 1 - currentValue);

		if (this->oneModeNetwork())
		{
			const OneModeNetworkLongitudinalData * pData =
				dynamic_cast<const OneModeNetworkLongitudinalData *>(
					this->pData());

			if (pData->symmetric())
			{
				this->lpNetwork->setTieValue(alter,
					this->lego,
					1 - currentValue);
			}
		}
	}
}


/**
 * This method does some preprocessing to speed up subsequent queries regarding
 * the specified (usually current) ego.
 */
void NetworkVariable::preprocessEgo(int ego)
{
	// Let the effects do their preprocessing.

	this->preprocessEgo(this->pEvaluationFunction(), ego);
	this->preprocessEgo(this->pEndowmentFunction(), ego);
	this->preprocessEgo(this->pCreationFunction(), ego);
}


/**
 * This method does some preprocessing for each effect of the given
 * function to speed up subsequent queries regarding
 * the specified (usually current) ego.
 */
void NetworkVariable::preprocessEgo(const Function * pFunction, int ego)
{
	for (unsigned i = 0; i < pFunction->rEffects().size(); i++)
	{
		NetworkEffect * pEffect =
			(NetworkEffect *) pFunction->rEffects()[i];
		pEffect->preprocessEgo(ego);
	}
}


/**
 * Determines the set of actors that are allowed to act as alters in the next
 * tie flip, and stores this information in <code>lpermitted</code>.
 */
void NetworkVariable::calculatePermissibleChanges()
{
	NetworkLongitudinalData * pData =
		(NetworkLongitudinalData *) this->pData();

	// Test each alter if a tie flip to that alter is permitted according
	// to upOnly/downOnly flags and the max degree.

	int m = this->m();

	for (int i = 0; i < m; i++)
	{
//	bool needEgo = true;
// 	for (map<int, double>::const_iterator iter =
// 			 this->lsetting.begin();
// 		 iter != this->lsetting.end();
// 		 iter++)
// 	{
// 		int i = iter->first;
// 		if (needEgo && i > this->lego )
// 		{
// 			this->lalterSet.push_back(this->lego);
// 			this->lpermitted[this->lego] = true;
// 			needEgo = false;
// 		}
// 		this->lalterSet.push_back(i);
		if (this->lpNetworkCache->outTieExists(i))
		{
			this->lpermitted[i] = !pData->upOnly(this->period());
		}
		else if (i != this->lego || !this->oneModeNetwork())
		{
			this->lpermitted[i] =
				// for comparability with siena3 comment this out
				//	this->pSimulation()->active(this->pReceivers(), i) &&
				!pData->downOnly(this->period()) &&
				this->lpNetwork->outDegree(this->lego) < pData->maxDegree();
		}
		else
		{
			// It is okay to not make any change at all
			this->lpermitted[i] = true;
		}
	}
// 	if (needEgo )
// 	{
// 		this->lalterSet.push_back(this->lego);
// 		this->lpermitted[this->lego] = true;
// 		needEgo = false;
// 	}

	// Prohibit the change of structural ties

	for (IncidentTieIterator iter =
			pData->pStructuralTieNetwork(this->period())->outTies(this->lego);
		iter.valid();
		iter.next())
	{
		this->lpermitted[iter.actor()] = false;
	}

	// Run the filters that may forbid some more changes

	for (unsigned i = 0; i < this->lpermittedChangeFilters.size(); i++)
	{
		PermittedChangeFilter * pFilter = this->lpermittedChangeFilters[i];
		//	Rprintf(" about to filter %d %d\n ", i, this->lpermitted);
		pFilter->filterPermittedChanges(this->lego, this->lpermitted);
		//Rprintf(" filtered %d %d\n ", i, this->lpermitted);
	}
	// for (int i = 0; i < m; i++)
	// {
	// 	Rprintf("permitted %d %d\n", i, this->lpermitted[i]);
	// }

}


/**
 * For each alter, this method calculates the contribution of each evaluation,
 * endowment, and tie creation effect if a tie from the ego to this alter was
 * flipped.
 * These contributions are stored in arrays
 * <code>levaluationEffectContribution</code>,
 * <code>lendowmentEffectContribution</code>, and
 * <code>lcreationEffectContribution</code>.
 */
void NetworkVariable::calculateTieFlipContributions()
{
	int evaluationEffectCount = this->pEvaluationFunction()->rEffects().size();
	int endowmentEffectCount = this->pEndowmentFunction()->rEffects().size();
	int creationEffectCount = this->pCreationFunction()->rEffects().size();
	const vector<Effect *> & rEvaluationEffects =
		this->pEvaluationFunction()->rEffects();
	const vector<Effect *> & rEndowmentEffects =
		this->pEndowmentFunction()->rEffects();
	const vector<Effect *> & rCreationEffects =
		this->pCreationFunction()->rEffects();
	bool twoModeNetwork = !this->oneModeNetwork();
	int m = this->m();

	for (int alter = 0; alter < m; alter++)
//	for (unsigned alteri = 0; alteri < this->lalterSet.size(); alteri++)
	{
		// alter = ego for one-mode networks means no change.
		// No change, no contribution.
		//if (this->lalterSet.size() < m)
		//{
		//	int alter = this->lalterSet[alteri];

		if (this->lpermitted[alter] &&
			(twoModeNetwork || alter != this->lego))
		{
			for (int i = 0; i < evaluationEffectCount; i++)
			{
				NetworkEffect * pEffect =
					(NetworkEffect *) rEvaluationEffects[i];
				double contribution =
					pEffect->calculateContribution(alter);

				// Tie withdrawals contribute in the opposite way

				if (this->lpNetworkCache->outTieExists(alter))
				{
					contribution = -contribution;
				}

				this->levaluationEffectContribution[alter][i] = contribution;
			}
		}
		else
		{
			for (int i = 0; i < evaluationEffectCount; i++)
			{
				if (!this->lpermitted[alter])
				{
					this->levaluationEffectContribution[alter][i] = R_NaN;
				}
				else
				{
					this->levaluationEffectContribution[alter][i] = 0;
				}
			}
		}

		// The endowment effects have non-zero contributions on tie
		// withdrawals only

		if (this->lpNetworkCache->outTieExists(alter) &&
			this->lpermitted[alter])
		{
			for (int i = 0; i < endowmentEffectCount; i++)
			{
				NetworkEffect * pEffect =
					(NetworkEffect *) rEndowmentEffects[i];
				this->lendowmentEffectContribution[alter][i] =
					-pEffect->calculateContribution(alter);
			}
		}
		else
		{
			for (int i = 0; i < endowmentEffectCount; i++)
			{
				if (!this->lpermitted[alter])
				{
					this->lendowmentEffectContribution[alter][i] = R_NaN;
				}
				else
				{
					this->lendowmentEffectContribution[alter][i] = 0;
				}
			}
		}

		// The tie creation effects have non-zero contributions on tie
		// creation only

		if (!this->lpNetworkCache->outTieExists(alter) &&
			this->lpermitted[alter] &&
			(twoModeNetwork || alter != this->lego))
		{
			for (int i = 0; i < creationEffectCount; i++)
			{
				NetworkEffect * pEffect =
					(NetworkEffect *) rCreationEffects[i];
				double contribution =
					pEffect->calculateContribution(alter);

				this->lcreationEffectContribution[alter][i] = contribution;
			}
		}
		else
		{
			for (int i = 0; i < creationEffectCount; i++)
			{
				if (!this->lpermitted[alter])
				{
					this->lcreationEffectContribution[alter][i] = R_NaN;
				}
				else
				{
					this->lcreationEffectContribution[alter][i] = 0;
				}
			}
		}
	}

	// need to initialise the no-effect option of alter = this->m() for
	// twomode networks

	if (twoModeNetwork)
	{
		for (int i = 0; i < evaluationEffectCount; i++)
		{
			this->levaluationEffectContribution[m][i] = 0;
		}

		for (int i = 0; i < endowmentEffectCount; i++)
		{
			this->lendowmentEffectContribution[m][i] = 0;
		}

		for (int i = 0; i < creationEffectCount; i++)
		{
			this->lcreationEffectContribution[m][i] = 0;
		}
	}
}


/**
 * Calculates the probability of each actor of being chosen for the next
 * tie flip.
 */
void NetworkVariable::calculateTieFlipProbabilities()
{
//	ConstantDyadicCovariate * pCov =
//		this->pSimulation()->pData()->rConstantDyadicCovariates()[0];
//	this->liter = pCov->pRowValues(this->lego).begin();
//	this->lsetting = pCov->rRowValues(this->lego);
//	this->lalterSet.clear();
	this->preprocessEgo(this->lego);
	this->calculatePermissibleChanges();
//	int m = this->m();
//	int fastAlter = this->lego;
//	while(fastAlter == this->lego && this->lpermitted[fastAlter])
//	{
//		fastAlter = nextInt(m);
//	}

//	this->lalterSet.push_back(min(this->lego, fastAlter));
//	this->lalterSet.push_back(max(this->lego, fastAlter));

	this->calculateTieFlipContributions();

	int evaluationEffectCount = this->pEvaluationFunction()->rEffects().size();
	int endowmentEffectCount = this->pEndowmentFunction()->rEffects().size();
	int creationEffectCount = this->pCreationFunction()->rEffects().size();

	double total = 0;
	int m = this->m();
//	Rprintf("%d\n", this->lego);

	for (int alter = 0; alter < m; alter++)
	{
//	for (unsigned alteri = 0; alteri < this->lalterSet.size(); alteri++)
//	{
//			if (this->lalterSet.size() < m)
//	{
//		int alter = this->lalterSet[alteri];
		if (this->lpermitted[alter])
		{
			// Calculate the total contribution of all effects

			double contribution = 0;

			for (int i = 0; i < evaluationEffectCount; i++)
			{
				Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];
				contribution +=
					pEffect->parameter() *
					this->levaluationEffectContribution[alter][i];
			}

			if (this->lpNetworkCache->outTieExists(alter))
			{
				for (int i = 0; i < endowmentEffectCount; i++)
				{
					Effect * pEffect =
						this->pEndowmentFunction()->rEffects()[i];
					contribution +=
						pEffect->parameter() *
						this->lendowmentEffectContribution[alter][i];
				}
			}
			else
			{
				for (int i = 0; i < creationEffectCount; i++)
				{
					Effect * pEffect =
						this->pCreationFunction()->rEffects()[i];
					contribution +=
						pEffect->parameter() *
						this->lcreationEffectContribution[alter][i];
				}
			}

			// The selection probability is the exponential of the total
			// contribution.
			//	this->lprobabilities[alteri] = exp(contribution);

			this->lprobabilities[alter] = exp(contribution);
		}
		else
		{
			this->lprobabilities[alter] = 0;
		}

		total += this->lprobabilities[alter];
	}
//	}
	if (!this->oneModeNetwork())
	{
		total += 1;
		this->lprobabilities[m] = 1;
		m++;
	}
//	m = this->lalterSet.size();
//	if (!this->oneModeNetwork())
//	{
//		total += 1;
//		this->lprobabilities[this->lalterSet.size()] = 1;
//		m++;
//	}

	// Normalize

	if (total != 0)
	{
	   	for (int alter = 0; alter < m; alter++)
		{
			this->lprobabilities[alter] /= total;
		}
	}
}


/**
 * Updates the scores for effects according
 * to the current step in the simulation.
 */
void NetworkVariable::accumulateScores(int alter) const
{

 int m = this->m();
	for (unsigned i = 0;
		 i < this->pEvaluationFunction()->rEffects().size();
		 i++)
	{
		Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];
		double score = this->levaluationEffectContribution[alter][i];

		for (int j = 0; j < m; j++)
		//	for (unsigned alteri = 0; alteri < this->lalterSet.size(); alteri++)
		{
			//	if (this->lalterSet.size() < m)
			//{
			//int j = this->lalterSet[alteri];
			if (this->lpermitted[j])
			{
				score -=
					this->levaluationEffectContribution[j][i] *
					//				this->lprobabilities[alteri];
					this->lprobabilities[j];
			}
		}
		//	}
		this->pSimulation()->score(pEffect->pEffectInfo(),
			this->pSimulation()->score(pEffect->pEffectInfo()) + score);
	}

	for (unsigned i = 0;
		 i < this->pEndowmentFunction()->rEffects().size();
		 i++)
	{
		Effect * pEffect = this->pEndowmentFunction()->rEffects()[i];

		double score = 0;
		if (this->lpNetworkCache->outTieExists(alter))
		{
			score += this->lendowmentEffectContribution[alter][i];
		}

		for (int j = 0; j < m; j++)
		{
			if (this->lpNetworkCache->outTieExists(j) &&
				this->lpermitted[j])
			{
				score -=
					this->lendowmentEffectContribution[j][i] *
					this->lprobabilities[j];
			}
		}
		this->pSimulation()->score(pEffect->pEffectInfo(),
			this->pSimulation()->score(pEffect->pEffectInfo()) + score);
	}

	for (unsigned i = 0;
		 i < this->pCreationFunction()->rEffects().size();
		 i++)
	{
		Effect * pEffect = this->pCreationFunction()->rEffects()[i];

		double score = 0;

		if (!this->lpNetworkCache->outTieExists(alter))
		{
			score += this->lcreationEffectContribution[alter][i];
		}

		for (int j = 0; j < m; j++)
		{
			if (!this->lpNetworkCache->outTieExists(j) &&
				this->lpermitted[j])
			{
				score -=
					this->lcreationEffectContribution[j][i] *
					this->lprobabilities[j];
			}
		}
		this->pSimulation()->score(pEffect->pEffectInfo(),
			this->pSimulation()->score(pEffect->pEffectInfo()) + score);
	}
}


// ----------------------------------------------------------------------------
// Section: symmetric networks methods
// ----------------------------------------------------------------------------

/**
 * Checks whether the alter would like to create the proposed link to the
 * current ego.
 */
void NetworkVariable::checkAlterAgreement(int alter)
{
	this->pSimulation()->pCache()->initialize(alter);
	this->preprocessEgo(alter);

	this->calculateSymmetricTieFlipContributions(this->lego, 1);

	this->calculateSymmetricTieFlipProbabilities(this->lego, 1);

	double probability = exp(this->lsymmetricProbabilities[1]);

	probability = probability / (1.0 + probability);


	this->lsymmetricProbability = probability;
}

/**
 * Updates the scores for evaluation and endowment function effects according
 * to the current step in the simulation.
 */
void NetworkVariable::addAlterAgreementScores(bool accept)
{
	double probability = this->lsymmetricProbability;
	if (accept)
	{
		probability = 1 - probability;
	}

	for (unsigned i = 0;
		 i < this->pEvaluationFunction()->rEffects().size();
		 i++)
	{
		Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];

		double score = this->lsymmetricEvaluationEffectContribution[1][i]
			* probability;

		if (!accept)
		{
 			score = -1 * score;
 		}
		this->pSimulation()->score(pEffect->pEffectInfo(),
			this->pSimulation()->score(pEffect->pEffectInfo()) + score);
	}

	for (unsigned i = 0;
		 i < this->pEndowmentFunction()->rEffects().size();
		 i++)
	{
		Effect * pEffect = this->pEndowmentFunction()->rEffects()[i];

		double score = 0;

		if (this->lpNetworkCache->outTieExists(this->lego))
		{
			score = this->lsymmetricEndowmentEffectContribution[1][i]
				* probability;
		}
		if (!accept)
 		{
 			score = -1 * score;
 		}

		this->pSimulation()->score(pEffect->pEffectInfo(),
			this->pSimulation()->score(pEffect->pEffectInfo()) + score);
	}

	for (unsigned i = 0;
		i < this->pCreationFunction()->rEffects().size();
		i++)
	{
		Effect * pEffect = this->pCreationFunction()->rEffects()[i];

		double score = 0;

		if (!this->lpNetworkCache->outTieExists(this->lego))
		{
			score = this->lsymmetricCreationEffectContribution[1][i]
				* probability;
		}

		if (!accept)
 		{
 			score = -score;
 		}

		this->pSimulation()->score(pEffect->pEffectInfo(),
			this->pSimulation()->score(pEffect->pEffectInfo()) + score);
	}
}
/**
 * Updates the scores for evaluation and endowment function effects according
 * to the current step in the simulation.
 */

void NetworkVariable::accumulateSymmetricModelScores(int alter, bool accept)
{
	double score = 0;
	double prEgo = 0;
	double prAlter = 0;
	double prSum = 0;

	switch(this->pSimulation()->pModel()->modelType())
	{
	case BFORCE:
		prEgo = this->lsymmetricProbabilities[0];
		for (unsigned i = 0;
			 i < this->pEvaluationFunction()->rEffects().size();
			 i++)
		{
			Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];
			if (accept)
			{
				score = this->lsymmetricEvaluationEffectContribution[0][i]
					* (1 - prEgo);
			}
			else
			{
				score = -1 * this->lsymmetricEvaluationEffectContribution[0][i]
					* prEgo;
			}

			this->pSimulation()->score(pEffect->pEffectInfo(),
				this->pSimulation()->score(pEffect->pEffectInfo()) + score);
		}

		for (unsigned i = 0;
			 i < this->pEndowmentFunction()->rEffects().size();
			 i++)
		{
			Effect * pEffect = this->pEndowmentFunction()->rEffects()[i];

			if (this->lpNetworkCache->outTieExists(alter))
			{
				if (accept)
				{
					score =
						this->lsymmetricEndowmentEffectContribution[0][i]
						* (1 - prEgo);
				}
				else
				{
					score =  -1  * prEgo *
						this->lsymmetricEndowmentEffectContribution[0][i];
				}
			}
			this->pSimulation()->score(pEffect->pEffectInfo(),
				this->pSimulation()->score(pEffect->pEffectInfo()) + score);
		}

		for (unsigned i = 0;
			i < this->pCreationFunction()->rEffects().size();
			i++)
		{
			Effect * pEffect = this->pCreationFunction()->rEffects()[i];

			if (!this->lpNetworkCache->outTieExists(alter))
			{
				if (accept)
				{
					score =
						this->lsymmetricCreationEffectContribution[0][i] *
							(1 - prEgo);
				}
				else
				{
					score =  -prEgo *
						this->lsymmetricCreationEffectContribution[0][i];
				}
			}

			this->pSimulation()->score(pEffect->pEffectInfo(),
				this->pSimulation()->score(pEffect->pEffectInfo()) + score);
		}

		break;

	case BAGREE:
		prEgo = this->lsymmetricProbabilities[0];
		prAlter = this->lsymmetricProbabilities[1];

		for (unsigned i = 0;
			 i < this->pEvaluationFunction()->rEffects().size();
			 i++)
		{
			Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];
			if (!this->lpNetworkCache->outTieExists(alter))
			{
				if (accept)
				{
					score = this->lsymmetricEvaluationEffectContribution[0][i]
						* (1 - prEgo)  +
						this->lsymmetricEvaluationEffectContribution[1][i]
						* (1 - prAlter);
				}
				else
				{
					score =  -1  * prAlter * prEgo * ((1 - prEgo) *
						this->lsymmetricEvaluationEffectContribution[0][i] +
							(1 - prAlter) *
						this->lsymmetricEvaluationEffectContribution[1][i]) /
						(1 - prEgo * prAlter);
				}
			}
			else
			{
				if (accept)
				{
					score = (1 - prEgo) * (1 - prAlter) *
						(this->lsymmetricEvaluationEffectContribution[0][i]
							* prEgo +
						this->lsymmetricEvaluationEffectContribution[1][i]
							* prAlter) /
						(prEgo + prAlter - prEgo * prAlter);
				}
				else
				{
					score = -1 *
						((this->lsymmetricEvaluationEffectContribution[0][i]
							* prEgo) +
							(this->lsymmetricEvaluationEffectContribution[1][i]
								* prAlter));
				}
			}

			this->pSimulation()->score(pEffect->pEffectInfo(),
				this->pSimulation()->score(pEffect->pEffectInfo()) + score);
		}

		for (unsigned i = 0;
			 i < this->pEndowmentFunction()->rEffects().size();
			 i++)
		{
			Effect * pEffect = this->pEndowmentFunction()->rEffects()[i];

			if (this->lpNetworkCache->outTieExists(alter))
			{
				if (accept)
				{
					score = (1 - prEgo) * (1 - prAlter) *
						(this->lsymmetricEndowmentEffectContribution[0][i]
							* prEgo + this->
							lsymmetricEndowmentEffectContribution[1][i]
							* prAlter) /
						(prEgo + prAlter - prEgo * prAlter);
				}
				else
				{
					score = -1 *
						((this->lsymmetricEndowmentEffectContribution[0][i]
							* prEgo) + (this->
								lsymmetricEndowmentEffectContribution[1][i]
								* prAlter));
				}
			}

			this->pSimulation()->score(pEffect->pEffectInfo(),
				this->pSimulation()->score(pEffect->pEffectInfo()) + score);
		}

		for (unsigned i = 0;
			 i < this->pCreationFunction()->rEffects().size();
			 i++)
		{
			Effect * pEffect = this->pCreationFunction()->rEffects()[i];

			if (!this->lpNetworkCache->outTieExists(alter))
			{
				if (accept)
				{
					score = (1 - prEgo) * (1 - prAlter) *
						(this->lsymmetricCreationEffectContribution[0][i] *
							prEgo +
							this->lsymmetricCreationEffectContribution[1][i] *
							prAlter) /
						(prEgo + prAlter - prEgo * prAlter);
				}
				else
				{
					score = -1 *
						(this->lsymmetricCreationEffectContribution[0][i] *
							prEgo +
							this->lsymmetricCreationEffectContribution[1][i] *
							prAlter);
				}
			}

			this->pSimulation()->score(pEffect->pEffectInfo(),
				this->pSimulation()->score(pEffect->pEffectInfo()) + score);
		}

		break;
	case BJOINT:

		prEgo = this->lsymmetricProbabilities[0];
		prAlter = this->lsymmetricProbabilities[1];
		prSum = exp(prEgo + prAlter);
		prSum = prSum / (1 + prSum);
		if (!accept)
		{
			prSum = 1 - prSum;
		}
		for (unsigned i = 0;
			 i < this->pEvaluationFunction()->rEffects().size();
			 i++)
		{
			Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];
			score =  (1 - prSum) *
				(this->lsymmetricEvaluationEffectContribution[0][i] +
					this->lsymmetricEvaluationEffectContribution[1][i]);

			if (!accept)
			{
				score = -1 * score;
			}

			this->pSimulation()->score(pEffect->pEffectInfo(),
				this->pSimulation()->score(pEffect->pEffectInfo()) + score);
		}

		for (unsigned i = 0;
			 i < this->pEndowmentFunction()->rEffects().size();
			 i++)
		{
			Effect * pEffect = this->pEndowmentFunction()->rEffects()[i];

			if (this->lpNetworkCache->outTieExists(alter))
			{
				score = (1 - prSum) *
					(this->lsymmetricEndowmentEffectContribution[0][i] +
						this->lsymmetricEndowmentEffectContribution[1][i]);

				if (!accept)
				{
					score = -1 * score;
				}
				this->pSimulation()->score(pEffect->pEffectInfo(),
					this->pSimulation()->score(pEffect->pEffectInfo()) + score);
			}
		}

		if (!this->lpNetworkCache->outTieExists(alter))
		{
			for (unsigned i = 0;
				 i < this->pCreationFunction()->rEffects().size();
				 i++)
			{
				Effect * pEffect = this->pCreationFunction()->rEffects()[i];

				score = (1 - prSum) *
					(this->lsymmetricCreationEffectContribution[0][i] +
						this->lsymmetricCreationEffectContribution[1][i]);

				if (!accept)
				{
					score = -score;
				}

				this->pSimulation()->score(pEffect->pEffectInfo(),
					this->pSimulation()->score(pEffect->pEffectInfo()) +
						score);
			}
		}

		break;

	case NORMAL:
	case AFORCE:
	case AAGREE:
	case NOTUSED:
		break;
	}
}

/**
 * For the given alter, this method calculates the contribution of each
 * effect if a tie from the ego to the alter
 * was flipped. These contributions are stored in arrays
 * <code>lsymmetricEvaluationEffectContribution</code>,
 * <code>lsymmetricEndowmentEffectContribution</code>, and
 * <code>lsymmetricCreationEffectContribution</code>. The first entry of the
 * array is for the ego effects, the second for the alter, controlled by the
 * integer parameter sub (0 or 1), as we need to do it both ways round.
 * Always permitted, never bipartite. Since always permitted we don't need
 * nan's to indicate not permitted.
 */
void NetworkVariable::calculateSymmetricTieFlipContributions(int alter,
int sub)
{
	int evaluationEffectCount = this->pEvaluationFunction()->rEffects().size();
	int endowmentEffectCount = this->pEndowmentFunction()->rEffects().size();
	int creationEffectCount = this->pCreationFunction()->rEffects().size();
	const vector<Effect *> & rEvaluationEffects =
		this->pEvaluationFunction()->rEffects();
	const vector<Effect *> & rEndowmentEffects =
		this->pEndowmentFunction()->rEffects();
	const vector<Effect *> & rCreationEffects =
		this->pCreationFunction()->rEffects();

	for (int i = 0; i < evaluationEffectCount; i++)
	{
		NetworkEffect * pEffect =
			(NetworkEffect *) rEvaluationEffects[i];
		double contribution =
			pEffect->calculateContribution(alter);

		// Tie withdrawals contribute in the opposite way

		if (this->lpNetworkCache->outTieExists(alter))
 		{
 			contribution = -contribution;
 		}
		this->lsymmetricEvaluationEffectContribution[sub][i] = contribution;
	}

	// The endowment effects have non-zero contributions on tie
	// withdrawals only. The opposite is true for tie creation effects.

	if (this->lpNetworkCache->outTieExists(alter) )
	{
		for (int i = 0; i < endowmentEffectCount; i++)
		{
			NetworkEffect * pEffect =
				(NetworkEffect *) rEndowmentEffects[i];
			this->lsymmetricEndowmentEffectContribution[sub][i] =
				-pEffect->calculateContribution(alter);
		}

		for (int i = 0; i < creationEffectCount; i++)
		{
			this->lsymmetricCreationEffectContribution[sub][i] = 0;
		}
	}
	else
	{
		for (int i = 0; i < creationEffectCount; i++)
		{
			NetworkEffect * pEffect =
				(NetworkEffect *) rCreationEffects[i];
			this->lsymmetricCreationEffectContribution[sub][i] =
				pEffect->calculateContribution(alter);
		}

		for (int i = 0; i < endowmentEffectCount; i++)
		{
			this->lsymmetricEndowmentEffectContribution[sub][i] = 0;
		}
	}

}

/**
 * Calculates the linear combinations which will be used to create the
 * probabilities of accepting change for symmetric networks.
 */
void NetworkVariable::calculateSymmetricTieFlipProbabilities(int alter, int sub)
{
	int evaluationEffectCount = this->pEvaluationFunction()->rEffects().size();
	int endowmentEffectCount = this->pEndowmentFunction()->rEffects().size();
	int creationEffectCount = this->pCreationFunction()->rEffects().size();

	// Calculate the total contribution of all effects

	double contribution = 0;

	for (int i = 0; i < evaluationEffectCount; i++)
	{
		Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];
		contribution +=
			pEffect->parameter() *
			this->lsymmetricEvaluationEffectContribution[sub][i];
	}

	if (this->lpNetworkCache->outTieExists(alter))
	{
		for (int i = 0; i < endowmentEffectCount; i++)
		{
			Effect * pEffect =
				this->pEndowmentFunction()->rEffects()[i];
			contribution +=
				pEffect->parameter() *
				this->lsymmetricEndowmentEffectContribution[sub][i];
		}
	}
	else
	{

		for (int i = 0; i < creationEffectCount; i++)
		{
			Effect * pEffect = this->pCreationFunction()->rEffects()[i];
			contribution +=
				pEffect->parameter() *
					this->lsymmetricCreationEffectContribution[sub][i];
		}
	}

	this->lsymmetricProbabilities[sub] = contribution;

}
/**
 * Proposes and calculates probabilities for change.
 * Used for symmetric networks with model types beginning with B only.
 */
bool NetworkVariable::calculateModelTypeBProbabilities()
{
	this->preprocessEgo(this->lego);
	this->calculatePermissibleChanges();

	// choose alter
	int alter = this->lego;

	double * cumulativeRates = new double[this->n()];

	int numberPermitted = 0;
	for (int i = 0; i < this->n(); i++)
	{
		if (this->lpermitted[i] && i != this->lego)
		{
			numberPermitted++;
		}
		cumulativeRates[i] = this->rate(i);
		if (i > 0)
		{
			cumulativeRates[i] += cumulativeRates[i - 1];
		}

    }

	if (numberPermitted > 1)
	{
		while (alter == this->lego)
		{
			alter = nextIntWithCumulativeProbabilities(this->n(),
				cumulativeRates);
		}
	}

	this->lalterProbability = this->rate(alter) / cumulativeRates[this->n()];

	delete [] cumulativeRates;

	this->lalter = alter;
//	Rprintf("net ego %d alter %d\n",this->lego, this->lalter);

	if (numberPermitted == 0 ||  (!this->lpermitted[alter]) ||
		alter == this->lego)
	{
		return false;
	}

	// calculate the probabilities

	double probability = 0;

	this->pSimulation()->pCache()->initialize(alter);
	this->preprocessEgo(alter);
	this->calculateSymmetricTieFlipContributions(this->lego, 1);
	this->calculateSymmetricTieFlipProbabilities(this->lego, 1);

	this->pSimulation()->pCache()->initialize(this->lego);
	this->preprocessEgo(this->lego);
	this->calculateSymmetricTieFlipContributions(alter, 0);
	this->calculateSymmetricTieFlipProbabilities(alter, 0);
	switch(this->pSimulation()->pModel()->modelType())
	{
	case BFORCE:

		probability = exp(this->lsymmetricProbabilities[0]);
		probability = probability / (1.0 + probability);
		this->lsymmetricProbabilities[0] = probability;
		break;

	case BAGREE:
		probability = exp(this->lsymmetricProbabilities[0]);
		probability = probability / (1.0 + probability);
		this->lsymmetricProbabilities[0] = probability;
		probability = exp(this->lsymmetricProbabilities[1]);
		probability = probability / (1.0 + probability);
		this->lsymmetricProbabilities[1] = probability;

		if (!this->lpNetworkCache->outTieExists(alter))
		{
			probability = this->lsymmetricProbabilities[0] *
				this->lsymmetricProbabilities[1];
		}
		else
		{
			probability = this->lsymmetricProbabilities[0] +
				this->lsymmetricProbabilities[1] -
				this->lsymmetricProbabilities[0] *
				this->lsymmetricProbabilities[1];
		}
		break;

	case BJOINT:

		probability = exp(this->lsymmetricProbabilities[0] +
			this->lsymmetricProbabilities[1]);
		probability = probability / (1.0 + probability);
		break;

	case NORMAL:
	case AFORCE:
	case AAGREE:
	case NOTUSED:
		break;
	}

	this->lsymmetricProbability = probability;

	return true;
	// accept = nextDouble() < probability;

	// if (this->pSimulation()->pModel()->needScores())
	// {
	// 	this->accumulateSymmetricModelScores(alter, accept);
	// }

	// return accept;
}

// ----------------------------------------------------------------------------
// Section: Maximum likelihood related methods
// ----------------------------------------------------------------------------

/**
 * Calculates the probability of the given ministep assuming that the
 * ego of the ministep will change this variable.
 */
double NetworkVariable::probability(MiniStep * pMiniStep)
{
	// Initialize the cache object for the current ego
	this->pSimulation()->pCache()->initialize(pMiniStep->ego());

	NetworkChange * pNetworkChange =
		dynamic_cast<NetworkChange *>(pMiniStep);
	this->lego = pNetworkChange->ego();
	if (this->pSimulation()->pModel()->modelTypeB())
	{
		this->calculateModelTypeBProbabilities();
		if (this->pSimulation()->pModel()->needScores())
		{
			this->accumulateSymmetricModelScores(pNetworkChange->alter(),
				!pNetworkChange->diagonal());
		}
		// TODO no derivatives for symmetric models yet
		// if (this->pSimulation()->pModel()->needDerivatives())
		// {
		// 	this->accumulateDerivatives();
		// }
	}
	else
	{
		this->calculateTieFlipProbabilities();
		if (this->pSimulation()->pModel()->needScores())
		{
			this->accumulateScores(pNetworkChange->alter());
		}
		if (this->pSimulation()->pModel()->needDerivatives())
		{
			this->accumulateDerivatives();
		}
		if (this->pSimulation()->pModel()->needChangeContributions())
		{
			this->copyChangeContributions(pMiniStep);
		}
	}
	return this->lprobabilities[pNetworkChange->alter()];
}

/**
 * Updates the derivatives for effects
 * according to the current miniStep in the chain.
 */
void NetworkVariable::accumulateDerivatives() const
{
	int totalEvaluationEffects = this->pEvaluationFunction()->rEffects().size();
	int totalEndowmentEffects = this->pEndowmentFunction()->rEffects().size();
	int totalCreationEffects = this->pCreationFunction()->rEffects().size();
	int totalEffects =
		totalEvaluationEffects + totalEndowmentEffects + totalCreationEffects;
	Effect * pEffect1;
	Effect * pEffect2;
	double derivative;
	double * product = new double[totalEffects];
	double contribution1 = 0.0;
	double contribution2 = 0.0;

//	Rprintf("%d %d %d\n",totalEvaluationEffects,totalEndowmentEffects,
//		totalEffects);
	for (int effect1 = 0; effect1 < totalEffects; effect1++)
	{
		product[effect1] = 0.0;
		int endowment1 = effect1 - totalEvaluationEffects;
		int creation1 = effect1 - totalEvaluationEffects - totalEndowmentEffects;

		if (effect1 < totalEvaluationEffects)
		{
			pEffect1 = this->pEvaluationFunction()->rEffects()[effect1];
		}
		else if (effect1 < totalEvaluationEffects + totalEndowmentEffects)
		{
			pEffect1 = this->pEndowmentFunction()->rEffects()[endowment1];
		}
		else
		{
			pEffect1 = this->pCreationFunction()->rEffects()[creation1];
		}

		for (int alter = 0; alter < this->m(); alter++)
		{
			//	Rprintf("accum %d %d %d\n", alter, this->lpermitted[alter], this->lego);
			if (this->lpermitted[alter])
			{
				if (effect1 < totalEvaluationEffects)
				{
					product[effect1] +=
						this->levaluationEffectContribution[alter][effect1] *
						this->lprobabilities[alter];
				}
				else if (effect1 < totalEvaluationEffects + totalEndowmentEffects)
				{
					product[effect1] +=
						this->lendowmentEffectContribution[alter][endowment1] *
						this->lprobabilities[alter];
				}
				else
				{
					product[effect1] +=
						this->lcreationEffectContribution[alter][creation1] *
						this->lprobabilities[alter];
				}

				//	Rprintf("%d %d %f\n", alter, effect1, product[effect1]);
			}
		}
		for (int effect2 = effect1; effect2 < totalEffects; effect2++)
		{
			int endowment2 = effect2 - totalEvaluationEffects;
			int creation2 = effect2 - totalEvaluationEffects -
				totalEndowmentEffects;
			derivative = 0.0;

			if (effect2 < totalEvaluationEffects)
			{
				pEffect2 = this->pEvaluationFunction()->rEffects()[effect2];
			}
			else if (effect2 < totalEvaluationEffects + totalEndowmentEffects)
			{
				pEffect2 =
					this->pEndowmentFunction()->rEffects()[endowment2];
			}
			else
			{
				pEffect2 = this->pCreationFunction()->rEffects()[creation2];
			}

			for (int alter = 0; alter < this->m(); alter++)
			{
				if (this->lpermitted[alter])
				{
					if (effect1 < totalEvaluationEffects)
					{
						contribution1 =
							this->levaluationEffectContribution[alter][effect1];
					}
					else if (effect1 <
						totalEvaluationEffects + totalEndowmentEffects)
					{
						contribution1 =
							this->lendowmentEffectContribution[alter][endowment1];
					}
					else
					{
						contribution1 =
							this->lcreationEffectContribution[alter][creation1];
					}

					if (effect2 < totalEvaluationEffects)
					{
						contribution2 =
							this->levaluationEffectContribution[alter][effect2];
					}
					else if (effect2 <
						totalEvaluationEffects + totalEndowmentEffects)
					{
						contribution2 =
							this->lendowmentEffectContribution[alter][endowment2];
					}
					else
					{
						contribution2 =
							this->lcreationEffectContribution[alter][creation2];
					}


					derivative -=
						contribution1 * contribution2 *
						this->lprobabilities[alter];
					// Rprintf("deriv 2 %d %d %d %d %d %f %f %f %f %x %x\n",
					//alter,
					// 	effect1,
					// 	effect2, relativeEffect1, relativeEffect2,
					// 	derivative, contribution1, contribution2,
					// 	//	this->levaluationEffectContribution[alter][effect1],
					// 	//this->levaluationEffectContribution[alter][effect2],
					// 	this->lprobabilities[alter], pEffect1, pEffect2);
				}
			}

			this->pSimulation()->derivative(pEffect1->pEffectInfo(),
				pEffect2->pEffectInfo(),
				this->pSimulation()->derivative(pEffect1->pEffectInfo(),
					pEffect2->pEffectInfo()) +	derivative);
		}
	}

	for (int effect1 = 0; effect1 < totalEffects; effect1++)
	{
		int endowment1 = effect1 - totalEvaluationEffects;
		int creation1 = effect1 - totalEvaluationEffects - totalEndowmentEffects;

		for (int effect2 = effect1; effect2 < totalEffects; effect2++)
		{
			int endowment2 = effect2 - totalEvaluationEffects;
			int creation2 = effect2 - totalEvaluationEffects -
				totalEndowmentEffects;
			if (effect1 < totalEvaluationEffects)
			{
				pEffect1 = this->pEvaluationFunction()->rEffects()[effect1];
			}
			else if (effect1 < totalEvaluationEffects + totalEndowmentEffects)
			{
				pEffect1 = this->pEndowmentFunction()->rEffects()[endowment1];
			}
			else
			{
				pEffect1 = this->pCreationFunction()->rEffects()[creation1];
			}

			if (effect2 < totalEvaluationEffects)
			{
				pEffect2 = this->pEvaluationFunction()->rEffects()[effect2];
			}
			else if (effect2 < totalEvaluationEffects + totalEndowmentEffects)
			{
				pEffect2 = this->pEndowmentFunction()->rEffects()[endowment2];
			}
			else
			{
				pEffect2 = this->pCreationFunction()->rEffects()[creation2];
			}

			this->pSimulation()->derivative(pEffect1->pEffectInfo(),
				pEffect2->pEffectInfo(),
				this->pSimulation()->derivative(pEffect1->pEffectInfo(),
						pEffect2->pEffectInfo()) +
					product[effect1] * product[effect2]);
		}
	}

	delete [] product;
}

//	Rprintf("deriv %f\n", derivative;



/**
 * Returns whether applying the given ministep on the current state of this
 * variable would be valid with respect to all constraints. One can disable
 * the checking of up-only and down-only conditions.
 */
bool NetworkVariable::validMiniStep(const MiniStep * pMiniStep,
	bool checkUpOnlyDownOnlyConditions) const
{
	bool valid = DependentVariable::validMiniStep(pMiniStep);

	if (valid && !pMiniStep->diagonal())
	{
		NetworkLongitudinalData * pData =
			(NetworkLongitudinalData *) this->pData();
		const NetworkChange * pNetworkChange =
			dynamic_cast<const NetworkChange *>(pMiniStep);
		int i = pNetworkChange->ego();
		int j = pNetworkChange->alter();

		if (this->lpNetwork->tieValue(i, j))
		{
			if (checkUpOnlyDownOnlyConditions)
			{
				valid = !pData->upOnly(this->period());
			}
		}
		else
		{
			if (checkUpOnlyDownOnlyConditions)
			{
				valid = !pData->downOnly(this->period());
			}

			valid &= this->lpNetwork->outDegree(i) < pData->maxDegree() &&
				this->pReceivers()->active(j);
		}

		if (valid)
		{
			valid = !pData->structural(i, j, this->period());
		}

		// The filters may add some more conditions.

		for (unsigned i = 0;
			i < this->lpermittedChangeFilters.size() && valid;
			i++)
		{
			PermittedChangeFilter * pFilter = this->lpermittedChangeFilters[i];
			valid = pFilter->validMiniStep(pNetworkChange);
		}
	}

	return valid;
}


/**
 * Generates a random ministep for the given ego.
 */
MiniStep * NetworkVariable::randomMiniStep(int ego)
{
	this->pSimulation()->pCache()->initialize(ego);
	this->lego = ego;
	this->calculateTieFlipProbabilities();

	int m = 0;
	if (this->oneModeNetwork())
	{
		m = this->m();
	}
	else
	{
		m = this->m() + 1;
	}
	int alter = nextIntWithProbabilities(m, this->lprobabilities);

	MiniStep * pMiniStep =
		new NetworkChange(this->lpData, ego, alter,
			this->diagonalMiniStep(ego, alter));
	pMiniStep->logChoiceProbability(log(this->lprobabilities[alter]));

	return pMiniStep;
}


/**
 * Returns if the observed value for the option of the given ministep
 * is missing at either end of the period.
 */
bool NetworkVariable::missing(const MiniStep * pMiniStep) const
{
	const NetworkChange * pNetworkChange =
		dynamic_cast<const NetworkChange *>(pMiniStep);

	return this->lpData->missing(pNetworkChange->ego(),
		pNetworkChange->alter(),
		this->period()) ||
		this->lpData->missing(pNetworkChange->ego(),
			pNetworkChange->alter(),
			this->period() + 1);
}

/**
 * Returns if the given ministep is structurally determined in the period.
 */
bool NetworkVariable::structural(const MiniStep * pMiniStep) const
{
	const NetworkChange * pNetworkChange =
		dynamic_cast<const NetworkChange *>(pMiniStep);
	return !pMiniStep->diagonal() &&
		this->lpData->structural(pNetworkChange->ego(),
			pNetworkChange->alter(),
			this->period());
}
/**
 * Copies the change contributions for effects according to the
 * current miniStep.
 */
void NetworkVariable::copyChangeContributions(MiniStep * pMiniStep) const
{
	 NetworkChange * pNetworkChange =
		dynamic_cast< NetworkChange *>(pMiniStep);

	 int nEvaluationEffects = this->pEvaluationFunction()->rEffects().size();
	 int nEndowmentEffects = this->pEndowmentFunction()->rEffects().size();
	 int nCreationEffects = this->pCreationFunction()->rEffects().size();
	 pNetworkChange->allocateEffectContributionArrays(nEvaluationEffects,
		 nEndowmentEffects,
		 nCreationEffects,
		 this->m());

	 for (int alter = 0; alter < this->m(); alter++)
	 {
		for (unsigned i = 0;
			 i < this->pEvaluationFunction()->rEffects().size(); i++)
		{
			pNetworkChange->evaluationEffectContribution(
				this->levaluationEffectContribution[alter][i], alter, i);
		}

		for (unsigned i = 0;
			 i < this->pEndowmentFunction()->rEffects().size(); i++)
		{
			pNetworkChange->endowmentEffectContribution(
				this->lendowmentEffectContribution[alter][i], alter, i);
		}

		for (unsigned i = 0;
			 i < this->pCreationFunction()->rEffects().size(); i++)
		{
			pNetworkChange->creationEffectContribution(
				this->lcreationEffectContribution[alter][i],
				alter,
				i);
		}
	}
}


/**
 * Calculates the log probability of the choice of this ministep,
 * using stored change contributions.
 *
 */
double NetworkVariable::calculateChoiceProbability(const MiniStep * pMiniStep)
const
{
	const NetworkChange * pNetworkChange =
		dynamic_cast< const NetworkChange *>(pMiniStep);
	int evaluationEffectCount = this->pEvaluationFunction()->rEffects().size();
	int endowmentEffectCount = this->pEndowmentFunction()->rEffects().size();
	int creationEffectCount = this->pCreationFunction()->rEffects().size();

	double total = 0;
	double * probabilities = new double[this->m()];
	double value;

	for (int alter = 0; alter < this->m(); alter++)
	{
		double contribution = 0;

		for (int i = 0; i < evaluationEffectCount; i++)
		{
			Effect * pEffect = this->pEvaluationFunction()->rEffects()[i];
			contribution += pEffect->parameter() *
				pNetworkChange->evaluationEffectContribution(alter, i);
			//	Rprintf("%d %d %d %f %f \n",alter, i, pNetworkChange->ego(),
			//	pEffect->parameter(),
			//	pNetworkChange->evaluationEffectContribution(alter, i));
		}

		for (int i = 0; i < endowmentEffectCount; i++)
		{
			Effect * pEffect = this->pEndowmentFunction()->rEffects()[i];
			contribution +=	pEffect->parameter() *
				pNetworkChange->endowmentEffectContribution(alter, i);
		}

		for (int i = 0; i < creationEffectCount; i++)
		{
			Effect * pEffect = this->pCreationFunction()->rEffects()[i];
			contribution +=	pEffect->parameter() *
				pNetworkChange->creationEffectContribution(alter, i);
		}

		// The selection probability is the exponential of the total
		// contribution.

		probabilities[alter] = exp(contribution);
		if (R_IsNaN(probabilities[alter]))
		{
			PrintValue(getMiniStepDF(*pNetworkChange));
		}
		total += probabilities[alter];

	}

	// Normalize

	if (total != 0)
	{
		for (int alter = 0; alter < this->m(); alter++)
		{
			probabilities[alter] /= total;
		}
	}
	value = log(probabilities[pNetworkChange->alter()]);
	delete[] probabilities;
	return value;
}


// ----------------------------------------------------------------------------
// Section: Properties
// ----------------------------------------------------------------------------

/**
 * Returns if this is a network variable.
 */
bool NetworkVariable::networkVariable() const
{
	return true;
}

/**
 * Returns if this is a symmetric network variable.
 */
bool NetworkVariable::symmetric() const
{
	const OneModeNetworkLongitudinalData * pData =
		dynamic_cast<const OneModeNetworkLongitudinalData *>(this->lpData);
	if (pData)
	{
		return pData->symmetric();
	}
	else
	{
		return false;
	}
}

/**
 * Returns if there are any constraints on the permitted changes of this
 * variable.
 */
bool NetworkVariable::constrained() const
{
	return DependentVariable::constrained() ||
		!this->lpermittedChangeFilters.empty();
}

/**
 * Returns the value of the alter in the current step. Only used for model type
 * B with symmetric networks.
 */
int NetworkVariable::alter() const
{
//	Rprintf("in net %d\n", this->lalter);
	return this->lalter;
}

/**
 * Returns whether this is a diagonal step, for non-symmetric cases only
 */
bool NetworkVariable::diagonalMiniStep(int ego, int alter) const
{
	return (!this->oneModeNetwork() && alter == this->m()) ||
		(this->oneModeNetwork() && ego == alter);
}

}
