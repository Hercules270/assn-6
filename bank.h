#ifndef _BANK_H
#define _BANK_H

typedef struct Bank
{
  unsigned int numberBranches;
  struct Branch *branches;
  struct Report *report;
  pthread_mutex_t lock;     // This variable is used for locking bank while calculating bank_balance
  // This variable is used to wait all threads before Report_DoReport and wake them up when report's done
  pthread_cond_t condition; 
} Bank;

#include "account.h"

int Bank_Balance(Bank *bank, AccountAmount *balance, int workerNum);

Bank *Bank_Init(int numBranches, int numAccounts, AccountAmount initAmount,
                AccountAmount reportingAmount,
                int numWorkers);

int Bank_Validate(Bank *bank);
int Bank_Compare(Bank *bank1, Bank *bank2);

#endif /* _BANK_H */
