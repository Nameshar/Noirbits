// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2012 Litecoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of // Yo dawg, this is the secret. Checkpoint 0 hash == Genesis block hash.
        (         0, uint256("0xd113acfb96e64a16dba0f54ddf5a08ef0dd2c955d8650e2f3ad5d9873995a25f"))
        (	   4748, uint256("0xa66370541b06db6caa881db9dfbf3727b398b3da58694eb72d7cd66463893a00"))
        (	   6100, uint256("0x8c8f52584613e95d27c3850f6ac93f63a3530e4fd5ae13f19dd82cd8ac575afe"))
        (	   7379, uint256("0xeaec0db11ef39065c0aa915258463e604d30b712f44136d3c5594ad7926117a5"))
        (	   7800, uint256("0xb75df3e8b2d7b7065aaccd53810489542ab0a29a3ab2ccec607d2d8768146e11"))
        (	   9100, uint256("0x4ae92fdbd868972bd9592507585bfc0ca6480b6cbbd78c74876e357e9e63ea40"))
        (	   9700, uint256("0xf28176bcfa835c6fac1bbf1923bf5b091aca96782dd13a3d14b7e8c68a102494"))
        ;

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (fTestNet) return true; // Testnet has no checkpoints

        MapCheckpoints::const_iterator i = mapCheckpoints.find(nHeight);
        if (i == mapCheckpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        if (fTestNet) return 0;
        return mapCheckpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (fTestNet) return NULL;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, mapCheckpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
