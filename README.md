Noirbits (2.2.1.0)
-

Noirbits is a community coin, originally launched by [barwizi](https://github.com/Nameshar/Noirbits). After some political turmoil,
his version is no longer maintained and this is considered to be the new official client.

Download binaries
-

Head over to the [official Noirbits website](http://www.noirbits.org) to get the precompiled binaries. All binaries
released on the official are guaranteed to be virus-free, and we cannot urge you enough to be suspicious when retrieving the binaries from anywhere else. A known exception to that rule is the links provided on the [Noirbits thread](https://bitcointalk.org/index.php?topic=270264.0). 

Contributing
-

Everyone is welcome to participate. Just fork, code, compile, test, and issue a pull request when you feel your changes are ready
for mainstream. Regular, serious coders will be added as collaborators to the repository on request.

When coding, please use [Allman style](http://en.wikipedia.org/wiki/Indent_style#Allman_style) for indentation and block scopes.

Operators require spaces around them, parentheses do not, and so... Pull requests failing to respect conventions will be rejected. Once the wiki is setup, a more complete
coding convention guide will be available.

New Features
-

Despite Noirbits originally being a foocoin clone, in all its flaws and glory, active development is under way to restore the coin
to prime-quality code and features.

The recently implemented features are :

* UPNP support restore (foocoin has all UPNP code stripped out, compiling with UPNP had absolutely no effect).
* Transaction refunds. Now available in GUI client. Incoming transaction can be refunded, provided the associated coins have not been spent. The RPC call is undocumented
as of now, but in place. The call can also be used to forward coins to another address.

	`Noirbitsd refundtransaction <transactionHash> [<return address>]`

Noirbits wiki
-

A wiki will be soon set up to document usage and features.
