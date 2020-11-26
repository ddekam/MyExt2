/************* level1.c functions **************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include <unistd.h>
#include <sys/types.h>

char *t1 = "xwrxwrxwr-------";
char fullPath[256] = "";

#define OWNER 000700
#define GROUP 000070
#define OTHER 000007

#include "type.h"
#include "util.c"
#include "Functions/mkdir.c"
#include "Functions/rmdir.c"
#include "Functions/cd.c"
#include "Functions/ls.c"
#include "Functions/pwd.c"
#include "Functions/creat.c"
#include "Functions/h_link.c"
#include "Functions/unlink.c"
#include "Functions/s_link.c"

MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;

char gpath[256]; // global for tokenized components
char *names[64]; // assume at most 64 components in pathname
int n = 0;       // number of component strings

int developerMode = 0; // if 1: prints additional info for developer
                       //   0: encapsulates excess info from user

int fd = 0, dev = 0;
int nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd_entered[32], pathname[256], second_path[256];
char *disk = "disk";

// Initialize data structures of LEVEL-1
int init()
{
  int i, j;
  MINODE *mip;
  PROC *p;

  if (developerMode)
  {
    printf("init()\n");
  }

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }

  for (i = 0; i < NPROC; i++)
  {
    p = &proc[i];
    p->pid = i;
    p->uid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j = 0; j < NFD; j++)
      p->fd[j] = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{
  if (developerMode)
  {
    printf("mount_root()\n");
  }

  root = iget(dev, 2);
}

int findCmd(char *command)
{
  int i = 0;
  char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "link",
                 "unlink", "symlink", "quit", NULL};

  while (cmd[i])
  {
    if (!strcmp(command, cmd[i]))
    {
      return i; // found command: return index i
    }

    i++;
  }

  printf("%s: command not found\n", cmd_entered);
  return -1; // not found: return -1
}

// Exit the program
int quit(char *path, char *second)
{
  int i;
  MINODE *mip;

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];

    if (mip->refCount > 0)
    {
      iput(mip);
    }
  }

  exit(0);
}

int main(int argc, char *argv[])
{
  int ino = 0;
  int r = 0, index = 0;
  int (*fptr[])(char *, char *) = {(int (*)())make_dir, rm_dir, ls_dir,
                                   change_dir, pwd, create, mylink, myunlink, mysymlink, quit};
  char buf[BLKSIZE] = "";
  char shellprompt[32] = "mysh>";

  printf("\n\n\n---- WELCOME TO MY SHELL PROGRAM ----\ndeveloped by: Dayton Dekam\n");

  if (argc > 1)
  {
    disk = argv[1];

    if (argc > 2)
    {
      if (strcmp(argv[2], "developermode") == 0)
      {
        developerMode = 1;
        printf("DEVELOPER MODE\t[ON]\n\n");
      }
    }
  }

  if (developerMode)
  {
    printf("checking EXT2 FS ....");
  }

  if ((fd = open(disk, O_RDWR)) < 0)
  {
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;
  /********** read super block at 1024 ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system *****************/
  if (sp->s_magic != 0xEF53)
  {
    printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
    exit(1);
  }

  if (developerMode)
  {
    printf("Ext2 Filesystem Confirmed\n");
  }

  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;

  if (developerMode)
  {
    printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);
  }

  init();
  mount_root();

  if (developerMode)
  {
    printf("root refCount = %d\n", root->refCount);
    printf("creating P0 as running process\n");
  }

  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);

  if (developerMode)
  {
    printf("root refCount = %d\n", root->refCount);
  }

  while (1)
  {
    printf("%s> ", shellprompt);
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;
    if (line[0] == 0)
      continue;
    pathname[0] = 0;
    cmd_entered[0] = 0;
    second_path[0] = 0;

    sscanf(line, "%s %s %s", cmd_entered, pathname, second_path);

    if (developerMode)
    {
      printf("cmd=%s pathname=%s\n", cmd_entered, pathname);
    }

    index = findCmd(cmd_entered);
    r = fptr[index](pathname, second_path);
  }
}
