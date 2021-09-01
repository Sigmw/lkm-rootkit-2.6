#ifndef FILE_HIDING_H
#define FILE_HIDING_H

#include <linux/dirent.h>
#include "brootus.h"


struct linux_dirent {
  unsigned long   d_ino;
  unsigned long   d_off;
  unsigned short  d_reclen;
  char            d_name[1];
};

BROOTUS_MODULE(file_hiding);

extern void set_file_prefix(char* prefix);

#endif
