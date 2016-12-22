D2VDump is a [Metamod:Source](http://metamodsource.net) plugin for Dota 2 that dumps out the VScript API functions and globals.

It outputs the dumps upon unload (including server exit), and currently supports JSON. Other formats may be added in the future.

# Compile-time Dependencies
* The [S2](https://github.com/alliedmodders/metamod-source/tree/S2) branch of Metamod:Source.
* The [dota 'hl2sdk'](https://github.com/alliedmodders/hl2sdk/tree/dota) from AlliedModders.
* [Jansson](http://www.digip.org/jansson/) (only v2.5 and v2.6 have been tested, compiled and linked statically).

# Run-time Dependencies
* Metamod:Source for Dota / Source 2, including gameinfo.gi edit for it to load.
* If wanting to see bot VM functions, another plugin to trigger its initialization.
