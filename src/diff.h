/*
 * diff.h
 *
 *  Created on: Jul 3, 2013
 *      Author: fatbeard
 */

#ifndef DIFF_H_
#define DIFF_H_

#include "main.h"

class CDiff
{
protected:
	static const int64 nTargetTimespan = 1 * 2 * 60 * 60; // Noirbits: 2 hour
	static const int64 nTargetSpacing = 120; // Noirbits: 2 minute blocks
	static const int64 nInterval = nTargetTimespan / nTargetSpacing;
	// Thanks: Balthazar for suggesting the following fix
	// https://bitcointalk.org/index.php?topic=182430.msg1904506#msg1904506
	static const int64 nReTargetHistoryFact = 4;

	// Fields
	CBigNum 		bnProofOfWorkLimit;
	unsigned int	nProofOfWorkLimit;

public:

	CDiff(CBigNum bnProofOfWorkLimit)
	{
		this->bnProofOfWorkLimit = bnProofOfWorkLimit;
		this->nProofOfWorkLimit = bnProofOfWorkLimit.GetCompact();
	}

	virtual unsigned int 	ComputeMinWork(unsigned int nBase, int64 nTime) = 0;
	virtual unsigned int 	GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock) = 0;
};


class CMainNetDiff : public CDiff
{
private:
	// How many retargets in history to include for diff recalc.
	static const int 			nRetargetHistoryFact = 4;
	// Height at which to apply new rules.
	static const int 			nHeightForNewRules = 25000;
	// Max. span between two retargets.
	static const unsigned int 	nMaxTimeInterval = 14400;

	// Methods
	int64				GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast);
	int 				GetBlocksToGoBack(const CBlockIndex* pindexLast);
	const CBlockIndex*	GetFirstBlock(const CBlockIndex* pindexLast);

protected:

	bool 				ShouldApplyNewRetargetRules(const CBlockIndex* pindexLast);
	bool 				ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock *pblock);

public:

	CMainNetDiff(CBigNum bnProofOfWorkLimit) : CDiff(bnProofOfWorkLimit)
	{ }

	virtual unsigned int 	ComputeMinWork(unsigned int nBase, int64 nTime);
	virtual unsigned int 	GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock);

};

class CTestNetDiff : public CMainNetDiff
{
private:

	// Methods
	unsigned int 	GetTestNetNextTarget(const CBlockIndex* pindexLast, const CBlock *pblock);

public:

	CTestNetDiff(CBigNum bnProofOfWorkLimit) : CMainNetDiff(bnProofOfWorkLimit)
		{ }

	virtual unsigned int 	ComputeMinWork(unsigned int nBase, int64 nTime);
	virtual unsigned int 	GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock);

};

#endif /* DIFF_H_ */
