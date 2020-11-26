
dd if=/dev/zero of=disk bs=1024 count=1440

echo "#     [Creating Ext2 Partition]"
mke2fs -b 1024 disk 1440

echo "#     [Mounting Disk]"
sudo mount -o loop disk /mnt

echo "#     [Adding files and directories to disk]"
yes | (cd /mnt; rm -r *; mkdir dir1 dir2; mkdir dir1/dir3; touch file1 file2; ls -l)

echo "#     [Unmounting Disk]"
sudo umount /mnt

echo "#     [Removing existing binary from last compile]"
rm launchshell.elf 2> /dev/null

echo "#     [Compiling main.c]"
cc -m32 -g -o launchshell.elf main.c

echo "#     [launching shell]..."
sudo ./launchshell.elf disk developermode
