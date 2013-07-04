/*
 * diff.cpp
 *
 *  Created on: Jul 3, 2013
 *      Author: fatbeard
 */

#include "diff.h"

const struct SRetargetParams* CMainNetDiff::sNewRules = new SRetargetParams(3600, 120);
const struct SRetargetParams* CMainNetDiff::sOldRules = new SRetargetParams(7200, 120);

bool CMainNetDiff::ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock *pblock)
{
	bool bShouldRetarget = false;
	const SRetargetParams* params = sOldRules;

	if (ShouldApplyNewRetargetRules(pindexLast->nHeight))
	{
		params = sNewRules;
		// We have exceeded max. time for current difficulty, change (hard limit)
		bShouldRetarget |= (pindexLast->nTime + nMaxTimeInterval) < pblock->nTime;
	}

	// We have reached retarget height
	bShouldRetarget |= (pindexLast->nHeight + 1) % params->nInterval == 0;

	return bShouldRetarget;
}

int64 CMainNetDiff::GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast)
{
	int64 nActualTimespan = 0;
	int64 nActualTimespanMax = 0;
	int64 nActualTimespanMin = 0;

	bool useNewRules = ShouldApplyNewRetargetRules(pindexLast->nHeight);
	const SRetargetParams* params = (useNewRules) ? sNewRules : sOldRules;

	if (pindexLast->nHeight > COINFIX1_BLOCK && !useNewRules)
	{
		// obtain average actual timespan
		nActualTimespan = (pindexLast->GetBlockTime() - pindexFirst->GetBlockTime()) / nRetargetHistoryFact;
	}
	else
	{
		nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
	}

	if (useNewRules)
	{
		// FeatherCoin's cap system.
		// For diff. increase
		nActualTimespanMin = (nActualTimespan * 55) / 99;
		// For diff. decrease
		nActualTimespanMax = (nActualTimespan * 99) / 55;
	}
	else
	{
		nActualTimespanMin = params->nTargetTimespan / 4;
		nActualTimespanMax = params->nTargetTimespan * 4;
	}

	printf("  nActualTimespan = %"PRI64d"  before bounds\n", nActualTimespan);

	if (nActualTimespan > nActualTimespanMax) nActualTimespan = nActualTimespanMax;
	if (nActualTimespan < nActualTimespanMin) nActualTimespan = nActualTimespanMin;

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
	bool useNewRules = ShouldApplyNewRetargetRules(pindexLast->nHeight);
	const SRetargetParams* params = (useNewRules) ? sNewRules : sOldRules;
	int nBlocksToGoBack = params->nInterval - 1;

	if ((pindexLast->nHeight + 1) != params->nInterval)
	{
		nBlocksToGoBack = params->nInterval;
	}

	if (pindexLast->nHeight > COINFIX1_BLOCK && !useNewRules)
	{
		nBlocksToGoBack = nRetargetHistoryFact * params->nInterval;
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

	bool useNewRules = ShouldApplyNewRetargetRules(nBestHeight);
	const SRetargetParams* params = (useNewRules) ? sNewRules : sOldRules;

	while (nTime > 0 && bnResult < bnProofOfWorkLimit)
	{
		if (useNewRules)
			bnResult = bnResult * 99 / 55;
		else
			bnResult = bnResult / 4;

		nTime -= params->nTargetTimespan * 4;
	}

	if (bnResult > bnProofOfWorkLimit)
		bnResult = bnProofOfWorkLimit;

	return bnResult.GetCompact();
}

unsigned int CMainNetDiff::GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	bool useNewRules = ShouldApplyNewRetargetRules(pindexLast->nHeight);

	// Genesis block
	if (pindexLast == NULL)
		return nProofOfWorkLimit;

	// Check if we should retarget diff.
	if (!useNewRules)
	{
		return pindexLast->nBits;
	}

	const SRetargetParams* params = (useNewRules) ? sNewRules : sOldRules;

	// Limit adjustment step
	int64 nActualTimespan = GetActualTimespan(GetFirstBlock(pindexLast), pindexLast);

	// Retarget
	CBigNum bnNew;
	bnNew.SetCompact(pindexLast->nBits);
	bnNew *= nActualTimespan / params->nTargetTimespan;

	if (bnNew > bnProofOfWorkLimit)
		bnNew = bnProofOfWorkLimit;

	/// debug print
	printf("GetNextWorkRequired RETARGET\n");
	printf("nTargetTimespan = %"PRI64d"    nActualTimespan = %"PRI64d"\n", params->nTargetTimespan, nActualTimespan);
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
	bool useNewRules = ShouldApplyNewRetargetRules(nBestHeight);
	const SRetargetParams* params = (useNewRules) ? sNewRules : sOldRules;

	// Testnet has min-difficulty blocks
	// after nTargetSpacing*2 time between blocks:
	if (nTime > params->nTargetSpacing * 2)
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
	bool useNewRules = ShouldApplyNewRetargetRules(nBestHeight);
	const SRetargetParams* params = (useNewRules) ? sNewRules : sOldRules;

	// If the new block's timestamp is more than 2* 10 minutes
	// then allow mining of a min-difficulty block.
	if (pblock->nTime > pindexLast->nTime + params->nTargetSpacing * 2)
	{
		return nProofOfWorkLimit;
	}
	else
	{
		// Return the last non-special-min-difficulty-rules-block
		const CBlockIndex* pindex = pindexLast;
		while (pindex->pprev && pindex->nHeight % params->nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
		{
			pindex = pindex->pprev;
		}
		return pindex->nBits;
	}
}
