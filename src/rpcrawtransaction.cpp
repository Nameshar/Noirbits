// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include <utility>
#include <boost/assign/list_of.hpp>

#include "base58.h"
#include "bitcoinrpc.h"
#include "db.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "wallet.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

// These are all in bitcoinrpc.cpp:
extern Object JSONRPCError(int code, const string& message);
extern int64 AmountFromValue(const Value& value);
extern Value ValueFromAmount(int64 amount);
extern std::string HelpRequiringPassphrase();
extern void EnsureWalletIsUnlocked();

void
ScriptPubKeyToJSON(const CScript& scriptPubKey, Object& out)
{
    txnouttype type;
    vector<CTxDestination> addresses;
    int nRequired;

    out.push_back(Pair("asm", scriptPubKey.ToString()));
    out.push_back(Pair("hex", HexStr(scriptPubKey.begin(), scriptPubKey.end())));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired))
    {
        out.push_back(Pair("type", GetTxnOutputType(TX_NONSTANDARD)));
        return;
    }

    out.push_back(Pair("reqSigs", nRequired));
    out.push_back(Pair("type", GetTxnOutputType(type)));

    Array a;
    BOOST_FOREACH(const CTxDestination& addr, addresses)
        a.push_back(CBitcoinAddress(addr).ToString());
    out.push_back(Pair("addresses", a));
}

void
TxToJSON(const CTransaction& tx, const uint256 hashBlock, Object& entry)
{
    entry.push_back(Pair("txid", tx.GetHash().GetHex()));
    entry.push_back(Pair("version", tx.nVersion));
    entry.push_back(Pair("locktime", (boost::int64_t)tx.nLockTime));
    Array vin;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        Object in;
        if (tx.IsCoinBase())
            in.push_back(Pair("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
        else
        {
            in.push_back(Pair("txid", txin.prevout.hash.GetHex()));
            in.push_back(Pair("vout", (boost::int64_t)txin.prevout.n));
            Object o;
            o.push_back(Pair("asm", txin.scriptSig.ToString()));
            o.push_back(Pair("hex", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
            in.push_back(Pair("scriptSig", o));
        }
        in.push_back(Pair("sequence", (boost::int64_t)txin.nSequence));
        vin.push_back(in);
    }
    entry.push_back(Pair("vin", vin));
    Array vout;
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];
        Object out;
        out.push_back(Pair("value", ValueFromAmount(txout.nValue)));
        out.push_back(Pair("n", (boost::int64_t)i));
        Object o;
        ScriptPubKeyToJSON(txout.scriptPubKey, o);
        out.push_back(Pair("scriptPubKey", o));
        vout.push_back(out);
    }
    entry.push_back(Pair("vout", vout));

    if (hashBlock != 0)
    {
        entry.push_back(Pair("blockhash", hashBlock.GetHex()));
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second)
        {
            CBlockIndex* pindex = (*mi).second;
            if (pindex->IsInMainChain())
            {
                entry.push_back(Pair("confirmations", 1 + nBestHeight - pindex->nHeight));
                entry.push_back(Pair("time", (boost::int64_t)pindex->nTime));
            }
            else
                entry.push_back(Pair("confirmations", 0));
        }
    }
}

Value getrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getrawtransaction <txid> [verbose=0]\n"
            "If verbose=0, returns a string that is\n"
            "serialized, hex-encoded data for <txid>.\n"
            "If verbose is non-zero, returns an Object\n"
            "with information about <txid>.");

    uint256 hash;
    hash.SetHex(params[0].get_str());

    bool fVerbose = false;
    if (params.size() > 1)
        fVerbose = (params[1].get_int() != 0);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock))
        throw JSONRPCError(-5, "No information available about transaction");

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;
    string strHex = HexStr(ssTx.begin(), ssTx.end());

    if (!fVerbose)
        return strHex;

    Object result;
    result.push_back(Pair("hex", strHex));
    TxToJSON(tx, hashBlock, result);
    return result;
}

