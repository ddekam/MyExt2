// recursive path to root helper function
int rPathToRoot(MINODE *wd)
{
    char buf[BLKSIZE] = "";
    char myname[256] = "";
    char rootStr[256] = "/";
    if (wd == root)
    {
        strcat(fullPath, rootStr);
        return 0;
    }
    ip = &wd->INODE;
    get_block(dev, ip->i_block[0], buf);
    dp = (DIR *)buf;
    char *cp = buf;
    int myino = dp->inode;
    cp += dp->rec_len;
    dp = (DIR *)cp;
    int parentino = dp->inode;
    MINODE *pmip = iget(dev, parentino);
    getMyName(pmip, myino, myname);
    if (myname[0])
    {
        rPathToRoot(pmip);
        if (strcmp(fullPath, "/") != 0) // path is not root
            strcat(fullPath, "/");

        strcat(fullPath, myname);
    }
    iput(pmip);
    return 1;
}

int pathToRoot(MINODE *wd)
{
    strcpy(fullPath, "");
    rPathToRoot(wd);
    return 1;
}

// print path from root to cwd
int pwd(char *path, char *second)
{
    MINODE *wd = running->cwd;
    pathToRoot(wd);
    printf("pwd: %s\n", fullPath);
    return 1;
}
