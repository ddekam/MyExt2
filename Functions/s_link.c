// creates a symbolic link from first to second [dirs allowed]
int mysymlink(char *first, char *second)
{
    printf("symlink: first=%s   second=%s\n", first, second);
    if (first[0] == 0 || second[0] == 0)
        return -1;
    char old[64] = "";
    char new[64] = "";
    strcpy(old, first);
    strcpy(new, second);
    int oino = getino(old);
    if (oino <= 0)
    {
        printf("%s doesn't exits\n", old);
        return -1;
    }
    MINODE *omip = iget(dev, oino);
    INODE *oip = &omip->INODE;

    // check if new file exists
    if (getino(new) > 0)
    {
        printf("%s already exists\n", new);
        return -2;
    }

    // get parent and child of old file's path
    char parent_path[64] = "";
    char child_name[64] = "";
    char buf[BLKSIZE] = "";
    strcpy(buf, old);
    strcpy(parent_path, dirname(buf));
    strcpy(buf, old);
    strcpy(child_name, basename(buf));

    // get parent's ino and in-memory inode
    int pino = getino(parent_path);
    if (pino <= 0)
    {
        printf("ino of %s doesn't exits\n", parent_path);
        return -3;
    }
    MINODE *pmip = iget(dev, pino);
    if (pmip == NULL)
    {
        printf("MINODE of %s doesn't exits\n", parent_path);
        return -4;
    }
    INODE *pip = &pmip->INODE;

    // create a new file and change to LNK type
    mycreat(pmip, new);
    int lnk_ino = getino(new);
    if (lnk_ino <= 0)
    {
        printf("error getting created ino of %s\n", new);
        return -5;
    }
    MINODE *lnk_mip = iget(dev, lnk_ino);
    if (lnk_mip == NULL)
    {
        printf("error getting MINODE of %s\n", new);
        return -6;
    }
    // change mode to lnk and size to old filename
    INODE *lnk_ip = &lnk_mip->INODE;
    ip->i_mode = 0120000;
    ip->i_size = strlen(child_name);
    // put MINODES back to disk
    iput(lnk_mip);
    iput(pmip);
    return 1;
}