Value listunspent(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw runtime_error(
            "listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]\n"
            "Returns array of unspent transaction outputs\n"
            "with between minconf and maxconf (inclusive) confirmations.\n"
            "Optionally filtered to only include txouts paid to specified addresses.\n"
            "Results are an array of Objects, each of which has:\n"
            "{txid, vout, scriptPubKey, amount, confirmations}");

    RPCTypeCheck(params, list_of(int_type)(int_type)(array_type));

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    int nMaxDepth = 9999999;
    if (params.size() > 1)
        nMaxDepth = params[1].get_int();

    set<CBitcoinAddress> setAddress;
    if (params.size() > 2)
    {
        Array inputs = params[2].get_array();
        BOOST_FOREACH(Value& input, inputs)
        {
            CBitcoinAddress address(input.get_str());
            if (!address.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Bitcoin address: ")+input.get_str());
            if (setAddress.count(address))
                throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+input.get_str());
           setAddress.insert(address);
        }
    }

    Array results;
    vector<COutput> vecOutputs;
    pwalletMain->AvailableCoins(vecOutputs, false);
    BOOST_FOREACH(const COutput& out, vecOutputs)
    {
        if (out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
            continue;

        if (setAddress.size())
        {
            CTxDestination address;
            if (!ExtractDestination(out.tx->vout[out.i].scriptPubKey, address))
                continue;

            if (!setAddress.count(address))
                continue;
        }

        int64 nValue = out.tx->vout[out.i].nValue;
        const CScript& pk = out.tx->vout[out.i].scriptPubKey;
        Object entry;
        entry.push_back(Pair("txid", out.tx->GetHash().GetHex()));
        entry.push_back(Pair("vout", out.i));
        CTxDestination address;
        if (ExtractDestination(out.tx->vout[out.i].scriptPubKey, address))
        {
            entry.push_back(Pair("address", CBitcoinAddress(address).ToString()));
            if (pwalletMain->mapAddressBook.count(address))
                entry.push_back(Pair("account", pwalletMain->mapAddressBook[address]));
        }
        entry.push_back(Pair("scriptPubKey", HexStr(pk.begin(), pk.end())));
        if (pk.IsPayToScriptHash())
        {
            CTxDestination address;
            if (ExtractDestination(pk, address))
            {
                const CScriptID& hash = boost::get<const CScriptID&>(address);
                CScript redeemScript;
                if (pwalletMain->GetCScript(hash, redeemScript))
                    entry.push_back(Pair("redeemScript", HexStr(redeemScript.begin(), redeemScript.end())));
            }
        }
        entry.push_back(Pair("amount",ValueFromAmount(nValue)));
        entry.push_back(Pair("confirmations",out.nDepth));
        results.push_back(entry);
    }

    return results;
}

Value createrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "createrawtransaction [{\"txid\":txid,\"vout\":n},...] {address:amount,...}\n"
            "Create a transaction spending given inputs\n"
            "(array of objects containing transaction id and output number),\n"
            "sending to given address(es).\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.");

    RPCTypeCheck(params, list_of(array_type)(obj_type));

    Array inputs = params[0].get_array();
    Object sendTo = params[1].get_obj();

    CTransaction rawTx;

    BOOST_FOREACH(Value& input, inputs)
    {
        const Object& o = input.get_obj();

        const Value& txid_v = find_value(o, "txid");
        if (txid_v.type() != str_type)
            throw JSONRPCError(-8, "Invalid parameter, missing txid key");
        string txid = txid_v.get_str();
        if (!IsHex(txid))
            throw JSONRPCError(-8, "Invalid parameter, expected hex txid");

        const Value& vout_v = find_value(o, "vout");
        if (vout_v.type() != int_type)
            throw JSONRPCError(-8, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(-8, "Invalid parameter, vout must be positive");

        CTxIn in(COutPoint(uint256(txid), nOutput));
        rawTx.vin.push_back(in);
    }

    set<CBitcoinAddress> setAddress;
    BOOST_FOREACH(const Pair& s, sendTo)
    {
        CBitcoinAddress address(s.name_);
        if (!address.IsValid())
            throw JSONRPCError(-5, string("Invalid Bitcoin address:")+s.name_);

        if (setAddress.count(address))
            throw JSONRPCError(-8, string("Invalid parameter, duplicated address: ")+s.name_);
        setAddress.insert(address);

        CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
        int64 nAmount = AmountFromValue(s.value_);

        CTxOut out(nAmount, scriptPubKey);
        rawTx.vout.push_back(out);
    }

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << rawTx;
    return HexStr(ss.begin(), ss.end());
}

