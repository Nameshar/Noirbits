/*
 * diff.h
 *
 *  Created on: Jul 3, 2013
 *      Author: fatbeard
 */

#ifndef DIFF_H_
#define DIFF_H_

#include "main.h"

struct SRetargetParams
{
	int64 nTargetTimespan;
	int64 nTargetSpacing;
	int64 nInterval;

	SRetargetParams(int64 nTargetTimespan, int64 nTargetSpacing)
	{
		this->nTargetTimespan = nTargetTimespan;
		this->nTargetSpacing = nTargetSpacing;
		this->nInterval = nTargetTimespan / nTargetSpacing;
	}
};

class CDiff
{
protected:
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
	// Thanks: Balthazar for suggesting the following fix
	// https://bitcointalk.org/index.php?topic=182430.msg1904506#msg1904506
	static const int 			nRetargetHistoryFact = 4;
	// Height at which to apply new rules.
	static const int 			nMinHeightForNewRules = 20000;
	// Max. span between two retargets.
	static const unsigned int 	nMaxTimeInterval = 14400;

	// Methods
	int64				GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast);
	int 				GetBlocksToGoBack(const CBlockIndex* pindexLast);
	const CBlockIndex*	GetFirstBlock(const CBlockIndex* pindexLast);

protected:

	bool 				ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock *pblock);

public:
	static const SRetargetParams* sOldRules;
	static const SRetargetParams* sNewRules;

	CMainNetDiff(CBigNum bnProofOfWorkLimit) : CDiff(bnProofOfWorkLimit)
	{ }

	virtual unsigned int 	ComputeMinWork(unsigned int nBase, int64 nTime);
	virtual unsigned int 	GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock);

	static bool ShouldApplyNewRetargetRules(int nHeight)
	{
		return nHeight + 1 >= nMinHeightForNewRules;
	}
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
