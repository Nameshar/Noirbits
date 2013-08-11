RPC Calls documentation
-----------------------

Following is the list of available RPC calls for Noirbitsd

* `addmultisigaddress <nrequired> <'["key","key"]'> [account]`
* `backupwallet <destination>`
* `createrawtransaction [{"txid":txid,"vout":n},...] {address:amount,...}`
* `decoderawtransaction <hex string>`
* `dumpprivkey <Noirbits address>`
* `getaccount <Noirbits address>`
* `getaccountaddress <account>`
* `getaddressesbyaccount <account>`
* `getbalance [account] [minconf=1]`
* `getblock <hash>`
* `getblockcount`
* `getblockhash <index>`
* `getblocktemplate [params]`
* `getconnectioncount`
* `getdifffrombits targetbits` (*useless, will be removed*)
* `getdifficulty`
* `getgenerate`
* `gethashespersec`
* `getinfo`
* `getmininginfo`
* `getnetworkhashps [blocks]`
* `getnewaddress [account]`
* `getpeerinfo`
* `getrawmempool`
* `getrawtransaction <txid> [verbose=0]`
* `getreceivedbyaccount <account> [minconf=1]`
* `getreceivedbyaddress <Noirbits address> [minconf=1]`
* `gettransaction <txid>`
* `getwork [data]`
* `getworkex [data, coinbase]`
* `help [command]`
* `importprivkey <Noirbits private key> [label]`
* `keypoolrefill`
* `listaccounts [minconf=1]`
* `listreceivedbyaccount [minconf=1] [includeempty=false]`
* `listreceivedbyaddress [minconf=1] [includeempty=false]`
* `listsinceblock [blockhash] [target-confirmations]`
* `listtransactions [account] [count=10] [from=0]`
* `istunspent [minconf=1] [maxconf=9999999] ["address",...]`
* `move <fromaccount> <toaccount> <amount> [minconf=1] [comment]`
* `refundtransaction <txid> [<returnAddress>]`
* `sendfrom <fromaccount> <to Noirbits address> <amount> [minconf=1] [comment] [comment-to]`
* `sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]`
* `sendrawtransaction <hex string>`
* `sendtoaddress <Noirbits address> <amount> [comment] [comment-to]`
* `setaccount <Noirbits address> <account>`
* `setgenerate <generate> [genproclimit]`
* `setmininput <amount>`
* `settxfee <amount>`
* `signmessage <Noirbits address> <message>`
* `signrawtransaction <hex string> [{"txid":txid,"vout":n,"scriptPubKey":hex},...] [<privatekey1>,...] [sighashtype="ALL"]`
* `stop`
* `validateaddress <Noirbits address>`
* `verifymessage <Noirbits address> <signature> <message>`
* `walletlock`
* `walletpassphrase <passphrase> <timeout>`
* `walletpassphrasechange <oldpassphrase> <newpassphrase>`

----------

TODO
-

Auto-populate RPC call list from help output, and gather documentation for each call.