Value decoderawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "decoderawtransaction <hex string>\n"
            "Return a JSON object representing the serialized, hex-encoded transaction.");

    RPCTypeCheck(params, list_of(str_type));

    vector<unsigned char> txData(ParseHex(params[0].get_str()));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;
    try {
        ssData >> tx;
    }
    catch (std::exception &e) {
        throw JSONRPCError(-22, "TX decode failed");
    }

    Object result;
    TxToJSON(tx, 0, result);

    return result;
}

pair<bool, CTransaction> SignTransaction(vector<CTransaction> txVariants, Array prevTxs, Array keys, int nHashType = SIGHASH_ALL)
{
	// mergedTx will end up with all the signatures; it
	    // starts as a clone of the rawtx:
	    CTransaction mergedTx(txVariants[0]);
	    bool fComplete = true;

	    // Fetch previous transactions (inputs):
	    map<COutPoint, CScript> mapPrevOut;
	    {
	        MapPrevTx mapPrevTx;
	        CTxDB txdb("r");
	        map<uint256, CTxIndex> unused;
	        bool fInvalid;
	        mergedTx.FetchInputs(txdb, unused, false, false, mapPrevTx, fInvalid);

	        // Copy results into mapPrevOut:
	        BOOST_FOREACH(const CTxIn& txin, mergedTx.vin)
	        {
	            const uint256& prevHash = txin.prevout.hash;
	            if (mapPrevTx.count(prevHash))
	                mapPrevOut[txin.prevout] = mapPrevTx[prevHash].second.vout[txin.prevout.n].scriptPubKey;
	        }
	    }

	    // Add previous txouts given in the RPC call:
	    if (prevTxs.size() > 0)
	    {
	        BOOST_FOREACH(Value& p, prevTxs)
	        {
	            if (p.type() != obj_type)
	                throw JSONRPCError(-22, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");

	            Object prevOut = p.get_obj();

	            RPCTypeCheck(prevOut, map_list_of("txid", str_type)("vout", int_type)("scriptPubKey", str_type));

	            string txidHex = find_value(prevOut, "txid").get_str();
	            if (!IsHex(txidHex))
	                throw JSONRPCError(-22, "txid must be hexadecimal");
	            uint256 txid;
	            txid.SetHex(txidHex);

	            int nOut = find_value(prevOut, "vout").get_int();
	            if (nOut < 0)
	                throw JSONRPCError(-22, "vout must be positive");

	            string pkHex = find_value(prevOut, "scriptPubKey").get_str();
	            if (!IsHex(pkHex))
	                throw JSONRPCError(-22, "scriptPubKey must be hexadecimal");
	            vector<unsigned char> pkData(ParseHex(pkHex));
	            CScript scriptPubKey(pkData.begin(), pkData.end());

	            COutPoint outpoint(txid, nOut);
	            if (mapPrevOut.count(outpoint))
	            {
	                // Complain if scriptPubKey doesn't match
	                if (mapPrevOut[outpoint] != scriptPubKey)
	                {
	                    string err("Previous output scriptPubKey mismatch:\n");
	                    err = err + mapPrevOut[outpoint].ToString() + "\nvs:\n"+
	                        scriptPubKey.ToString();
	                    throw JSONRPCError(-22, err);
	                }
	            }
	            else
	                mapPrevOut[outpoint] = scriptPubKey;
	        }
	    }

	    bool fGivenKeys = false;
	    CBasicKeyStore tempKeystore;
	    if (keys.size() > 0)
	    {
	        fGivenKeys = true;
	        BOOST_FOREACH(Value k, keys)
	        {
	            CBitcoinSecret vchSecret;
	            bool fGood = vchSecret.SetString(k.get_str());
	            if (!fGood)
	                throw JSONRPCError(-5,"Invalid private key");
	            CKey key;
	            bool fCompressed;
	            CSecret secret = vchSecret.GetSecret(fCompressed);
	            key.SetSecret(secret, fCompressed);
	            tempKeystore.AddKey(key);
	        }
	    }
	    const CKeyStore& keystore = (fGivenKeys ? tempKeystore : *pwalletMain);

	    // Sign what we can:
	    for (unsigned int i = 0; i < mergedTx.vin.size(); i++)
	    {
	        CTxIn& txin = mergedTx.vin[i];
	        if (mapPrevOut.count(txin.prevout) == 0)
	        {
	            fComplete = false;
	            continue;
	        }
	        const CScript& prevPubKey = mapPrevOut[txin.prevout];

	        txin.scriptSig.clear();
	        SignSignature(keystore, prevPubKey, mergedTx, i, nHashType);

	        // ... and merge in other signatures:
	        BOOST_FOREACH(const CTransaction& txv, txVariants)
	        {
	            txin.scriptSig = CombineSignatures(prevPubKey, mergedTx, i, txin.scriptSig, txv.vin[i].scriptSig);
	        }
	        if (!VerifyScript(txin.scriptSig, prevPubKey, mergedTx, i, true, 0))
	            fComplete = false;
	    }

	    return make_pair(fComplete, mergedTx);
}

Value signrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
            "signrawtransaction <hex string> [{\"txid\":txid,\"vout\":n,\"scriptPubKey\":hex},...] [<privatekey1>,...] [sighashtype=\"ALL\"]\n"
            "Sign inputs for raw transaction (serialized, hex-encoded).\n"
            "Second optional argument is an array of previous transaction outputs that\n"
            "this transaction depends on but may not yet be in the blockchain.\n"
            "Third optional argument is an array of base58-encoded private\n"
            "keys that, if given, will be the only keys used to sign the transaction.\n"
            "Fourth option is a string that is one of six values; ALL, NONE, SINGLE or\n"
            "ALL|ANYONECANPAY, NONE|ANYONECANPAY, SINGLE|ANYONECANPAY.\n"
            "Returns json object with keys:\n"
            "  hex : raw transaction with signature(s) (hex-encoded string)\n"
            "  complete : 1 if transaction has a complete set of signature (0 if not)"
            + HelpRequiringPassphrase());

    if (params.size() < 3)
        EnsureWalletIsUnlocked();

    RPCTypeCheck(params, list_of(str_type)(array_type)(array_type));

    vector<unsigned char> txData(ParseHex(params[0].get_str()));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    vector<CTransaction> txVariants;
    while (!ssData.empty())
    {
        try {
            CTransaction tx;
            ssData >> tx;
            txVariants.push_back(tx);
        }
        catch (std::exception &e) {
            throw JSONRPCError(-22, "TX decode failed");
        }

        if (fDebug)
        	printf("ssData.eof() : %d", ssData.eof());
    }

    if (txVariants.empty())
        throw JSONRPCError(-22, "Missing transaction");

	Array prevTxs;
	if (params.size() > 1)
		prevTxs = params[1].get_array();
	Array keys;
	if (params.size() > 2)
		params[2].get_array();

	int nHashType = SIGHASH_ALL;

	if (params.size() > 3)
	{
		static map<string, int> mapSigHashValues =
			boost::assign::map_list_of
			(string("ALL"), int(SIGHASH_ALL))
			(string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY))
			(string("NONE"), int(SIGHASH_NONE))
			(string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY))
			(string("SINGLE"), int(SIGHASH_SINGLE))
			(string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY))
			;
		string strHashType = params[3].get_str();
		if (mapSigHashValues.count(strHashType))
			nHashType = mapSigHashValues[strHashType];
		else
			throw JSONRPCError(-8, "Invalid sighash param");
	}

    pair<bool, CTransaction> signResult = SignTransaction(txVariants, prevTxs, keys, nHashType);

    bool fComplete = signResult.first;
    Object result;
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << signResult.second;

    result.push_back(Pair("hex", HexStr(ssTx.begin(), ssTx.end())));
    result.push_back(Pair("complete", fComplete));

    return result;
}

