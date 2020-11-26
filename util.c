/*********** util.c file ****************/
/**** globals defined in main.c file ****/

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;

extern char gpath[256];
extern char *names[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd_entered[32], pathname[256];

// reads i_block into memory
void get_block(int dev, int blk, char *buf)
{
  lseek(dev, blk * BLKSIZE, SEEK_SET);
  n = read(dev, buf, BLKSIZE);
  if (n < 0)
    printf("get_block [%d %d] error\n", dev, blk);
}

// writes i_block to device
void put_block(int dev, int blk, char *buf)
{
  lseek(dev, blk * BLKSIZE, SEEK_SET);
  n = write(dev, buf, BLKSIZE);
  if (n != BLKSIZE)
    printf("put_block [%d %d] error\n", dev, blk);
}

// tokenizes a pathname into char *names[]
void tokenize(char *pathname)
{
  if (!pathname)
    return;
  char gpath[256];
  int i;
  int n;
  char *s;

  strcpy(gpath, pathname);
  n = 0;
  s = strtok(gpath, "/");

  while (s)
  {
    names[n] = malloc(256);
    strcpy(names[n++], s); // OR  names[n++] = s;
    s = strtok(0, "/");
  }
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, disp;
  INODE *ip;

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];

    if (mip->refCount && mip->dev == dev && mip->ino == ino)
    {
      mip->refCount++;
      printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
      return mip;
    }
  }

  for (i = 0; i < NMINODE; i++)
  {
    mip = &minode[i];
    if (mip->refCount == 0)
    {
      printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
      mip->refCount = 1;
      mip->dev = dev;
      mip->ino = ino;

      // get INODE of ino to buf
      blk = (ino - 1) / 8 + inode_start;
      disp = (ino - 1) % 8;
      printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);
      get_block(dev, blk, buf);
      ip = (INODE *)buf + disp;
      // copy INODE to mp->INODE
      mip->INODE = *ip;

      return mip;
    }
  }
  printf("PANIC: no more free minodes\n");
  return NULL;
}

// writes an MINODE to device if refCount = 0
void iput(MINODE *mip)
{
  int i, block, offset;
  char buf[BLKSIZE];
  INODE *ip;

  if (mip == 0)
    return;

  mip->refCount--;

  if (mip->refCount > 0)
    return;

  if (!mip->dirty)
    return;

  /* write back */
  //  printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

  block = ((mip->ino - 1) / 8) + inode_start;
  offset = (mip->ino - 1) % 8;

  /* first get the block containing this inode */
  get_block(mip->dev, block, buf);

  ip = (INODE *)buf + offset;
  *ip = mip->INODE;

  put_block(mip->dev, block, buf);
}

// returns inode matching name, 0 if don't exist
int search(MINODE *mip, char *name)
{
  if (!name)
    return 0;
  char wantedName[64] = "";
  strcpy(wantedName, name); // copy into wantedName because name turns into 
    //null when calling get_block
  printf("[Searching for %s]\n", wantedName);
  printf("inumber   ");
  printf("rec_len  ");
  printf("name_len  ");
  printf("name\n");

  char sbuf[BLKSIZE], temp[256];
  char *cp;
  int i;
  ip = &mip->INODE;

  for (i = 0; i < 12; ++i) // assume DIR at most 12 direct blocks
  {
    if (ip->i_block[i] == 0)
      break;
    printf("i_block: %d\n", ip->i_block[i]);
    get_block(dev, ip->i_block[i], sbuf);

    dp = (DIR *)sbuf;
    cp = sbuf;

    while (cp < sbuf + BLKSIZE)
    {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%4d\t   %4d\t   %4d\t      %s\n",
             dp->inode, dp->rec_len, dp->name_len, temp);
      if (!strcmp(wantedName, temp))
        return dp->inode;
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  return 0;
}

// returns inode at path
int getino(char *pathname)
{
  int i, ino = -1, blk, disp;
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/") == 0)
    return 2;

  if (pathname[0] == '/')
    mip = iget(dev, 2);
  else
    mip = iget(running->cwd->dev, running->cwd->ino);

  tokenize(pathname);

  for (i = 0; i < n; i++)
  {
    printf("===========================================\n");
    if (!names[i])
      break;
    ino = search(mip, names[i]);

    if (ino == 0)
    {
      iput(mip);
      printf("name %s does not exist\n", names[i]);
      return 0;
    }
    iput(mip);
    mip = iget(dev, ino);
  }
  iput(mip);
  return ino;
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;
  j = bit % 8;
  if (buf[i] & (1 << j))
    return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;
  j = bit % 8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;
  j = bit % 8;
  buf[i] &= ~(1 << j);
}

// increment free inodes count in sp and gp
int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

// decrement free inodes count in sp and gp
int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

// allocate an inode number
int ialloc(int dev)
{
  int i = 0;
  char buf[BLKSIZE] = "";

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i = 0; i < ninodes; i++)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf, i);
      put_block(dev, imap, buf);
      decFreeInodes(dev);
      return i + 1;
    }
  }
  return 0;
}

