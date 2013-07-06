/*
 * diff.cpp
 *
 *  Created on: Jul 3, 2013
 *      Author: fatbeard
 */

#include "diff.h"

const struct SRetargetParams* CMainNetDiff::sRules = new SRetargetParams(3600, 120);
const struct SRetargetParams* COldNetDiff::sRules = new SRetargetParams(7200, 120);

const CBigNum CDiffProvider::bnProofOfWorkLimit = 0;

CDiff* CDiffProvider::pnewDiff = NULL;
CDiff* CDiffProvider::poldDiff = NULL;
CDiff* CDiffProvider::ptestDiff = NULL;

double CDiff::GetDifficulty(const CBlockIndex* blockindex = NULL)
{
	// Floating point number that is a multiple of the minimum difficulty,
	// minimum difficulty = 1.0.
	if (blockindex == NULL)
	{
		if (pindexBest == NULL)
			return 1.0;
		else
			blockindex = pindexBest;
	}

	int nShift = (blockindex->nBits >> 24) & 0xff;

	double dDiff = (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

	while (nShift < 29)
	{
		dDiff *= 256.0;
		nShift++;
	}
	while (nShift > 29)
	{
		dDiff /= 256.0;
		nShift--;
	}

	return dDiff;
}

json_spirit::Value CDiff::GetNetworkHashPS(int lookup)
{
	if (pindexBest == NULL)
	        return 0;

	// If lookup is -1, then use blocks since last difficulty change.
	if (lookup <= 0)
	{
		int nInterval = CDiffProvider::GetDiff(nBestHeight)->GetRules()->nInterval;

		lookup = pindexBest->nHeight % nInterval + 1;
	}

	// If lookup is larger than chain, then set it to chain length.
	if (lookup > pindexBest->nHeight)
		lookup = pindexBest->nHeight;

	CBlockIndex* pindexPrev = pindexBest;
	for (int i = 0; i < lookup; i++)
		pindexPrev = pindexPrev->pprev;

	double timeDiff = pindexBest->GetBlockTime() - pindexPrev->GetBlockTime();
	double timePerBlock = timeDiff / lookup;

	return (boost::int64_t)(((double)GetDifficulty() * pow(2.0, 32)) / timePerBlock);
}

bool COldNetDiff::ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	bool bShouldRetarget = false;

	// We have reached retarget height
	bShouldRetarget |= (pindexLast->nHeight + 1) % sRules->nInterval == 0;

	return bShouldRetarget;
}

int64 COldNetDiff::GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast)
{
	int64 nActualTimespan = 0;
	int64 nActualTimespanMax = 0;
	int64 nActualTimespanMin = 0;

	if (pindexLast->nHeight > COINFIX1_BLOCK)
	{
		// obtain average actual timespan
		nActualTimespan = (pindexLast->GetBlockTime() - pindexFirst->GetBlockTime()) / nRetargetHistoryFact;
	}
	else
	{
		nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
	}

	nActualTimespanMin = sRules->nTargetTimespan / 4;
	nActualTimespanMax = sRules->nTargetTimespan * 4;

	printf("  nActualTimespan = %"PRI64d"  before bounds\n", nActualTimespan);

	if (nActualTimespan > nActualTimespanMax) nActualTimespan = nActualTimespanMax;
	if (nActualTimespan < nActualTimespanMin) nActualTimespan = nActualTimespanMin;

	return nActualTimespan;
}

const CBlockIndex* COldNetDiff::GetFirstBlock(const CBlockIndex* pindexLast)
{
	const CBlockIndex* pindexFirst = pindexLast;
	for (int i = 0; pindexFirst && i < GetBlocksToGoBack(pindexLast); i++)
	{
		pindexFirst = pindexFirst->pprev;
	}

	assert(pindexFirst);

	return pindexFirst;
}

int COldNetDiff::GetBlocksToGoBack(const CBlockIndex* pindexLast)
{
	// Fixes an issue where a 51% attack can change difficulty at will.
	// Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
	int nBlocksToGoBack = sRules->nInterval - 1;

	if ((pindexLast->nHeight + 1) != sRules->nInterval)
	{
		nBlocksToGoBack = sRules->nInterval;
	}

	if (pindexLast->nHeight > COINFIX1_BLOCK)
	{
		nBlocksToGoBack = nRetargetHistoryFact * sRules->nInterval;
	}

	return nBlocksToGoBack;
}

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int COldNetDiff::ComputeMinWork(unsigned int nBase, int64 nTime)
{
	CBigNum bnResult;
	bnResult.SetCompact(nBase);

	while (nTime > 0 && bnResult < bnProofOfWorkLimit)
	{
		bnResult = bnResult / 4;
		nTime -= sRules->nTargetTimespan * 4;
	}

	if (bnResult > bnProofOfWorkLimit)
		bnResult = bnProofOfWorkLimit;

	return bnResult.GetCompact();
}

