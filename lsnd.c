#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"



/**************************SPRINTF********************************/
static void
sputc(char* buff, char c, int* index)
{
  buff[*index] = c;
  (*index)++;
}

static void
sprintint(char* buff, int xx, int base, int sgn, int* index)
{
  static char digits[] = "0123456789ABCDEF";
  char buf[16];
  int i, neg;
  uint x;

  neg = 0;
  if(sgn && xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);
  if(neg)
    buf[i++] = '-';

  while(--i >= 0)
    sputc(buff, buf[i], index);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void
sprintf(char* buff, const char *fmt, ...)
{
  char *s;
  int c, i, state;
  uint *ap;
  int index=0;

  state = 0;
  ap = (uint*)(void*)&fmt + 1;
  for(i = 0; fmt[i]; i++){
    c = fmt[i] & 0xff;
    if(state == 0){
      if(c == '%'){
        state = '%';
      } else {
        sputc(buff, c, &index);
      }
    } else if(state == '%'){
      if(c == 'd'){
        sprintint(buff, *ap, 10, 1,&index);
        ap++;
      } else if(c == 'x' || c == 'p'){
        sprintint(buff, *ap, 16, 0,&index);
        ap++;
      } else if(c == 's'){
        s = (char*)*ap;
        ap++;
        if(s == 0)
          s = "(null)";
        while(*s != 0){
          sputc(buff, *s,&index);
          s++;
        }
      } else if(c == 'c'){
        sputc(buff, *ap, &index);
        ap++;
      } else if(c == '%'){
        sputc(buff, c, &index);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        sputc(buff, '%',&index);
        sputc(buff, c,&index);
      }
      state = 0;
    }
  }
}
/**********************************************************************/


void print_formatted_inode_info(char* text){
    char *semicolon;
    char *newline;
    char buf[100];
    semicolon = text;
    int index = 0;
    while ((semicolon = strchr(semicolon, ':'))) {
        semicolon += 2;
        newline = strchr(semicolon, '\n');
        memmove(buf, semicolon, newline - semicolon);
        buf[newline - semicolon] = 0;
        if(index == 4) 
          printf(1, "(");
        printf(1, buf);
        if(index == 4) 
          printf(1, ")");
        printf(1, " ");
        index++;
    }
    printf(1, "\n");
}



int main() {
    int fd, inodeinfo_fd;
    char text_to_print[512], filename[50];
    struct dirent dir_entry;
    printf(1,"Starting lsnd...\n");
    if ((inodeinfo_fd = open("/proc/inodeinfo", 0)) < 0) {
        printf(1, "error in opening inodeinfo!\n");
        exit();
    }
    read(inodeinfo_fd, &dir_entry, sizeof(dir_entry)); // reading: .
    read(inodeinfo_fd, &dir_entry, sizeof(dir_entry)); // reading: ..

    while (read(inodeinfo_fd, &dir_entry, sizeof(dir_entry)) == sizeof(dir_entry)) {
      if(dir_entry.inum != 0){
        sprintf(filename, "/proc/inodeinfo/%s", dir_entry.name);
        fd = open(filename,0);
        if(fd >= 0){
          if (read(fd, text_to_print, 512) <= 0) {
              printf(2, "error in reading inode file!\n");
              exit();
          }
          print_formatted_inode_info(text_to_print);
          close(fd);
        }
      }
    }
    close(inodeinfo_fd);
    printf(1,"lsnd finished!\n");
    exit();
}