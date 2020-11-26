int mycreat(MINODE *pmip, char *name)
{
        // pmip points at the parent minode[] of "/a/b", name is a string "c")
    // allocate an inode and a disk block for the new directory;
    int ino = ialloc(dev);

    printf("ino is %d\n", ino);
    if (ino < 0)
    {
        printf("error getting ino\n");
        return -1;
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
    ip->i_mode = 0x81A4;                                // REG type and permissions
    ip->i_uid = running->uid;                           // Owner uid
    ip->i_gid = running->gid;                           // Group Id
    ip->i_size = 0;                                     // Size in bytes
    ip->i_links_count = 1;                              // // inc link count by 1 because of . and ..
    ip->i_mtime = time(0L);                             // set to current time
    ip->i_blocks = 0;                                   
    for (int i = 0; i < 15; ++i)                        // initialize the other blocks to 0
        ip->i_block[i] = 0;

    new_mip->dirty = 1; // mark minode dirty
    iput(new_mip);      // write INODE to disk

    // Finally, enter name ENTRY into parent's directory by
    enter_name(pmip, ino, name);
    return 1;
}

// creates a file from path and/or name
int create(char *path, char *second)
{
    printf("[inside create]\n");

    if (path[0] == 0)
    {
        printf("no path specified\n");
        return -1;
    }

    // parse dir and base from path to get parent & child name
    char buf[64] = "";
    char parent[64] = "";
    char child[64] = "";
    int pino = 0;
    strcpy(buf, path);
    strcpy(parent, dirname(buf));
    strcpy(buf, path);
    strcpy(child, basename(buf));
    if (child[0] == '.')
    {
        printf("invalid file name\n");
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
    mycreat(pmip, child);
    iput(pmip);

    return 1;
}
