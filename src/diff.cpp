/*
 * diff.cpp
 *
 *  Created on: Jul 3, 2013
 *      Author: fatbeard
 */

#include "diff.h"

bool CMainNetDiff::ShouldApplyNewRetargetRules(const CBlockIndex* pindexLast)
{
	int nMinHeightForNewRules = 25000;
	return pindexLast->nHeight + 1 > nMinHeightForNewRules;
}

bool CMainNetDiff::ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock *pblock)
{
	bool bShouldRetarget = false;

	if (ShouldApplyNewRetargetRules(pindexLast))
	{
		// We have exceeded max. time for current difficulty, change
		bShouldRetarget |= (pindexLast->nTime + nMaxTimeInterval) < pblock->nTime;
	}

	// We have reached retarget height
	bShouldRetarget |= (pindexLast->nHeight + 1) % nInterval == 0;

	return bShouldRetarget;
}

int64 CMainNetDiff::GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast)
{
	int64 nActualTimespan = 0;
	bool useNewRules = ShouldApplyNewRetargetRules(pindexLast);

	if (pindexLast->nHeight > COINFIX1_BLOCK && !useNewRules)
	{
		// obtain average actual timespan
		nActualTimespan = (pindexLast->GetBlockTime() - pindexFirst->GetBlockTime()) / nReTargetHistoryFact;
	}
	else
	{
		nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
	}


	printf("  nActualTimespan = %"PRI64d"  before bounds\n", nActualTimespan);

	if (nActualTimespan < nTargetTimespan / 4) nActualTimespan = nTargetTimespan / 4;
	if (nActualTimespan > nTargetTimespan * 4) nActualTimespan = nTargetTimespan * 4;

	return nActualTimespan;
}

const CBlockIndex* CMainNetDiff::GetFirstBlock(const CBlockIndex* pindexLast)
{
	const CBlockIndex* pindexFirst = pindexLast;
	for (int i = 0; pindexFirst && i < GetBlocksToGoBack(pindexLast); i++)
	{
		pindexFirst = pindexFirst->pprev;
	}

	assert(pindexFirst);

	return pindexFirst;
}

int CMainNetDiff::GetBlocksToGoBack(const CBlockIndex* pindexLast)
{
	// Fixes an issue where a 51% attack can change difficulty at will.
	// Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
	int nBlocksToGoBack = nInterval - 1;
	if ((pindexLast->nHeight + 1) != nInterval)
	{
		nBlocksToGoBack = nInterval;
	}

	if (pindexLast->nHeight > COINFIX1_BLOCK)
	{
		nBlocksToGoBack = nReTargetHistoryFact * nInterval;
	}

	return nBlocksToGoBack;
}

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int CMainNetDiff::ComputeMinWork(unsigned int nBase, int64 nTime)
{
	CBigNum bnResult;
	bnResult.SetCompact(nBase);
	while (nTime > 0 && bnResult < bnProofOfWorkLimit)
	{
		// Maximum 400% adjustment...
		bnResult *= 4;
		// ... in best-case exactly 4-times-normal target time
		nTime -= nTargetTimespan * 4;
	}

	if (bnResult > bnProofOfWorkLimit)
		bnResult = bnProofOfWorkLimit;

	return bnResult.GetCompact();
}

unsigned int CMainNetDiff::GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	// Genesis block
	if (pindexLast == NULL)
		return nProofOfWorkLimit;

	// Check if we should retarget diff.
	if (!ShouldApplyRetarget(pindexLast, pblock))
	{
		return pindexLast->nBits;
	}

	// Limit adjustment step
	int64 nActualTimespan = GetActualTimespan(GetFirstBlock(pindexLast), pindexLast);

	// Retarget
	CBigNum bnNew, bnOld;
	bnOld.SetCompact(pindexLast->nBits);
	bnNew.SetCompact(pindexLast->nBits);
	bnNew *= nActualTimespan;
	bnNew /= nTargetTimespan;

	if (bnNew > bnProofOfWorkLimit)
		bnNew = bnProofOfWorkLimit;

	/// debug print
	printf("GetNextWorkRequired RETARGET\n");
	printf("nTargetTimespan = %"PRI64d"    nActualTimespan = %"PRI64d"\n", nTargetTimespan, nActualTimespan);
	printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
	printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());

	return bnNew.GetCompact();
}

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int CTestNetDiff::ComputeMinWork(unsigned int nBase, int64 nTime)
{
	// Testnet has min-difficulty blocks
	// after nTargetSpacing*2 time between blocks:
	if (nTime > nTargetSpacing*2)
		return bnProofOfWorkLimit.GetCompact();

	return CMainNetDiff::ComputeMinWork(nBase, nTime);
}

unsigned int CTestNetDiff::GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	if (!ShouldApplyRetarget(pindexLast, pblock))
	{
		return GetTestNetNextTarget(pindexLast, pblock);
	}

	return CMainNetDiff::GetNextWorkRequired(pindexLast, pblock);
}

unsigned int CTestNetDiff::GetTestNetNextTarget(const CBlockIndex* pindexLast, const CBlock *pblock)
{
	// If the new block's timestamp is more than 2* 10 minutes
	// then allow mining of a min-difficulty block.
	if (pblock->nTime > pindexLast->nTime + nTargetSpacing * 2)
	{
		return nProofOfWorkLimit;
	}
	else
	{
		// Return the last non-special-min-difficulty-rules-block
		const CBlockIndex* pindex = pindexLast;
		while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
		{
			pindex = pindex->pprev;
		}
		return pindex->nBits;
	}
}
