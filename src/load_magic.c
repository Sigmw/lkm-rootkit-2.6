#include <linux/random.h>

#include "load_magic.h"


int MAGIC;
int magic_status;

void init_load_magic(void)
{

  get_random_bytes(&MAGIC, sizeof(MAGIC));
  magic_status = MAGIC;
}

void unset_magic(void)
{
tor
  magic_status = 0;
}

int check_load_magic(void)
{
  return MAGIC == magic_status;
}

