/*************** type.h file ************************/
typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc GD;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD *gp;
INODE *ip;
DIR *dp;

#define FREE 0
#define READY 1

#define BLKSIZE 1024
#define NMINODE 64
#define NFD 8
#define NPROC 2

typedef struct minode
{
  INODE INODE;
  int dev, ino;
  int refCount; // # of current references to it
  int dirty;  // means data was altered
  // for level-3
  int mounted;
  struct mntable *mptr;
} MINODE;

typedef struct oft
{ // for level-2
  int mode;
  int refCount;
  MINODE *mptr;
  int offset;
} OFT;

typedef struct proc
{
  struct proc *next;
  int pid;
  int uid;
  int gid;
  int status;
  MINODE *cwd;
  OFT *fd[NFD];
} PROC;

/*REFERENCE*/
/*
struct ext2_super_block 
{
  u32 s_inodes_count;  Inodes count 
  u32 s_blocks_count;  Blocks count 
  u32 s_r_blocks_count;  Reserved blocks count 
  u32 s_free_blocks_count;  Free blocks count 
  u32 s_free_inodes_count;  Free inodes count 
  u32 s_first_data_block;  First Data Block 
  u32 s_log_block_size;  Block size 
  u32 s_log_cluster_size;  Allocation cluster size 
  u32 s_blocks_per_group;  # Blocks per group 
  u32 s_clusters_per_group;  # Fragments per group 
  u32 s_inodes_per_group;  # Inodes per group 
  u32 s_mtime;  Mount time 
  u32 s_wtime;  Write time 
  u16 s_mnt_count;  Mount count
  s16 s_max_mnt_count;  Maximal mount count 
  u16 s_magic;  Magic signature 
  // more non-essential fields
  u16 s_inode_size;  size of inode structure 
}

struct ext2_inode 
{
  u16  i_mode;         // 16 bits = |tttt|ugs|rwx|rwx|rwx|  
  u16  i_uid;          // owner uid
  u32  i_size;         // file size in bytes  
  u32  i_atime;        // time fields in seconds
  u32  i_ctime;        // since 00:00:00,1-1-1970
  u32  i_mtime;
  u32  i_dtime;
  u16  i_gid;          // group ID
  u16  i_links_count;  // hard-link count
  u32  i_blocks;       // number of 512-byte sectors
  u32  i_flags;        // IGNORE
  u32  i_reserved1;    // IGNORE
  u32  i_block[15];    // See details below
  u32  i_pad[7];       // for inode size = 128 bytes
  }
*/