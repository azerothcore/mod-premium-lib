/**
    This plugin can be used for common player customizations
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "CharacterDatabase.h"
#include "LoginDatabase.h"
#include "DatabaseWorkerPool.h"
#include "MySQLConnection.h"

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
    
    static bool HandlePremiumCharacterInfoCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* target = handler->getSelectedPlayer();
        string playerName = target->GetName().c_str();
        if (!target)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, 0))
            return false;

        int premium_level = GetCharacterPremiumLevel(target->GetGUIDLow());
        //int premium_level = target->GetCharacterPremiumLevel(target->GetGUIDLow());
        if (!premium_level)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("No premium level available for character.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        else
            (ChatHandler(handler->GetSession())).PSendSysMessage("Character premium level: %u.", premium_level);

        return true;
    }

    static bool HandlePremiumCharacterCreateCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

        // Which premium level to be added
        int premiumLevel = atoi((char*)args);

        Player* target = handler->getSelectedPlayer();
        string playerName = target->GetName().c_str();
        if (!target)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, 0))
            return false;

        bool characterPremiumLevelAdded = CreateCharacterPremiumLevel(target->GetGUIDLow(), premiumLevel);
        if (characterPremiumLevelAdded)
            (ChatHandler(handler->GetSession())).PSendSysMessage("Character created with premium level %u.", premiumLevel);
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("%s already has a premium level. Please remove first.", playerName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandlePremiumCharacterDeleteCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* target = handler->getSelectedPlayer();
        string playerName = target->GetName().c_str();
        if (!target)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, 0))
            return false;

        bool characterPremiumDeleted = DeleteCharacterPremiumLevel(target->GetGUIDLow());
        if (characterPremiumDeleted)
            (ChatHandler(handler->GetSession())).PSendSysMessage("Premium character deleted.");
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("No premium level assigned to %s.", playerName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandlePremiumAccountInfoCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* target = handler->getSelectedPlayer();
        string playerName = target->GetName().c_str();
        if (!target)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, 0))
            return false;

        uint64 accountID = target->ToPlayer()->GetSession()->GetAccountId();
        int premium_level = GetAccountPremiumLevel(accountID);
        if (!premium_level)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("%s's account doesn't have premium level.", playerName);
            handler->SetSentErrorMessage(true);
            return false;
        }
        else
            (ChatHandler(handler->GetSession())).PSendSysMessage("%s's account premium level: %u", playerName, premium_level);

        return true;
    }

    static bool HandlePremiumAccountCreateCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

        // Which premium level to be added
        int premiumLevel = atoi((char*)args);

        Player* target = handler->getSelectedPlayer();
        string playerName = target->GetName().c_str();
        if (!target)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, 0))
            return false;

        uint64 accountID = target->ToPlayer()->GetSession()->GetAccountId();

        bool characterPremiumLevelAdded = CreateAccountPremiumLevel(accountID, premiumLevel);
        if (characterPremiumLevelAdded)
            (ChatHandler(handler->GetSession())).PSendSysMessage("%s's account premium level set to: %u", playerName, premiumLevel);
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("%s's account already has a premium level. Remove first.", playerName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    static bool HandlePremiumAccountDeleteCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* target = handler->getSelectedPlayer();
        string playerName = target->GetName().c_str();
        if (!target)
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (handler->HasLowerSecurity(target, 0))
            return false;

        uint64 accountID = target->ToPlayer()->GetSession()->GetAccountId();

        bool characterPremiumDeleted = DeleteAccountPremiumLevel(accountID);
        if (characterPremiumDeleted)
            (ChatHandler(handler->GetSession())).PSendSysMessage("%s's Premium account deleted", playerName);
        else
        {
            (ChatHandler(handler->GetSession())).PSendSysMessage("No premium level assigned to %s's account.", playerName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    /* Premium character interaction.
     * These functions will allow modules to access the new generic
     * premium tables designed to help modules that interact with
     * players and characters without the need to create other tables.
     */
    static int8 GetCharacterPremiumLevel(uint64 guid)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT premium_level FROM premium_character WHERE character_id = %i", GUID_LOPART(guid));

        if (!result)
            return 0;

        int8 premium_level = (*result)[0].GetInt8();
        return premium_level;
    }


    static bool CreateCharacterPremiumLevel(uint64 guid, int8 premiumLevel)
    {
        // Validate if character to be inserted already has a premium level
        int hasPremiumLevel = GetCharacterPremiumLevel(guid);

        if (!hasPremiumLevel)
        {
            CharacterDatabase.PQuery("INSERT INTO premium_character (character_id, premium_level) VALUES (%i, %i)", GUID_LOPART(guid), premiumLevel);
            hasPremiumLevel = GetCharacterPremiumLevel(guid);
            if (!hasPremiumLevel)
                return false;

            return true;
        }
        else
            return false;
    }

    static bool DeleteCharacterPremiumLevel(uint64 guid)
    {
        // Validate if character to be removed has a premium level
        int hasPremiumLevel = GetCharacterPremiumLevel(guid);

        if (hasPremiumLevel)
        {
            CharacterDatabase.PQuery("DELETE FROM premium_character WHERE character_id = %i", GUID_LOPART(guid));
            int hasPremiumLevel = GetCharacterPremiumLevel(guid);
            if (!hasPremiumLevel)
                return true;

            return false;
        }
        else
            return false;
    }

    static int8 GetAccountPremiumLevel(uint64 accountID)
    {
        QueryResult result = LoginDatabase.PQuery("SELECT premium_level FROM premium_account WHERE account_id = %i", accountID);

        if (!result)
            return false;

        int8 account_premium_level = (*result)[0].GetInt8();
        return account_premium_level;
    }

    static bool CreateAccountPremiumLevel(uint64 accountID, int8 premiumLevel)
    {
        // Validate if account to be inserted already has a premium level
        int hasPremiumLevel = GetAccountPremiumLevel(accountID);

        if (!hasPremiumLevel)
        {
            LoginDatabase.PQuery("INSERT INTO premium_account (account_id, premium_level) VALUES (%i, %i)", accountID, premiumLevel);
            hasPremiumLevel = GetAccountPremiumLevel(accountID);

            if (!hasPremiumLevel)
                return false;

            return true;
        }
        else
            return false;
    }

    static bool DeleteAccountPremiumLevel(uint64 accountID)
    {
        // Validate if account to be removed has a premium level
        int hasPremiumLevel = GetAccountPremiumLevel(accountID);
        if (hasPremiumLevel)
        {
            LoginDatabase.PQuery("DELETE FROM premium_account WHERE account_id = %i", accountID);
            hasPremiumLevel = GetAccountPremiumLevel(accountID);
            if (!hasPremiumLevel)
                return true;

            return false;
        }
        else
            return false;
    }  
};

void AddPremiumCommandsScripts() {
    new PremiumCommands();
}

