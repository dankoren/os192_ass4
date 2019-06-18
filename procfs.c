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
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "buf.h"

#define NINODES 200


#define PROCFS_DIR 0
#define PROCFS_FILE 1
#define PROCFS_PROCESS_FILE 2
#define PROC_INUM 19
#define IDEINFO_INUM 201
#define FILESTAT_INUM 202
#define INODEINFO_INUM 203
#define INODE_OFFSET_START 210

#define PID_INUM_START 300

#define MINOR_IDEINFO 1


int proc_dir_read(struct inode* ip, char* dst, int off);
int pid_dir_read(char* dst, int off,int inum);
int filestat_file_read(char* dst);
int ideinfo_file_read(char* dst);
int inodeinfo_dir_read(char* dst, int);
int inode_exist(int inum, int* inum_fds);
int inodeinfo_file_read(char* dst, int inode_offset);

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
  /* if(ip->inum == PROC_INUM){
    ip->size = 7 * sizeof(struct dirent);
    ip->minor = PROCFS_DIR;
  }*/
  if(ip->inum == IDEINFO_INUM || ip->inum == FILESTAT_INUM ||
    (ip->inum >= INODE_OFFSET_START && ip->inum < PID_INUM_START)){
    //ip->size = 50;
    ip->size = 0;
    ip->minor = PROCFS_FILE;
  }
  else if(ip->inum  == INODEINFO_INUM){
    ip->minor = PROCFS_DIR;
  }
 /* else if(ip->inum == FILESTAT_INUM){
    ip->minor = PROCFS_FILE;
    //ip->size = 50;
  }
  else if(ip->inum == INODEINFO_INUM){
    ip->minor = PROCFS_FILE;
  }*/
  else if(ip->inum >= PID_INUM_START){
    if(ip->inum % 3 == 0){
      ip->minor = PROCFS_DIR;
      ip->size = 2 * sizeof(struct dirent);
    }
    else if(ip->inum % 3 == 1 || ip->inum % 3 == 2){
      //ip->size = 16;
      ip->minor = PROCFS_FILE;
      ip->size = 0;
    }
    /*if(ip->inum % 3 == 2){
      ip->minor = PROCFS_FILE;
      //ip->size = 50;
    }*/
  }
}

