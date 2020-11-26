int rm_child(MINODE *pmip, char *name)
{
    printf("rm_child: %s\n", name);
    char buf[BLKSIZE] = "";
    char dname[64] = "";
    char *cp = NULL;
    char *cp_prior = NULL;
    char *cp_end = NULL;
    DIR *dp_prior = NULL;
    DIR *dp_end = NULL;
    INODE * pip = &pmip->INODE;
    // search every data block
    for (int i = 0; i < 12 && pip->i_block[i]; ++i)
    {
        get_block(dev, pip->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        // search each dir entry in the block for matching name
        while (cp < buf + BLKSIZE)
        {
            strncpy(dname, dp->name, dp->name_len);
            dname[dp->name_len] = '\0';

            if (strcmp(dname, name) == 0)   // name matches
            {
                if (dp->rec_len + cp == buf + BLKSIZE)  // hit at last entry
                {
                    printf("removing last dir entry\n");
                    dp_prior->rec_len += dp->rec_len;   // write over unwanted dir's memory
                    put_block(dev, pip->i_block[i], buf);   // write change to disk
                }
                else if (cp == buf) // hit on first entry
                {
                    printf("match on first dir entry.. removing block\n");
                    // deallocate the data block
                    bdalloc(dev, pip->i_block[i]);
                    pip->i_size -= BLKSIZE;
                    // move blocks down to fill hole
                    for (; i < 11 && pip->i_block[i + 1]; ++i)
                    {
                        get_block(dev, pip->i_block[i + 1], buf);
                        put_block(dev, pip->i_block[i], buf);
                    }
                }
                else    // hit in middle of block
                {
                    printf("match at middle dir entry\n");
                    // we must get dp_end and cp_end pointing to the last dir entry
                    cp_end = buf;
                    dp_end = (DIR *)buf;
                    while ((cp_end + dp_end->rec_len) < buf + BLKSIZE)
                    {
                        cp_end += dp_end->rec_len;
                        dp_end = (DIR *)cp_end;
                    }

                    //extend the end dp
                    dp_end->rec_len += dp->rec_len;
                    // move the unwanted dir entry to the end
                    int begin_address = (int)cp + dp->rec_len;
                    const void *baddress = (const void *)begin_address;
                    int end_address = (int)buf + BLKSIZE;
                    // this is the difference from last address of dp and the
                    // last address of the block
                    size_t size = (size_t)(end_address - begin_address);
                    printf("moving %d bytes from %p to the location %p\n", 
                        (int)size, baddress, (const void *)cp);
                    // this moves the chunk after the dir entry to the beginning of the
                    // dir entry's address.. overwriting the unwanted dir entry
                    memmove((void *)cp, baddress, size);
                    // write it back to disk
                    put_block(dev, pip->i_block[i], buf);
                }
                pip->i_links_count--;
                pip->i_mtime = time(0L);
                pmip->dirty = 1;
                iput(pmip);
                return 1;
            }
            
            cp_prior = cp;
            dp_prior = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    printf("rmdir child name not found\n");
    return -1;
}

int rm_dir(char *path, char *second)
{
    printf("[inside rm_dir]\n");
    if (path[0] == 0)
    {
        printf("no path specified\n");
        return -1;
    }

    // parse dir and base from path to get parent & child name
    char buf[BLKSIZE] = "";
    char temp[64] = "";
    char parent[64] = "";
    char child[64] = "";
    char *cp = NULL;
    char dname[64] = "";
    int ino = -1;
    strcpy(temp, path);
    strcpy(parent, dirname(temp));
    strcpy(temp, path);
    strcpy(child, basename(temp));
    
    ino = getino(path);

    if (ino < 0)
    {
        printf("no child ino\n");
        return -2;
    }

    MINODE *mip = iget(dev, ino);

    // verify the child exists
    if (mip == NULL)
    {
        printf("child doesn't exist\n");
        return -2;
    }

    // verify child is dir
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("child is not a directory\n");
        return -3;
    }

    if (mip->refCount > 1)
    {
        printf("child is busy\n");
        return -4;
    }

    ip = &mip->INODE;
    // check if not empty
    if (ip->i_links_count > 2)
    {
        printf("child is not empty\n");
        return -5;
    }
    // there may still be contents if link count = 2
    if (ip->i_links_count == 2)
    {
        // search through dir entries in each data block to 
        // see if other than "." and ".."
        for (int i = 0; i < 12; ++i)
        {
            if (!ip->i_block[i])
                break;
            get_block(dev, ip->i_block[i], buf);
            cp = buf;
            dp = (DIR *)buf;
            while (cp < buf + BLKSIZE)
            {
                strncpy(dname, dp->name, dp->name_len);
                dname[dp->name_len] = '\0';
                // if name = "." or ".."
                if (dname[0] != '.')
                {
                    printf("dir is not empty\n");
                    return -5;
                }
                cp += dp->rec_len;
                dp = (DIR *)cp;
            }
        }
    }

    // at this point the child dir is empty and can be removed
    // call rm_child passing in parent's MINODE and child's name
    rm_child(iget(dev, getino(parent)), child);
    iput(mip);

    return 1;
}
