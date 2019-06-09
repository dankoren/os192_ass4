#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
//#include "mmu.h"
//#include "proc.h"
//#include "x86.h"

#define NINODES 200


#define PROCFS_DIR 0
#define PROC_INUM 18
#define IDEINFO 0
#define FILESTAT 1
#define INODEINFO 2




#define MINOR_IDEINFO 1

char*
safestrcpy2(char *s, const char *t, int n)
{
  char *os;

  os = s;
  if(n <= 0)
    return os;
  while(--n > 0 && (*s++ = *t++) != 0)
    ;
  *s = 0;
  return os;
}



//returns 0 if inode is not of type directory, else returns 1
int 
procfsisdir(struct inode *ip) {
  if(ip->minor != PROCFS_DIR)
    return 0;
  else
    return 1;
}

void 
procfsiread(struct inode* dp, struct inode *ip) {
  ip->major = PROCFS;
  ip->type = T_DEV;
  ip->valid = 1;
  ip->minor = 1;
  ip->size = 1;
}


void f(struct dirent* d){
  safestrcpy2(d->name, "filestat", sizeof(d->name));
}


int
procfsread(struct inode *ip, char *dst, int off, int n) {
  // if(off >= ip->size)
  //  return 0;
  if(ip->minor == PROCFS_DIR ){
    //if(ip->inum == PROC_INUM){ // case it's /proc/
        /* if(off == IDEINFO * sizeof(struct dirent)){
         safestrcpy2(((struct dirent *)dst)->name, "ideinfo", sizeof(((struct dirent *)dst)->name));
         ((struct dirent *)dst)->inum = NINODES + IDEINFO;
         return sizeof(struct dirent);
       }*/
        if(off == FILESTAT * sizeof(struct dirent)){
          struct dirent* d =  ((struct dirent *)dst);
          f(d);
         ((struct dirent *)dst)->inum = NINODES + FILESTAT;
         return sizeof(struct dirent);
      }
       /* else if(off == INODEINFO * sizeof(struct dirent)){
         safestrcpy(((struct dirent *)dst)->name, "inodeinfo", sizeof(((struct dirent *)dst)->name));
         ((struct dirent *)dst)->inum = NINODES + INODEINFO;
         return sizeof(struct dirent);
       }
       else{
         return 0;
       }*/
    //}
  }
  else if(ip->minor == 1){
    //cprintf("reached minor = 1\n");
    // if(ip->inum == NINODES + IDEINFO){
    //   dst[0] = 200;
    //   dst[1] = 233;
    //   return 2;
    // }
    if(ip->inum == NINODES + FILESTAT){
      dst[0] = 90;
      //getFds();
      return 1;
    }
  }
  

  return 0;
}

int
procfswrite(struct inode *ip, char *buf, int n)
{
  return 0;
}

void
procfsinit(void)
{
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}

