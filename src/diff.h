/*
 * diff.h
 *
 *  Created on: Jul 3, 2013
 *      Author: fatbeard
 */

#ifndef DIFF_H_
#define DIFF_H_

#include "init.h"
#include "main.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

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

	static double GetDifficultyFromTargetBits(unsigned int nTargetBits)
	{
		if (fDebug)
			printf("Getting difficulty from targetsBits : %u\n", nTargetBits);

		int nShift = (nTargetBits >> 24) & 0xff;
		double dDiff = (double)0x0000ffff / (double)(nTargetBits & 0x00ffffff);

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

	CDiff(CBigNum bnProofOfWorkLimit)
	{
		this->bnProofOfWorkLimit = bnProofOfWorkLimit;
		this->nProofOfWorkLimit = bnProofOfWorkLimit.GetCompact();
	}

	virtual ~CDiff() { }

	virtual unsigned int 			ComputeMinWork(unsigned int nBase, int64 nTime) = 0;
	virtual double					GetDifficulty(const CBlockIndex* blockindex);
	virtual unsigned int 			GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock) = 0;
	virtual json_spirit::Value		GetNetworkHashPS(int lookup);
	virtual const SRetargetParams* 	GetRules() = 0;
	virtual bool					ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock) = 0;
};

class COldNetDiff : public CDiff
{
private:
	// Thanks: Balthazar for suggesting the following fix
	// https://bitcointalk.org/index.php?topic=182430.msg1904506#msg1904506
	static const int 				nRetargetHistoryFact = 4;

	// Methods
	int64				GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast);
	int 				GetBlocksToGoBack(const CBlockIndex* pindexLast);
	const CBlockIndex*	GetFirstBlock(const CBlockIndex* pindexLast);

public:
	static const SRetargetParams* 	sRules;

	COldNetDiff(CBigNum bnProofOfWorkLimit) : CDiff(bnProofOfWorkLimit)
	{ }

	virtual unsigned int 			ComputeMinWork(unsigned int nBase, int64 nTime);
	virtual unsigned int 			GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock);
	virtual const SRetargetParams* 	GetRules();
	virtual bool					ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock);
};

class CMainNetDiff : public CDiff
{
private:
	// Max. span between two retargets.
	static const unsigned int 	nMaxTimeInterval = 14400;

	// Methods
	int64				GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast);
	int 				GetBlocksToGoBack(const CBlockIndex* pindexLast);
	const CBlockIndex*	GetFirstBlock(const CBlockIndex* pindexLast);

public:
	static const SRetargetParams* sRules;

	CMainNetDiff(CBigNum bnProofOfWorkLimit) : CDiff(bnProofOfWorkLimit)
	{ }

	virtual unsigned int 			ComputeMinWork(unsigned int nBase, int64 nTime);
	virtual unsigned int 			GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock);
	virtual const SRetargetParams* 	GetRules();
	virtual bool					ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock);

};

class CDynamicDiff : public CMainNetDiff
{

private:
	int64				GetActualTimespan(const CBlockIndex* pindexFirst, const CBlockIndex* pindexLast);

public:
	CDynamicDiff(CBigNum bnProofOfWorkLimit) : CMainNetDiff(bnProofOfWorkLimit)
	{ }

	virtual bool					ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock);

};

class CTestNetDiff : public CDiff
{
private:
	CDiff* 			pparentRules;
	// Methods
	unsigned int 	GetTestNetNextTarget(const CBlockIndex* pindexLast, const CBlock *pblock);

public:

	CTestNetDiff(CBigNum bnProofOfWorkLimit, CDiff* parentDiff) : CDiff(bnProofOfWorkLimit)
	{
		this->pparentRules = parentDiff;
	}

	virtual unsigned int 			ComputeMinWork(unsigned int nBase, int64 nTime);
	virtual unsigned int 			GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlock* pblock);
	virtual const SRetargetParams* 	GetRules();
	virtual bool					ShouldApplyRetarget(const CBlockIndex* pindexLast, const CBlock* pblock);
};

/**
 * Provides an instance of CDiff to perform difficulty retargets depending on height.
 */
class CDiffProvider
{

public:
	static const CBigNum bnProofOfWorkLimit;

	static CDiff* pnewDiff;
	static CDiff* pdynDiff;
	static CDiff* poldDiff;
	static CDiff* ptestDiff;

	static CDiff* GetDiff(int nHeight)
	{
		if (fDebug)
			printf("Diff instance requested for height %d : ", nHeight);

		if (fTestNet)
		{
			if (fDebug)
				printf("using CTestNetDiff instance\n");
			return GetTestNetDiff(GetDynamicDiff());
		}
		else if (nHeight + 1 >= nMinHeightForDynamicRules && false)
		{
			if (fDebug)
				printf("using CMainNetDiff instance\n");
			return GetDynamicDiff();
		}
        else if ((nHeight + 1 >= nMinHeightForNewRules && nHeight + 1 < nFixHeight)
                  || nHeight + 1 >= nNewDiffHeight)
		{
			if (fDebug)
				printf("using CMainNetDiff instance\n");
			return GetNewDiff();
		}
		else
		{
			if (fDebug)
				printf("using COldNetDiff instance\n");
			return GetOldDiff();
		}
	}

private:
	// Height at which to apply new rules.
	static const int nMinHeightForNewRules = 25020;
	static const int nMinHeightForDynamicRules = 33333;
	static const int nFixHeight = 32128;
	static const int nNewDiffHeight = 36000;
	
	inline static CDiff* GetOldDiff()
	{
		if (poldDiff == NULL) poldDiff = new COldNetDiff(bnProofOfWorkLimit);

		return poldDiff;
	}

	inline static CDiff* GetNewDiff()
	{
		if (pnewDiff == NULL) pnewDiff = new CMainNetDiff(bnProofOfWorkLimit);

		return pnewDiff;
	}

	inline static CDiff* GetDynamicDiff()
	{
		if (pdynDiff == NULL) {
			pdynDiff = new CDynamicDiff(bnProofOfWorkLimit);
		}

		return pdynDiff;
	}

	inline static CDiff* GetTestNetDiff(CDiff* pparentDiff)
	{
		if (ptestDiff == NULL) ptestDiff = new CTestNetDiff(bnProofOfWorkLimit, pparentDiff);

		return ptestDiff;
	}
};

#endif /* DIFF_H_ */
