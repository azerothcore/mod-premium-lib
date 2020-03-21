/**
    This plugin can be used for common player customizations
 */

#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "Language.h"
#include "CharacterDatabase.h"
#include "LoginDatabase.h"
#include "DatabaseWorkerPool.h"
#include "MySQLConnection.h"
#include "PremiumLib.h"

/* Premium character interaction.
     * These functions will allow modules to access the new generic
     * premium tables designed to help modules that interact with
     * players and characters without the need to create other tables.
*/

PremiumLibData PremiumLibManager::GetAccountPremiumLevel(uint64 accountID)
{
    QueryResult result = LoginDatabase.PQuery("SELECT premium_level, duration, FROM_UNIXTIME(duration) FROM premium_account WHERE account_id = %u", accountID);

    if (!result)
        return PremiumLibData();

    PremiumLibData data;
    data.premiumLevel = (*result)[0].GetInt8();
    data.expirationDateUnixtime = (*result)[1].GetInt32();
    data.expirationDate = (*result)[2].GetCString();
    return data;
}

PremiumLibData PremiumLibManager::GetCharacterPremiumLevel(uint64 guid)
{
    QueryResult result = CharacterDatabase.PQuery("SELECT premium_level, duration, FROM_UNIXTIME(duration) FROM premium_character WHERE character_id = %u", GUID_LOPART(guid));

    if (!result)
        return PremiumLibData();

    PremiumLibData data;
    data.premiumLevel = (*result)[0].GetInt8();
    data.expirationDateUnixtime = (*result)[1].GetInt32();
    data.expirationDate = (*result)[2].GetCString();
    return data;
}

bool PremiumLibManager::CreateAccountPremiumLevel(uint64 accountID, int8 premiumLevel)
{
    // Validate if account to be inserted already has a premium level
    PremiumLibData data = GetAccountPremiumLevel(accountID);

    if (!data.premiumLevel)
    {
        uint32 expirationDateUnixtime = duration != 0 ? time(0) + duration * DAY : 0;
        LoginDatabase.PQuery("INSERT INTO premium_account (account_id, premium_level, duration) VALUES (%u, %i, %u)", accountID, premiumLevel, expirationDateUnixtime);
        return true;
    }

    return false;
}

bool PremiumLibManager::CreateCharacterPremiumLevel(uint64 guid, int8 premiumLevel)
{
    // Validate if character to be inserted already has a premium level
    PremiumLibData data = GetCharacterPremiumLevel(guid);

    if (!data.premiumLevel)
    {
        uint32 expirationDateUnixtime = duration != 0 ? time(0) + duration * DAY : 0;
        CharacterDatabase.PQuery("INSERT INTO premium_character (character_id, premium_level, duration) VALUES (%u, %i, %u)", GUID_LOPART(guid), premiumLevel, expirationDateUnixtime);
        return true;
    }

    return false;
}

bool PremiumLibManager::DeleteAccountPremiumLevel(uint64 accountID)
{
    // Validate if account to be removed has a premium level
    PremiumLibData data = GetAccountPremiumLevel(accountID);

    if (data.premiumLevel)
    {
        LoginDatabase.PQuery("DELETE FROM premium_account WHERE account_id = %i", accountID);
        return true;
    }

    return false;
}

bool PremiumLibManager::DeleteCharacterPremiumLevel(uint64 guid)
{
    // Validate if character to be removed has a premium level
    PremiumLibData data = GetCharacterPremiumLevel(guid);

    if (data.premiumLevel)
    {
        CharacterDatabase.PQuery("DELETE FROM premium_character WHERE character_id = %u", GUID_LOPART(guid));
        return true;
    }

    return false;
}

class PremiumCommands : public CommandScript
{
public:
    PremiumCommands() : CommandScript("PremiumCommands") {  }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> characterCommandTable
        {
            { "create",     SEC_GAMEMASTER, false, &HandlePremiumCharacterCreateCommand, "" },
            { "delete",     SEC_GAMEMASTER, false, &HandlePremiumCharacterDeleteCommand, "" },
            { "info",       SEC_GAMEMASTER, false, &HandlePremiumCharacterInfoCommand, "" }
        };

