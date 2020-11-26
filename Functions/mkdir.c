// makes directory from parent MINODE and child name
int mymkdir(MINODE *pmip, char *name)
{
    // pmip points at the parent minode[] of "/a/b", name is a string "c")
    // allocate an inode and a disk block for the new directory;
    int ino = ialloc(dev);
    int bno = balloc(dev);

    printf("ino is %d, and bno is %d\n", ino, bno);
    if (ino < 0)
    {
        printf("error getting ino\n");
        return -1;
    }

    if (bno < 0)
    {
        printf("error getting bno\n");
        return -2;
    }

    // load the inode into a minode[] (in order to
    // write contents to the INODE in memory.
    MINODE *new_mip = iget(dev, ino);

    if (new_mip == NULL)
    {
        printf("error getting mip\n");
        return -3;
    }
    // Write contents to mip->INODE to make it as a DIR INODE.
    INODE *ip = &new_mip->INODE;
    ip->i_mode = 0x41ED;                                // OR 040755: DIR type and permissions
    ip->i_uid = running->uid;                           // Owner uid
    ip->i_gid = running->gid;                           // Group Id
    ip->i_size = BLKSIZE;                               // Size in bytes
    ip->i_links_count++;                                // Inc link count by 1 because of . and ..
    ip->i_mtime = time(0L);                             // set to current time
    ip->i_blocks = 2;                                   // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = bno;                               // new DIR has one data block
    for (int i = 1; i < 15; ++i)                        // initialize the other blocks to 0
        ip->i_block[i] = 0;

    new_mip->dirty = 1; // mark minode dirty
    iput(new_mip);      // write INODE to disk

    // create data block for new DIR containing . and .. entries
    char buf[BLKSIZE] = "";
    get_block(dev, bno, buf);

    // set child properties
    dp = (DIR *)buf;
    dp->inode = ino;
    dp->file_type = (char)EXT2_FT_DIR;
    dp->name_len = strlen(".");
    dp->name[0] = '.';
    dp->name[1] = '\0';
    dp->rec_len = 12;

    // set parent properties
    char *cp = buf + 12;
    dp = (DIR *)cp;
    dp->file_type = (char)EXT2_FT_DIR;
    dp->inode = pmip->ino;
    dp->name_len = strlen("..");
    dp->rec_len = 1012;
    dp->name[0] = '.';
    dp->name[1] = '.';
    dp->name[2] = '\0';

    // Then, write buf[ ] to the disk block bno;
    put_block(dev, bno, buf);
    // Finally, enter name ENTRY into parent's directory by
    enter_name(pmip, ino, name);
    return 1;
}

// creates a directory from the path and/or name
int make_dir(char *path, char *second)
{
    printf("[inside make_dir]\n");

    if (path[0] == 0)
    {
        printf("no path specified\n");
        return -1;
    }

    // parse dir and base from path to get parent & child name
    char buf[64] = "";
    char parent[64] = "";
    char child[64] = "";
    int pino = -1;
    strcpy(buf, path);
    strcpy(parent, dirname(buf));
    strcpy(buf, path);
    strcpy(child, basename(buf));
    // this is so we can check for '.' as first char for
    // the name in rmdir and be sure it's not a real dir
    if (child[0] == '.')
    {
        printf("invalid dir name\n");
        return -1;
    }
    
    if (strcmp(parent, ".") == 0)
        pino = running->cwd->ino;
    else
        pino = getino(parent);

    if (pino < 0)
    {
        printf("no parent ino\n");
        return -2;
    }

    MINODE *pmip = iget(dev, pino);

    // verify the parent exists
    if (pmip == NULL)
    {
        printf("parent doesn't exist\n");
        return -2;
    }

    // verify parent is root or dir
    if (pmip == root)
        printf("parent is root\n");

    else if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("parent is not a directory\n");
        return -3;
    }

    // verify child with same name doesn't exist
    if (getino(path) > 0)
    {
        printf("child already exits\n");
        return -4;
    }
    // call mymkdir(pip, child)
    mymkdir(pmip, child);
    iput(pmip);

    return 1;
}
