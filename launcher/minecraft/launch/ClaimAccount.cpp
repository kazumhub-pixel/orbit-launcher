#include "ClaimAccount.h"
#include <launch/LaunchTask.h>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

ClaimAccount::ClaimAccount(LaunchTask* parent, AuthSessionPtr session) : LaunchStep(parent)
{
    if (session->launchMode == LaunchMode::Normal) {
        auto accounts = APPLICATION->accounts();
        const int index=accounts->findAccountByProfileId(session->uuid);
        m_account = index>=0 ? accounts->at(index) : accounts->getAccountByProfileName(session->player_name);
    }
}

void ClaimAccount::executeTask()
{
    if (m_account) {
        lock.reset(new UseLock(m_account.get()));
    }
    emitSucceeded();
}

void ClaimAccount::finalize()
{
    lock.reset();
}
