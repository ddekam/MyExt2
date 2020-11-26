/* decrements the file’s links_count by 1 and deletes the file name from its parent DIR.
    When a file’s links_count reaches 0, the file is truly removed 
    by deallocating its data blocks and inode.*/
int myunlink(char *path, char *second)
{   
    //*******check edge cases*******//

    if (path[0] == 0)
    {
        printf("no path specified\n");
        return -1;
    }
    // get child inode
    int ino = getino(path);
    if (ino <= 0)
    {
        printf("bad path.. no ino\n");
        return -2;
    }
    // get child in memory inode
    MINODE *mip = iget(dev, ino);
    if (mip == NULL)
    {
        printf("error getting in memory inode\n");
        return -3;
    }
    
    ip = &mip->INODE;
    if (!S_ISREG(ip->i_mode) && !S_ISLNK(ip->i_mode))
    {
        printf("%s not reg or link.. can't unlink\n", path);
        return -4;
    }

    //******* begin unlinking process*******//
    //decrement links count
    ip->i_links_count--;
    if (ip->i_links_count == 0)
    {
        // this means inode is empty so deallocate it and it's blocks
        mytruncate(ip);
        // deallocate inode
        idalloc(dev, ino);
    }

    // get parent path and child name
    char parent[64] = "";
    char child[64] = "";
    char buf[BLKSIZE] = "";
    strcpy(buf, path);
    strcpy(parent, dirname(buf));
    strcpy(buf, path);
    strcpy(child, basename(buf));

    // get parent ino and in memory inode
    int pino = getino(parent);
    if(pino <= 0)
    {
        printf("error getting parent ino\n");
        return -5;
    }
    MINODE *pmip = iget(dev, pino);
    if(pmip == NULL)
    {
        printf("error getting parent minode\n");
        return -6;
    }
    // remove the child name from parent dir
    rm_child(pmip, child);
    // pmip is marked dirty and iput in rm_child

    // mark child minode dirty and put back to disk
    mip->dirty = 1;
    iput(mip);

    return 1;
}
