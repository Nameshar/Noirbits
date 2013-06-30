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
        (	  10500, uint256("0x029234ba5fdaf42ce20ffa4a259313f676ce21eab0a994cd3c1a177a9ed320e6"))
        (	  13900, uint256("0xc5dbae08282bd2f1e3ccf0128665dac79eda6c50cdf2c09af5b643930955df8b"))
        (	  14420, uint256("0x9a0999bd0c6156c419f77db9bb101018ca63ca5cc0fdca197eefef90011c565b"))
        (	  15245, uint256("0x477b0e40c32c5662108f3c372373abddc2aaac821299fdde2bed37b9a47d0a91"))
        (	  15800, uint256("0x8ebaee1cd401a59765eebed59a0b97cfd4303952e15c18463a140c5a1a6797dc"))
        (	  16500, uint256("0x839b373ce4f14056e4092a8301d88ee1f77881ba7600eca6d4f9c3139d9317a7"))
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