unsigned int COldNetDiff::GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	// Genesis block
	if (pindexLast == NULL)
		return nProofOfWorkLimit;

	bool bretarget = ShouldApplyRetarget(pindexLast, pblock);

	// Check if we should retarget diff.
	if (!bretarget)
	{
		return pindexLast->nBits;
	}

	// Limit adjustment step
	int64 nActualTimespan = GetActualTimespan(GetFirstBlock(pindexLast), pindexLast);

	// Retarget
	CBigNum bnNew;
	bnNew.SetCompact(pindexLast->nBits);
	bnNew *= nActualTimespan / sRules->nTargetTimespan;

	if (bnNew > bnProofOfWorkLimit)
		bnNew = bnProofOfWorkLimit;

	/// debug print
	printf("COldDiff -- GetNextWorkRequired RETARGET\n");
	printf("nTargetTimespan = %"PRI64d"    nActualTimespan = %"PRI64d"\n", sRules->nTargetTimespan, nActualTimespan);
	printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
	printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());

	return bnNew.GetCompact();
}

const SRetargetParams* 	COldNetDiff::GetRules()
{
	return sRules;
}








bool CMainNetDiff::ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	bool bShouldRetarget = false;

	// We have exceeded max. time for current difficulty, change (hard limit)
	bShouldRetarget |= (pindexLast->nTime + nMaxTimeInterval) < pblock->nTime;
	// We have reached retarget height
	bShouldRetarget |= (pindexLast->nHeight + 1) % sRules->nInterval == 0;

	return bShouldRetarget;
}

int64 CMainNetDiff::GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast)
{
	int64 nActualTimespan = 0;
	int64 nActualTimespanMax = 0;
	int64 nActualTimespanMin = 0;

	nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();

	// FeatherCoin's cap system.
	// For diff. increase
	nActualTimespanMin = (nActualTimespan * 55) / 99;
	// For diff. decrease
	nActualTimespanMax = (nActualTimespan * 99) / 55;

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
	int nBlocksToGoBack = sRules->nInterval - 1;

	if ((pindexLast->nHeight + 1) != sRules->nInterval)
	{
		nBlocksToGoBack = sRules->nInterval;
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
		bnResult = bnResult * 99 / 55;
		nTime -= sRules->nTargetTimespan * 4;
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
	CBigNum bnNew;
	bnNew.SetCompact(pindexLast->nBits);
	bnNew *= nActualTimespan / sRules->nTargetTimespan;

	if (bnNew > bnProofOfWorkLimit)
		bnNew = bnProofOfWorkLimit;

	/// debug print
	printf("CMainNetDiff -- GetNextWorkRequired RETARGET\n");
	printf("nTargetTimespan = %"PRI64d"    nActualTimespan = %"PRI64d"\n", sRules->nTargetTimespan, nActualTimespan);
	printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
	printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());

	return bnNew.GetCompact();
}

const SRetargetParams* 	CMainNetDiff::GetRules()
{
	return sRules;
}



//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int CTestNetDiff::ComputeMinWork(unsigned int nBase, int64 nTime)
{
	// Testnet has min-difficulty blocks
	// after nTargetSpacing*2 time between blocks:
	if (nTime > pparentRules->GetRules()->nTargetSpacing * 2)
		return bnProofOfWorkLimit.GetCompact();

	return pparentRules->ComputeMinWork(nBase, nTime);
}

unsigned int CTestNetDiff::GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	if (!pparentRules->ShouldApplyRetarget(pindexLast, pblock))
	{
		return GetTestNetNextTarget(pindexLast, pblock);
	}

	return pparentRules->GetNextWorkRequired(pindexLast, pblock);
}

unsigned int CTestNetDiff::GetTestNetNextTarget(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	const SRetargetParams* rules = pparentRules->GetRules();
	// If the new block's timestamp is more than 2* 10 minutes
	// then allow mining of a min-difficulty block.
	if (pblock->nTime > pindexLast->nTime + rules->nTargetSpacing * 2)
	{
		return nProofOfWorkLimit;
	}
	else
	{
		// Return the last non-special-min-difficulty-rules-block
		const CBlockIndex* pindex = pindexLast;
		while (pindex->pprev && pindex->nHeight % rules->nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
		{
			pindex = pindex->pprev;
		}
		return pindex->nBits;
	}
}

const SRetargetParams* 	CTestNetDiff::GetRules()
{
	return pparentRules->GetRules();
}

bool CTestNetDiff::ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock)
{
	return pparentRules->ShouldApplyRetarget(pindexLast, pblock);
}
