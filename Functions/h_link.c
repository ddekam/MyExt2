// hard links first to second [dirs not allowed]
int mylink(char *first, char *second)
{
    printf("mylink: first=%s    second=%s\n", first, second);
    if (first[0] == 0 || second[0] == 0)
        return -1;
    char old[64] = "";
    char new[64] = "";
    strcpy(old, first);
    strcpy(new, second);
    int oino = getino(old);
    MINODE *omip = iget(dev, oino);
    INODE *oip = &omip->INODE;
    if (S_ISDIR(oip->i_mode))
    {
        printf("old file is a dir.. can't link\n");
        return -2;
    }
    // check if new file exists
    if (getino(new) > 0)
    {
        printf("%s already exists\n", new);
        return -3;
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
        return -4;
    }
    MINODE *pmip = iget(dev, pino);
    if (pmip == NULL)
    {
        printf("MINODE of %s doesn't exits\n", parent_path);
        return -5;
    }
    INODE *pip = &pmip->INODE;

    // create entry in new parent DIR with same inode number of old file
    enter_name(pmip, oino, new);
    // update inode data such as linkscount, dirty, and time edited
    oip->i_links_count++;
    omip->dirty = 1;
    iput(omip);
    pip->i_mtime = time(0L);
    pmip->dirty = 1;
    iput(pmip);

    return 1;
}
