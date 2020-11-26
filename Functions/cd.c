// cd into a directory
int change_dir(char *path, char *second)
{
    printf("[inside change_dir]\n");
    int ino = -1;
    MINODE *mip = NULL;

    if (path[0] == 0)
    {
        printf("no directory specified");
        return -1;
    }

    ino = getino(path);
    mip = iget(dev, ino);

    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("%s is not a directory\n", path);
        return -2;
    }
    iput(running->cwd);
    running->cwd = mip;
    iput(mip);
    return 1;
}