void SendTransaction(CTransaction tx)
{
	uint256 hashTx = tx.GetHash();

	// See if the transaction is already in a block
	// or in the memory pool:
	CTransaction existingTx;
	uint256 hashBlock = 0;
	if (GetTransaction(hashTx, existingTx, hashBlock))
	{
		if (hashBlock != 0)
			throw JSONRPCError(-5, string("transaction already in block ")+hashBlock.GetHex());
		// Not in block, but already in the memory pool; will drop
		// through to re-relay it.
	}
	else
	{
		// push to local node
		CTxDB txdb("r");
		if (!tx.AcceptToMemoryPool(txdb))
			throw JSONRPCError(-22, "TX rejected");

		SyncWithWallets(tx, NULL, true);
	}

	RelayMessage(CInv(MSG_TX, hashTx), tx);
}

Value sendrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "sendrawtransaction <hex string>\n"
            "Submits raw transaction (serialized, hex-encoded) to local node and network.");

    RPCTypeCheck(params, list_of(str_type));

    // parse hex string from parameter
    vector<unsigned char> txData(ParseHex(params[0].get_str()));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;

    // deserialize binary data stream
    try {
        ssData >> tx;
    }
    catch (std::exception &e) {
        throw JSONRPCError(-22, "TX decode failed");
    }

    SendTransaction(tx);

    return tx.GetHash().GetHex();
}

