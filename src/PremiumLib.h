#ifndef MOD_PREMIUM_LIB_H
#define MOD_PREMIUM_LIB_H

#include "ScriptMgr.h"

struct PremiumLibData
{
    int8 premiumLevel;
    uint32 expirationDateUnixtime;
    std::string expirationDate;
};

class PremiumLibManager
{
    friend class ACE_Singleton<PremiumLibManager, ACE_Null_Mutex>;
    public:
        // GETS
        PremiumLibData GetAccountPremiumLevel(uint64 accountID);
        PremiumLibData GetCharacterPremiumLevel(uint64 guid);
        // CREATE
        bool CreateAccountPremiumLevel(uint64 accountID, int8 premiumLevel);
        bool CreateCharacterPremiumLevel(uint64 guid, int8 premiumLevel);
        // DELETE
        bool DeleteAccountPremiumLevel(uint64 accountID);
        bool DeleteCharacterPremiumLevel(uint64 guid);

        int8 duration;
};

#define sPremiumLib ACE_Singleton<PremiumLibManager, ACE_Null_Mutex>::instance()

#endif