// deallocate an ino number
int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];

  if (ino > ninodes)
  {
    printf("inumber %d out of range\n", ino);
    return -1;
  }

  // get inode bitmap block
  get_block(dev, bmap, buf);
  clr_bit(buf, ino - 1);
  // write buf back
  put_block(dev, imap, buf);
  // update free inode count in SUPER and GD
  incFreeInodes(dev);
  return 1;
}

// returns a FREE disk block number
// 0 if not found
int balloc(int dev)
{
  printf("[balloc] bmap: %d  nblocks:%d\n", bmap, nblocks);

  char buf[BLKSIZE] = "";

  // read block_bitmap
  get_block(dev, bmap, buf);

  for (int i = 0; i < nblocks; ++i)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf, i);
      put_block(dev, bmap, buf);
      decFreeInodes(dev);
      return i;
    }
  }
  return 0;
}

// deallocate a blk number
int bdalloc(int dev, int blk)
{
  int i;
  char buf[BLKSIZE];

  if (blk > nblocks)
  {
    printf("bnumber %d out of range\n", blk);
    return -1;
  }
  // get inode bitmap block
  get_block(dev, bmap, buf);
  clr_bit(buf, blk - 1);
  // write buf back
  put_block(dev, bmap, buf);
  return 1;
}

// copies name of dir to myname by passing it's inode and parent MINODE
int getMyName(MINODE *pmip, int myino, char *myname)
{
  char sbuf[BLKSIZE] = "";
  char *cp = NULL;
  // check if root
  if (myino == root->ino)
  {
    strcpy(myname, "/");
    return 1;
  }
  // get parent's inode
  INODE *pip = &pmip->INODE;

  for (int i = 0; i < 12; ++i) // assume only direct blocks
  {
    if (pip->i_block[i] == 0)
      break; // reached empty block
    get_block(dev, pip->i_block[i], sbuf);
    cp = sbuf;
    dp = (DIR *)cp;

    while (cp < sbuf + BLKSIZE)
    {
      if (dp->inode == myino)
      {
        strncpy(myname, dp->name, dp->name_len);
        myname[dp->name_len] = '\0';
        return 1;
      }
      else
      {
        cp += dp->rec_len;
        dp = (DIR *)cp;
      }
    }
  }
  printf("[getMyName] name not found\n");
  return 0;
}

int mytruncate(INODE *inode)
{
  // deallocate every existing block
  for (int i = 0; i < 12 && inode->i_block[i]; ++i)
      bdalloc(dev, inode->i_block[i]);
  // need to add more to acount for indirect blocks
  return 1;
}

int enter_name(MINODE *pmip, int myino, char *myname)
{
    INODE *pip = &pmip->INODE;
    int parent_blk_size = pip->i_size / BLKSIZE;
    int i = 0, ideal_length = -1, blk = -1, need_length = -1, remain = -1;
    char *cp = NULL;
    char buf[BLKSIZE] = "";

    //  For each data block of parent DIR do { // assume: only 12 direct blocks
    for (i = 0; i < parent_blk_size; ++i)
    {
        if (pip->i_block[i] == 0)
            break;
        // get parent's ith data block into a buf[ ]
        get_block(dev, pip->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        blk = pip->i_block[i];
        // Step to the last entry in a data block
        printf("step to LAST entry in data block %d\n", blk);
        printf("dir_names in block: ");
        while ((cp + dp->rec_len) < (buf + BLKSIZE))
        {
            printf("%s\t", dp->name);
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("\n");

        int need_length = 4 * ((8 + strlen(myname) + 3) / 4);
        ideal_length = 4 * ((8 + dp->name_len + 3) / 4);

        //      Let remain = LAST entry's rec_len - its IDEAL_LENGTH;
        remain = dp->rec_len - ideal_length;
        printf("remain = %d\n", remain);

        if (remain >= need_length)
        {
            printf("[space exists]\n");
            // enter the new entry as the LAST entry and trim the previous entry
            // to its IDEAL_LENGTH;
            dp->rec_len = ideal_length;
            cp += ideal_length;
            dp = (DIR *)cp;

            // set properties of dp and enter the name
            dp->rec_len = BLKSIZE - ((int)cp - (int)buf);
            dp->inode = myino;
            dp->name_len = strlen(myname);
            strcpy(dp->name, myname);
            dp->file_type = EXT2_FT_DIR;
            // Write data block to disk;
            put_block(dev, pip->i_block[i], buf);

            return 1;
        }
    }

    printf("[allocating new disk block]\n");
    pip->i_block[i] = balloc(dev);
    pmip->dirty = 1;
    pip->i_size = BLKSIZE;

    get_block(dev, pip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    dp->file_type = EXT2_FT_DIR;
    dp->inode = myino;
    dp->rec_len = BLKSIZE;
    dp->name_len = strlen(myname);
    strcpy(dp->name, myname);

    // Write data block to disk;
    put_block(dev, pip->i_block[i], buf);

    return 1;
}