        static std::vector<ChatCommand> accountCommandTable
        {
            { "create",     SEC_GAMEMASTER, false, &HandlePremiumAccountCreateCommand, "" },
            { "delete",     SEC_GAMEMASTER, false, &HandlePremiumAccountDeleteCommand, "" },
            { "info",       SEC_GAMEMASTER, false, &HandlePremiumAccountInfoCommand, "" }
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "account",    SEC_GAMEMASTER, false, nullptr, "", accountCommandTable},
            { "character",  SEC_GAMEMASTER, false, nullptr, "", characterCommandTable}
        };
        static std::vector<ChatCommand> premiumCommandTable =
        {
            { "premium",    SEC_GAMEMASTER, false, nullptr, "", commandTable}
        };
        return premiumCommandTable;
    }
    
    static bool HandlePremiumCharacterInfoCommand(ChatHandler* handler, const char* args)
    {
        Player* target = nullptr;
        std::string playerName;
        uint64 playerGUID;

        if (!handler->extractPlayerTarget((char*)args, &target, &playerGUID, &playerName))
            return false;

        if (target)
            if (handler->HasLowerSecurity(target, 0))
                return false;

        PremiumLibData data = sPremiumLib->GetCharacterPremiumLevel(playerGUID);
        if (!data.premiumLevel)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("No premium level available for character %s.", playerName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }
        else
            (ChatHandler(handler->GetSession())).PSendSysMessage("Character %s premium level: %u, expires: %s.", playerName.c_str(), data.premiumLevel, sPremiumLib->duration != 0 ? data.expirationDate.c_str() : "Never");

        return true;
    }

    static bool HandlePremiumCharacterCreateCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

        Player* target = nullptr;
        std::string playerName;
        uint64 playerGUID;
        char* premiumLevelStr;

        char* playerNameStr = strtok((char*)args, " ");
        premiumLevelStr = strtok(nullptr, "");
        if (!atoi(playerNameStr)) // first argument is not an int
        {
            if (!handler->extractPlayerTarget(playerNameStr, nullptr, &playerGUID, &playerName))
                return false;
        }
        else
        {
            if (!handler->extractPlayerTarget(nullptr, &target, &playerGUID, &playerName))
                return false;
            // Which premium level to be added
            premiumLevelStr = playerNameStr; // selected target but without the player name as an argument, the first argument will be the premium level
        }
        
        if (!premiumLevelStr || !atoi(premiumLevelStr))
            return false;

        int premiumLevel = atoi(premiumLevelStr);
        if (premiumLevel < 1) // anti retards check
            return false;

        if (target)
            if (handler->HasLowerSecurity(target, 0))
                return false;

        bool characterPremiumLevelAdded = sPremiumLib->CreateCharacterPremiumLevel(playerGUID, premiumLevel);
        if (characterPremiumLevelAdded)
            (ChatHandler(handler->GetSession())).PSendSysMessage("Premium level %u added to character %s", premiumLevel, playerName.c_str());
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("%s already has a premium level. Please remove it first.", playerName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandlePremiumCharacterDeleteCommand(ChatHandler* handler, const char* args)
    {
        Player* target = nullptr;
        std::string playerName;
        uint64 playerGUID;

        if (!handler->extractPlayerTarget((char*)args, &target, &playerGUID, &playerName))
            return false;

        if (target)
            if (handler->HasLowerSecurity(target, 0))
                return false;

        bool characterPremiumDeleted = sPremiumLib->DeleteCharacterPremiumLevel(playerGUID);
        if (characterPremiumDeleted)
            (ChatHandler(handler->GetSession())).PSendSysMessage("Premium character %s deleted.", playerName.c_str());
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("No premium level assigned to %s.", playerName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandlePremiumAccountInfoCommand(ChatHandler* handler, const char* args)
    {
        Player* target = nullptr;
        std::string playerName;
        std::string playerAccount;
        std::string expirationDate;

        if (*args)
            playerAccount = strtok((char*)args, "");
        else if (!handler->extractPlayerTarget(nullptr, &target, nullptr, &playerName))
            return false;

        if (target)
            if (handler->HasLowerSecurity(target, 0))
                return false;

        if (!Utf8ToUpperOnlyLatin(playerAccount))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, playerAccount.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint64 accountID = target ? target->GetSession()->GetAccountId() : AccountMgr::GetId(playerAccount);
        if (!accountID)
            return false;
        std::string accountName;
        if (!AccountMgr::GetName(accountID, accountName))
            return false;

        PremiumLibData data = sPremiumLib->GetAccountPremiumLevel(accountID);
        if (!data.premiumLevel)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("Account %s doesn't have premium level.", accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }
        else
            (ChatHandler(handler->GetSession())).PSendSysMessage("Account %s premium level: %u, expires: %s.", accountName.c_str(), data.premiumLevel, sPremiumLib->duration != 0 ? data.expirationDate.c_str() : "Never");

        return true;
    }

    static bool HandlePremiumAccountCreateCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

        Player* target = nullptr;
        std::string playerAccount;
        char* premiumLevelStr;

        char* playerAccountStrOrPremiumLevel = strtok((char*)args, " ");
        if (!atoi(playerAccountStrOrPremiumLevel))
        {
            playerAccount = playerAccountStrOrPremiumLevel;
            premiumLevelStr = strtok(nullptr, "");
        }
        else if (handler->extractPlayerTarget(nullptr, &target, nullptr, nullptr)) // the first argument will be the premium level
            premiumLevelStr = playerAccountStrOrPremiumLevel;
        else
            return false;

        // Which premium level to be added
        if (!premiumLevelStr || !atoi(premiumLevelStr))
            return false;

        int premiumLevel = atoi(premiumLevelStr);
        if (premiumLevel < 1) // anti retards check
            return false;

        if (target)
            if (handler->HasLowerSecurity(target, 0))
                return false;

        uint64 accountID = target ? target->GetSession()->GetAccountId() : AccountMgr::GetId(playerAccount);
        if (playerAccount.empty())
            AccountMgr::GetName(accountID, playerAccount);

        bool characterPremiumLevelAdded = sPremiumLib->CreateAccountPremiumLevel(accountID, premiumLevel);
        if (characterPremiumLevelAdded)
            (ChatHandler(handler->GetSession())).PSendSysMessage("Account %s premium level set to: %u.", playerAccount.c_str(), premiumLevel);
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("Account %s already has a premium level. Remove it first.", playerAccount.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandlePremiumAccountDeleteCommand(ChatHandler* handler, const char* args)
    {
        Player* target = nullptr;
        std::string playerName;
        std::string playerAccount;

        if (*args)
            playerAccount = strtok((char*)args, "");
        else if (!handler->extractPlayerTarget(nullptr, &target, nullptr, &playerName))
            return false;

        if (target)
            if (handler->HasLowerSecurity(target, 0))
                return false;

        if (!Utf8ToUpperOnlyLatin(playerAccount))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, playerAccount.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint64 accountID = target ? target->GetSession()->GetAccountId() : AccountMgr::GetId(playerAccount);
        if (!accountID)
            return false;
        std::string accountName;
        if (!AccountMgr::GetName(accountID, accountName))
            return false;

        bool characterPremiumDeleted = sPremiumLib->DeleteAccountPremiumLevel(accountID);
        if (characterPremiumDeleted)
            (ChatHandler(handler->GetSession())).PSendSysMessage("Premium account deleted for %s.", accountName.c_str());
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("No premium level assigned to account %s.", accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }
};

class PremiumLibWorld : public WorldScript
{
public:
    PremiumLibWorld() : WorldScript("PremiumLibWorld") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload)
        {
            std::string conf_path = _CONF_DIR;
            std::string cfg_file = conf_path + "/mod_premium_lib.conf";
#ifdef WIN32
            cfg_file = "mod_premium_lib.conf";
#endif
            std::string cfg_def_file = cfg_file + ".dist";

            sConfigMgr->LoadMore(cfg_def_file.c_str());
            sConfigMgr->LoadMore(cfg_file.c_str());

            sPremiumLib->duration = sConfigMgr->GetIntDefault("PremiumLib.Duration", 30);
        }
    }
};

class PremiumLibPlayer : public PlayerScript
{
public:
    PremiumLibPlayer() : PlayerScript("PremiumLibPlayer") { }

    // Remove expired premiums. We only do this OnLogin for performance and simplicity.
    void OnLogin(Player* player) override
    {
        uint64 playerGUID = player->GetGUID();
        uint32 accountID = player->GetSession()->GetAccountId();
        PremiumLibData accData = sPremiumLib->GetAccountPremiumLevel(accountID);
        time_t now = time(nullptr);

        if (accData.premiumLevel && accData.expirationDateUnixtime != 0 && accData.expirationDateUnixtime <= now)
            sPremiumLib->DeleteAccountPremiumLevel(accountID);

        PremiumLibData charData = sPremiumLib->GetCharacterPremiumLevel(playerGUID);
        if (charData.premiumLevel && charData.expirationDateUnixtime != 0 && charData.expirationDateUnixtime <= now)
            sPremiumLib->DeleteCharacterPremiumLevel(playerGUID);
    }
};

void AddPremiumLibScripts()
{
    new PremiumCommands();
    new PremiumLibWorld();
    new PremiumLibPlayer();
}