CTxOut CreateTxOut(CBitcoinAddress caddr, int64 value)
{
	CScript scriptPubKey;
	scriptPubKey.SetDestination(caddr.Get());

	return CTxOut(value, scriptPubKey);
}

CBitcoinAddress FindTxSource(const CWalletTx& wtx)
{
	for (unsigned int i = 0; i < wtx.vin.size(); i++)
	{
		// Get input of refunded transaction
		const CTxIn& vin = wtx.vin[i];
		uint256 sourceHash = vin.prevout.hash;

		// Get transaction from which input was generated
		CTransaction tx;
		uint256 hashBlock = 0;
		if (!GetTransaction(sourceHash, tx, hashBlock))
			throw JSONRPCError(-5, "No information available about transaction");

		// Get matching output from transaction
		const CTxOut& vout = tx.vout[vin.prevout.n];

		txnouttype type;
		vector<CTxDestination> addresses;
		int nRequired;

		if (!ExtractDestinations(vout.scriptPubKey, type, addresses, nRequired))
		{
			throw JSONRPCError(-5, "Cannot refund non standard transaction");
		}

		return CBitcoinAddress(addresses[vin.prevout.n]);
	}

	throw JSONRPCError(-5, "Source address not found");
}

Value refundtransaction(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 1 || params.size() > 2)
	        throw runtime_error(
	            "refundtransaction <txid> [<returnAddress>]\n"
	            "Refund an in-wallet transaction <txid> using the coins that were sent.\n"
	            "Returns the unsigned raw transaction, or false in case of failure.");

	uint256 hash;
	hash.SetHex(params[0].get_str());

	Object entry;
	if (!pwalletMain->mapWallet.count(hash))
		throw JSONRPCError(-5, "Invalid or non-wallet transaction id");
	const CWalletTx& wtx = pwalletMain->mapWallet[hash];

	CTransaction wtxNew;
    wtxNew.vin.clear();
    wtxNew.vout.clear();

	int64 refundValue = 0;

	// Build inputs
	for (unsigned int i = 0; i < wtx.vout.size(); i++)
	{
		const CTxOut& vout = wtx.vout[i];
		if (IsMine(*pwalletMain, vout.scriptPubKey))
		{
			CTxIn vin(COutPoint(hash, i));
			wtxNew.vin.push_back(vin);

			if (fDebug)
				printf("Pushing vin : %s\n", vin.ToString().c_str());

			refundValue += vout.nValue;
		}
	}

	if (refundValue == 0) return false;

	// Find address from which the original transaction was sent if a return
	// address was not specified.
	if (params.size() == 1)
	{
		CTxOut out = CreateTxOut(FindTxSource(wtx), refundValue);

		if (fDebug)
			printf("Pushing vout : %s\n", out.ToString().c_str());

		wtxNew.vout.push_back(out);
	}
	else
	{
		CTxOut out = CreateTxOut(CBitcoinAddress(params[1].get_str()), refundValue);

		if (fDebug)
			printf("Pushing vout : %s\n", out.ToString().c_str());

		wtxNew.vout.push_back(out);
	}

	if (fDebug)
		printf("Refund transaction: %s\n", wtxNew.ToString().c_str());

	vector<CTransaction> txVariants;
	txVariants.push_back(wtxNew);

	Array prevTxs;
	Array keys;
	pair<bool, CTransaction> pair = SignTransaction(txVariants, prevTxs, keys, SIGHASH_ALL);

	if (pair.first == true)
	{
		CTransaction signedTx = pair.second;
		SendTransaction(signedTx);
		return signedTx.GetHash().GetHex();
	}

    return false;
}