int
procfsread(struct inode *ip, char *dst, int off, int n) {
  //cprintf("read from inode: %d, offset: %d\n",ip->inum,off);
  if(!procfsisdir(ip) && off > 0)
    return 0;
  if(ip->inum == PROC_INUM){ // /proc/
    return proc_dir_read(ip,dst,off);
  }
  else if(ip->inum >= PID_INUM_START && ip->inum % 3 == 0){ // /proc/PID/
    return pid_dir_read(dst,off,ip->inum);
  }
  else if(ip->inum == IDEINFO_INUM){ // /proc/ideinfo
    return ideinfo_file_read(dst);
  }
  else if(ip->inum == FILESTAT_INUM){ // /proc/filestat
    return filestat_file_read(dst);
  }
  else if(ip->inum == INODEINFO_INUM){ // /proc/inodeinfo
    return inodeinfo_dir_read(dst,off);
  }
  else if(ip->inum >= INODE_OFFSET_START && ip->inum < PID_INUM_START){ // /pid/inodeinfo/INDEX
    int inode_offset = ip->inum - INODE_OFFSET_START;
    return inodeinfo_file_read(dst,inode_offset);
  }
  else if(ip->inum >= PID_INUM_START){
    int proc_index = (ip->inum - PID_INUM_START) / 3;
    //cprintf("index: %d, pid: %d\n",proc_index, get_pid_by_index(proc_index));
    //struct proc* p = get_proc_by_pid(proc_id);
    if(get_pid_by_index(proc_index) == 0){
      return 0;
    }
    if(ip->inum % 3 == 1){ // /proc/PID/name
      sprintf(dst, "%s\n", get_name_by_index(proc_index));
      return strlen(dst);
    }
    else if(ip->inum % 3 == 2){ // /proc/PID/status
      sprintf(dst, "%s %d\n", get_state_name_by_index(proc_index), get_size_by_index(proc_index));
      return strlen(dst);
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


/************************************************************************************/

int proc_dir_read(struct inode* ip, char* dst, int off){
  //cprintf("reading from /proc, size is: %d, off is: %d\n", ip->size, off);
  struct dirent* dirent_dst = (struct dirent *)dst;
  if(off == 0){
    safestrcpy(dirent_dst->name, ".", sizeof("."));
    dirent_dst->inum = PROC_INUM;
    return sizeof(struct dirent);
  }
  else if(off == sizeof(struct dirent)){
    safestrcpy(dirent_dst->name, "..", sizeof(".."));
    dirent_dst->inum = ROOTINO;
    return sizeof(struct dirent);
  }
  if(off == 2 * sizeof(struct dirent)){
    safestrcpy(dirent_dst->name, "ideinfo", sizeof("ideinfo"));
    dirent_dst->inum = IDEINFO_INUM;
    return sizeof(struct dirent);
    }
  else if(off == 3 * sizeof(struct dirent)){
    safestrcpy(dirent_dst->name, "filestat", sizeof("filestat"));
    dirent_dst->inum = FILESTAT_INUM;
    return sizeof(struct dirent);
    }
  else if(off == 4 * sizeof(struct dirent)){
    safestrcpy(dirent_dst->name, "inodeinfo", sizeof("inodeinfo"));
    dirent_dst->inum = INODEINFO_INUM;
    return sizeof(struct dirent);
  }
  else{ //PID
    int proc_index = (off / sizeof(struct dirent)) - 5;
    int pid = get_pid_by_index(proc_index);
    if(pid==0){
      return 0;
    }
    char proc_id_string[100] = {0};
    sprintf(proc_id_string, "%d", pid);
    dirent_dst->inum = PID_INUM_START + 3 * proc_index; 
    //cprintf("reached pid: proc_id: %d,  inum: %d proc_id_string: %s\n", proc_id,  dirent_dst->inum,proc_id_string);
    safestrcpy(dirent_dst->name, proc_id_string, sizeof(dirent_dst->name));
    return sizeof(struct dirent);
  }
}


int pid_dir_read(char* dst, int off,int inum){
  struct dirent* dirent_dst = (struct dirent *)dst;
  if(off == 0){
    safestrcpy(dirent_dst->name, ".", sizeof("."));
    dirent_dst->inum = inum;
    return sizeof(struct dirent);
  }
  else if(off == sizeof(struct dirent)){
    safestrcpy(dirent_dst->name, "..", sizeof(".."));
    dirent_dst->inum = PROC_INUM;
    return sizeof(struct dirent);
  }
  else if(off == 2 * sizeof(struct dirent)){
    safestrcpy(dirent_dst->name, "name", sizeof("name"));
    dirent_dst->inum = inum + 1;
    return sizeof(struct dirent);
    }
  else if(off == 3 * sizeof(struct dirent)){
    safestrcpy(dirent_dst->name, "status", sizeof("status"));
    dirent_dst->inum = inum + 2;
    return sizeof(struct dirent);
  }
  else{
    return 0;
  }
}

int inode_exist(int inum, int* inum_fds){
  int i;
  for(i=0;i<NFILE;i++){
    if(inum_fds[i] == inum){
      return 1;
    }
  }
  return 0;
}

int filestat_file_read(char* dst){
  int inum_fds[NFILE];
  int free_fds=0, unique_inode_fds=0, writeable_fds=0, readable_fds=0, i, total_refs=0;
  struct file* ftable = get_ftable();
  for(i = 0; i < NFILE; i++){
    if(ftable[i].ref == 0){
      free_fds++;
    }
    else{
      if(ftable[i].readable == 1)
        readable_fds++;
      if(ftable[i].writable == 1)
        writeable_fds++;
      total_refs += ftable[i].ref;
      if(!inode_exist(ftable[i].ip->inum,inum_fds))
        unique_inode_fds++;
    }
  }
  int refs_per_fds = total_refs / (NFILE - free_fds);
  sprintf(dst,"Free fds: %d\nUnique inode fds: %d\nWriteable fds: %d\nReadable fds: %d\nRefs per fds: %d\n", 
      free_fds, unique_inode_fds, writeable_fds, readable_fds, refs_per_fds);
  return strlen(dst);
}

int inodeinfo_dir_read(char* dst, int off){
  struct dirent* dirent_dst = (struct dirent *)dst;
  memset(dirent_dst->name,0,14);
	if (off == 0){
		safestrcpy(dirent_dst->name, ".", sizeof("."));
		dirent_dst->inum = INODEINFO_INUM;
	}
	else if(off == 1*sizeof(struct dirent)){
		safestrcpy(dirent_dst->name, "..", sizeof(".."));
		dirent_dst->inum = PROC_INUM;
	}
	else{
    int inode_num = off / sizeof(struct dirent) - 2;
		dirent_dst->inum = INODE_OFFSET_START + inode_num; 
    int index = find_inode_index_in_icache(inode_num);
		if(index == -1){
			return 0;
		}
    //char buff[15];
    sprintf(dirent_dst->name,"%d",index);
    //safestrcpy(dirent_dst->name, buff, sizeof(dirent_dst->name));
	}
  return sizeof(struct dirent);
}

int inodeinfo_file_read(char* dst, int inode_offset){
  struct inode* ip = find_inode_in_icache(inode_offset);
  if(ip==0)
    return 0;
  static char *types[] = {
    [T_DIR]    "DIR",
    [T_FILE]    "FILE",
    [T_DEV]  "DEV"
    };
  sprintf(dst,"Device: %d\nInode number: %d\nis valid: %d\ntype: %s\nmajor minor: (%d,%d)\nhard links: %d\nblocks used: %d\n",
    ip->dev, ip->inum, ip->valid, types[ip->type], ip->major, ip->minor, ip->nlink, ip->size);
  return strlen(dst);
}




int ideinfo_file_read(char* dst){
  struct buf* idequeue = get_idequeue();
  int total_waiting = 0, reading_waiting = 0, writing_waiting = 0;
  char working_blocks_str[1000] ={0};
  struct buf* b = idequeue;
  while(b != 0){
    total_waiting++;
    if((b->flags & B_VALID) == 0){
      reading_waiting++;
    }
    if((b->flags & B_DIRTY) != 0){ // B_DIRTY on means write
      writing_waiting++;
    }
    b = b->next;
    char working_block[10]={0};
    sprintf(working_block,"(%d,%d);", b->dev, b->blockno);
    safestrcpy(working_blocks_str + strlen(working_blocks_str), working_block ,strlen(working_block));
  }
  sprintf(dst, "Waiting operations:%d\nRead waiting operations:%d\nWrite waiting operations:%d\nWorking blocks:%s\n",
  total_waiting, reading_waiting, writing_waiting, working_blocks_str);
  return strlen(dst);
}

