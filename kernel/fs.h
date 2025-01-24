// On-disk file system format.
// Both the kernel and user programs use this header file.

#define ROOTINO 1  // root i-number
#define BSIZE 1024 // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
/*struct superblock {
  uint32 ninodes;               // Number of inodes
  uint32 nblocks;               // Number of data blocks
  uint size;                  // Size of file system image (blocks)
  uint nlog;                  // Number of log blocks
  uint logstart;              // Block number of first log block
  uint inodestart;            // Block number of first inode block
  uint bmapstart;             // Block number of first free map block
  uint reserved[7];
  uint16 magic;
  uint16 state;
  uint reserved_ext[241];
};*/

struct superblock
{
  uint ninodes;                  /* Inodes count */
  uint size;                     /* Blocks count */
  uint s_r_blocks_count;         /* Reserved blocks count */
  uint nblocks;                  /* Free blocks count */
  uint s_free_inodes_count;      /* Free inodes count */
  uint s_first_data_block;       /* First Data Block */
  uint s_log_block_size;         /* Block size */
  uint s_log_frag_size;          /* Fragment size */
  uint s_blocks_per_group;       /* # Blocks per group */
  uint s_frags_per_group;        /* # Fragments per group */
  uint s_inodes_per_group;       /* # Inodes per group */
  uint s_mtime;                  /* Mount time */
  uint s_wtime;                  /* Write time */
  ushort s_mnt_count;            /* Mount count */
  ushort s_max_mnt_count;        /* Maximal mount count */
  ushort magic;                  /* Magic signature */
  ushort s_state;                /* File system state */
  ushort s_errors;               /* Behaviour when detecting errors */
  ushort s_minor_rev_level;      /* minor revision level */
  uint s_lastcheck;              /* time of last check */
  uint s_checkinterval;          /* max. time between checks */
  uint s_creator_os;             /* OS */
  uint s_rev_level;              /* Revision level */
  ushort s_def_resuid;           /* Default uid for reserved blocks */
  ushort s_def_resgid;           /* Default gid for reserved blocks */
  uint inodestart;               /* First non-reserved inode */
  ushort s_inode_size;           /* size of inode structure */
  ushort s_block_group_nr;       /* block group # of this superblock */
  uint s_feature_compat;         /* compatible feature set */
  uint s_feature_incompat;       /* incompatible feature set */
  uint s_feature_ro_compat;      /* readonly-compatible feature set */
  uchar s_uuid[16];              /* 128-bit uuid for volume */
  char s_volume_name[16];        /* volume name */
  char s_last_mounted[64];       /* directory where last mounted */
  uint s_algorithm_usage_bitmap; /* For compression */
  /*
   * Performance hints.  Directory preallocation should only
   * happen if the EXT2_COMPAT_PREALLOC flag is on.
   */
  uchar s_prealloc_blocks;     /* Nr of blocks to try to preallocate*/
  uchar s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
  ushort s_padding1;
  /*
   * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
   */
  uchar s_journal_uuid[16]; /* uuid of journal superblock */
  uint s_journal_inum;      /* inode number of journal file */
  uint s_journal_dev;       /* device number of journal file */
  uint s_last_orphan;       /* start of list of inodes to delete */
  uint s_hash_seed[4];      /* HTREE hash seed */
  uchar s_def_hash_version; /* Default hash version to use */
  uchar s_reserved_char_pad;
  ushort s_reserved_word_pad;
  uint s_default_mount_opts;
  uint s_first_meta_bg; /* First metablock block group */
  uint s_reserved[190]; /* Padding to the end of the block */
};

struct ext2_group_desc
{
  uint bg_block_bitmap;        /* Blocks bitmap block */
  uint bg_inode_bitmap;        /* Inodes bitmap block */
  uint bg_inode_table;         /* Inodes table block */
  ushort bg_free_blocks_count; /* Free blocks count */
  ushort bg_free_inodes_count; /* Free inodes count */
  ushort bg_used_dirs_count;   /* Directories count */
  ushort bg_pad;
  uint bg_reserved[3];
};

#define FSMAGIC 0xEF53

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
/*struct dinode
{
  short type;              // File type
  short major;             // Major device number (T_DEVICE only)
  short minor;             // Minor device number (T_DEVICE only)
  short nlink;             // Number of links to inode in file system
  uint size;               // Size of file (bytes)
  uint addrs[NDIRECT + 1]; // Data block addresses
};*/

struct dinode
{
  ushort type;       // File mode (type and permissions)
  ushort i_uid;      // Owner UID
  uint size;         // Size in bytes
  uint i_atime;      // Access time
  uint i_ctime;      // Creation time
  uint i_mtime;      // Modification time
  uint i_dtime;      // Deletion time
  ushort i_gid;      // Group ID
  ushort nlink;      // Number of hard links
  uint i_blocks;     // Number of 512-byte blocks allocated
  uint i_flags;      // File flags
  uint addrs[15];    // Block pointers (12 direct, 1 single, 1 double, 1 triple)
  uint i_generation; // File version for NFS
  uint i_file_acl;   // File Access Control List
  uint i_dir_acl;    // Directory Access Control List
  uint i_faddr;      // Fragment address
  uchar i_osd1[12];  // OS-dependent fields (first part)
  uchar i_osd2[12];  // OS-dependent fields (second part)
  union
  {
    uint reserved[3]; // Reserved for future use
    struct
    {
      ushort major; // Major device number
      ushort minor; // Minor device number
    };
  };
};

// Inodes per block.
#define IPB (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, bgd) ((i) / IPB + bgd.bg_inode_table)

// Bitmap bits per block
#define BPB (BSIZE * 8)

// Block of free map containing bit for block b
#define BBLOCK(b, bgd) ((b) / BPB + (bgd).bg_block_bitmap)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent
{
  ushort inum;
  char name[DIRSIZ];
};

#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
