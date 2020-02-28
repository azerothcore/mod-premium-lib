# mod-premium-lib

## Description

This module allows azerothcore to have premium/vip/special accounts or characters.
Currently only a few Getters and Setters have been implemented but more can be added to it as it is needed. You can always request features as issues.

## How to use ingame

In-game there are commands for accounts and characters
```
.premium account info
.premium account create 2 (Insert a number)
.premium account info
.premium character info
.premium character create 2 (Insert a number)
.premium character delete
```
---
- premium info of both account and character should give the premium level for each one. If there isn't a premium level to any, an error message will be delivered.
- premium create inserts on the database for characters and accounts acordingly. Auth stores premium accounts and characters stores premium characters. If you try to override a character with a new premium level, the gamemaster will be asked to delete first, then insert. The same will happen for account.
- premium delete will, as the command so specifies, remove the premium level for said character or account. If there's no level set, a message will clarify it.

**Note:** To set an account premium only needs to target a player and the script will look for that player's account so the only parameters that we need to send to the console are the premium levels when creating.

## Requirements

This module doesn't require anything other than a compiled version of Azerothcore. It can however serve as a requirement for other modules.
- AzerothCore v2.0.0+

## Installation

```
1) Simply `git clone` the module under the `modules` directory of your AzerothCore source or copy paste it manually.
2) Import the SQL manually to the right Database (auth, world or characters) or with the `db_assembler.sh` (if `include.sh` provided).
3) Re-run cmake and launch a clean build of AzerothCore.
```

## Edit the module's configuration (optional)

If you need to change the module configuration, go to your server configuration directory (where your `worldserver` or `worldserver.exe` is), copy `my_module.conf.dist` to `my_module.conf` and edit that new file.


## Credits

* [Gengar shadowball](https://github.com/gengarshadowball)
* AzerothCore: [repository](https://github.com/azerothcore) - [website](http://azerothcore.org/) - [discord chat community](https://discord.gg/PaqQRkd)
