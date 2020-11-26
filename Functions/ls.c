// provides stat information on file name
int ls_file(int ino, char *name)
{
    MINODE *mip = iget(dev, ino);
    INODE *ip = &(mip->INODE);

    printf("    ");

    if (S_ISREG(ip->i_mode))
        printf("%c", '-');
    if (S_ISDIR(ip->i_mode))
        printf("%c", 'd');
    if (S_ISLNK(ip->i_mode))
        printf("%c", 'l');
    
    for (int i = 8; i >= 0; i--)
    {
        if (ip->i_mode & (1 << i)) // print r|w|x
            printf("%c", t1[i]);
        else
            printf("-"); // or print -
    }
    printf("\t%d\t%d\t", ip->i_uid, ip->i_size);
    char *ftime = ctime((const time_t *)(&ip->i_mtime));
    ftime[strlen(ftime) - 1] = 0; // kill \n at end
    printf("%s\t", ftime);

    iput(mip);
    return 1;
}

/*Gets the inode of the path, gets minode of inode,
gets minode's inode's block into buff, steps through
each dir entry calling ls_file each time*/
int ls_dir(char *path, char *second)
{
    printf("[inside ls_dir]\n");
    char buff[BLKSIZE] = "";
    char *cp = NULL;
    int ino = -1;
    int i = 0;
    MINODE *mip = NULL;

    if (path[0] == 0)
        ino = running->cwd->ino;
    else if (!strcmp(path, "/"))
        ino = root->ino;
    else
        ino = getino(path);

    mip = iget(dev, ino);

    if (ino < 0)
    {
        printf("path does not exist\n");
        return -1;
    }
    else
    {
        // for each dir entry, call ls_file
        for (i = 0; i < 12; i++) // assume DIR at most 12 direct blocks
        {
            if (mip->INODE.i_block[i] == 0)
                break;
            printf("i_block: %d\n", mip->INODE.i_block[i]);
            get_block(dev, mip->INODE.i_block[i], buff);

            dp = (DIR *)buff;
            cp = buff;

            while (cp < buff + BLKSIZE)
            {
                ls_file(dp->inode, dp->name);

                // print name
                for (int j = 0; j < dp->name_len; j++)
                    printf("%c", dp->name[j]);
                printf("\n");

                cp += dp->rec_len;
                dp = (DIR *)cp;
            }
        }
    }

    return 1;
}
