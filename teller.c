#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"

BranchID getBranchID(AccountNumber accountNum)
{
  Y;
  return (BranchID)(accountNum >> 32);
}

/*
 * deposit money into an account
 * This method takes amount of money and deposits it into account balance.
 */
int Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%" PRIx64 " amount %" PRId64 ")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL)
  {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  pthread_mutex_lock(&(bank->branches[getBranchID(accountNum)].lock));
  Account_Adjust(bank, account, amount, 1);
  pthread_mutex_unlock(&(bank->branches[getBranchID(accountNum)].lock));

  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%" PRIx64 " amount %" PRId64 ")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  if (account == NULL)
  {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if (amount > Account_Balance(account))
  {
    return ERROR_INSUFFICIENT_FUNDS;
  }
  pthread_mutex_lock(&(bank->branches[getBranchID(accountNum)].lock));
  Account_Adjust(bank, account, -amount, 1);
  pthread_mutex_unlock(&(bank->branches[getBranchID(accountNum)].lock));

  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                      AccountNumber dstAccountNum,
                      AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoTransfer(src 0x%" PRIx64 ", dst 0x%" PRIx64
                ", amount %" PRId64 ")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL)
  {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL)
  {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if (amount > Account_Balance(srcAccount))
  {
    return ERROR_INSUFFICIENT_FUNDS;
  }

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);
  BranchID first;
  BranchID second;
  if (updateBranch)
  {
    //printf("Different branches Account transfer from %" PRIu64 " to %" PRIu64 "\n", srcAccountNum, dstAccountNum);
  }
  else
  {
    //printf("Same branches Account transfer from %" PRIu64 " to %" PRIu64 "\n", srcAccountNum, dstAccountNum);
  }
  if (updateBranch)
  {
    BranchID source = getBranchID(srcAccountNum);
    BranchID destination = getBranchID(dstAccountNum);
    first = (source < destination) ? source : destination;
    second = (source < destination) ? destination : source;
    // printf("Before bank lock in teller.c Branche transfer from %" PRIu64 " to %" PRIu64 "!!! of from account %" PRIu64 " to %" PRIu64 " amount:%" PRId64 "\n", source, destination, srcAccountNum, dstAccountNum, amount);

    pthread_mutex_lock(&(bank->lock));
    // printf("AFter bank lock in teller.c Branche transfer from %" PRIu64 " to %" PRIu64 "!!! of from account %" PRIu64 " to %" PRIu64 " amount:%" PRId64 "\n", source, destination, srcAccountNum, dstAccountNum, amount);

    pthread_mutex_lock(&(bank->branches[first].lock));
    pthread_mutex_lock(&(bank->branches[second].lock));
    //printf("Locked first %" PRIu64 " and second %" PRIu64 "\n", first, second);
  }
  Account_Adjust(bank, srcAccount, -amount, updateBranch);
  Account_Adjust(bank, dstAccount, amount, updateBranch);

  if (updateBranch)
  {
    pthread_mutex_unlock(&(bank->lock));
    // printf("AFter bank unlock in teller.c\n");

    pthread_mutex_unlock(&(bank->branches[first].lock));
    pthread_mutex_unlock(&(bank->branches[second].lock));

    //printf("UNLOCKED first %" PRIu64 " and second %" PRIu64 "\n", first, second);
  }
  //printf("Transfer from %" PRIu64 " to %" PRIu64 " ENDED SUCCESSFULLY\n", srcAccountNum, dstAccountNum);

  return ERROR_SUCCESS;
}
