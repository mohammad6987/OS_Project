// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
// there should be one superblock per disk device, but we run with
// only one device
struct superblock sb;

struct ext2_group_desc bgd;

struct dinode rootInode;

// Read the super block.
static void
readsb(int dev, struct superblock *sb)
{
  struct buf *bp;
  bp = bread(dev, 1);
  memmove(sb, bp->data, 1024);

  brelse(bp);
}

void readbgd(uint dev, struct ext2_group_desc *bgd)
{
  struct buf *bp;
  uint block = 2;
  bp = bread(dev, block);
  memmove(bgd, bp->data, sizeof(struct ext2_group_desc));
  brelse(bp);
}

void writebgd(uint dev, struct ext2_group_desc *bgd)
{
  struct buf *bp;
  uint block = 2;
  bp = bread(dev, block);
  memmove(bp->data, bgd, sizeof(struct ext2_group_desc));
  log_write(bp);
  brelse(bp);
}

void readRootInode(uint dev, struct dinode *rootInode)
{
  uint inode_num = 17;
  uint inode_table_start = bgd.bg_inode_table;
  uint inode_offset = (inode_num - 1) * sizeof(&rootInode);

  uint inode_block = inode_table_start + inode_offset / BSIZE;

  struct buf *bp = bread(dev, inode_block);

  uint inode_in_block_offset = inode_offset % BSIZE;

  memmove(rootInode, bp->data + inode_in_block_offset, sizeof(struct dinode));

  brelse(bp);
}

// Init fs
// Init fs
void fsinit(int dev)
{
  readsb(dev, &sb); // Read the superblock from the device
                    /* printf("Superblock contents:\n");
                     printf("Magic: 0x%x\n", sb.magic);
                     printf("File system size (blocks): %u\n", sb.size);
                     printf("Number of data blocks: %d\n", sb.nblocks);
                     printf("Number of inodes: %d\n", sb.ninodes);
                     printf("Inode start block: %u\n", sb.inodestart);
                   */
  if (sb.magic != FSMAGIC)
    panic("Invalid file system");

  printf("size of superblock: %ld\n", sizeof(sb));

  readbgd(dev, &bgd);

  printf("Block Group Descriptor contents:\n");
  printf("Block bitmap block: %u\n", bgd.bg_block_bitmap);
  printf("Inode bitmap block: %u\n", bgd.bg_inode_bitmap);
  printf("Inode table block: %u\n", bgd.bg_inode_table);
  printf("Free blocks count: %u\n", bgd.bg_free_blocks_count);
  printf("Free inodes count: %u\n", bgd.bg_free_inodes_count);
  printf("Used directories count: %u\n", bgd.bg_used_dirs_count);
  /*
    readRootInode(dev, &rootInode);
    printf("inode size : %ld", sizeof(rootInode));
    printf("Root inode contents:\n");
    printf("Inode type: 0x%x\n", rootInode.type);
    printf("Owner UID: %u\n", rootInode.i_uid);
    printf("Size: %u\n", rootInode.size);
    printf("Access time (i_atime): %u\n", rootInode.i_atime);
    printf("Creation time (i_ctime): %u\n", rootInode.i_ctime);
    printf("Modification time (i_mtime): %u\n", rootInode.i_mtime);
    printf("Deletion time (i_dtime): %u\n", rootInode.i_dtime);
    printf("Group ID (i_gid): %u\n", rootInode.i_gid);
    printf("Number of hard links (nlink): %u\n", rootInode.nlink);
    printf("Number of 512-byte blocks allocated (i_blocks): %u\n", rootInode.i_blocks);
    printf("File flags (i_flags): %u\n", rootInode.i_flags);

    printf("Block addresses:\n");
    for (int i = 0; i < 15; i++)
    { // There are 15 block addresses (12 direct, 3 indirect)
      printf("Address[%d]: %u\n", i, rootInode.addrs[i]);
    }
  */
  initlog(dev, &sb);
}

// Zero a block.
/*
static void
bzero(int dev, int bno)
{
  struct buf *bp;

  bp = bread(dev, bno);
  memset(bp->data, 0, BSIZE);
  log_write(bp);
  brelse(bp);
}
*/
// Blocks.

// Allocate a zeroed disk block.
// returns 0 if out of disk space.
/*
static uint
ext2fs_free_block(char *bitmap)
{
  int i, j, mask;
  for (i = 0; i < sb.s_blocks_per_group * 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      mask = 1 << (7 - j);
      if ((bitmap[i] & mask) == 0)
      {
        bitmap[i] |= mask;
        return i * 8 + j;
      }
    }
  }
  return -1;
}
*/
uint balloc(uint dev)
{
  struct buf *bp;
  uint block;

  bp = bread(dev, 10); // Read the block bitmap (Block 10)
  uchar *bitmap = (uchar *)bp->data;

  for (block = 0; block < 500; block++)
  {
    if ((bitmap[block / 8] & (1 << (block % 8))) == 0)
    {                                          // Check if bit is free
      bitmap[block / 8] |= (1 << (block % 8)); // Mark it as used
      log_write(bp);                           // Write back changes
      brelse(bp);
      return block;
    }
  }

  brelse(bp);
  return 0; // No free blocks
}

// Free a disk block.
static void
bfree(int dev, uint b)
{
  struct buf *bp;
  int bi, m;

  bp = bread(dev, BBLOCK(b, bgd));
  bi = b % BPB;
  m = 1 << (bi % 8);
  if ((bp->data[bi / 8] & m) == 0)
    panic("freeing free block");
  bp->data[bi / 8] &= ~m;
  log_write(bp);
  brelse(bp);
}

// Inodes.
//
// An inode describes a single unnamed file.
// The inode disk structure holds metadata: the file's type,
// its size, the number of links referring to it, and the
// list of blocks holding the file's content.
//
// The inodes are laid out sequentially on disk at block
// sb.inodestart. Each inode has a number, indicating its
// position on the disk.
//
// The kernel keeps a table of in-use inodes in memory
// to provide a place for synchronizing access
// to inodes used by multiple processes. The in-memory
// inodes include book-keeping information that is
// not stored on disk: ip->ref and ip->valid.
//
// An inode and its in-memory representation go through a
// sequence of states before they can be used by the
// rest of the file system code.
//
// * Allocation: an inode is allocated if its type (on disk)
//   is non-zero. ialloc() allocates, and iput() frees if
//   the reference and link counts have fallen to zero.
//
// * Referencing in table: an entry in the inode table
//   is free if ip->ref is zero. Otherwise ip->ref tracks
//   the number of in-memory pointers to the entry (open
//   files and current directories). iget() finds or
//   creates a table entry and increments its ref; iput()
//   decrements ref.
//
// * Valid: the information (type, size, &c) in an inode
//   table entry is only correct when ip->valid is 1.
//   ilock() reads the inode from
//   the disk and sets ip->valid, while iput() clears
//   ip->valid if ip->ref has fallen to zero.
//
// * Locked: file system code may only examine and modify
//   the information in an inode and its content if it
//   has first locked the inode.
//
// Thus a typical sequence is:
//   ip = iget(dev, inum)
//   ilock(ip)
//   ... examine and modify ip->xxx ...
//   iunlock(ip)
//   iput(ip)
//
// ilock() is separate from iget() so that system calls can
// get a long-term reference to an inode (as for an open file)
// and only lock it for short periods (e.g., in read()).
// The separation also helps avoid deadlock and races during
// pathname lookup. iget() increments ip->ref so that the inode
// stays in the table and pointers to it remain valid.
//
// Many internal file system functions expect the caller to
// have locked the inodes involved; this lets callers create
// multi-step atomic operations.
//
// The itable.lock spin-lock protects the allocation of itable
// entries. Since ip->ref indicates whether an entry is free,
// and ip->dev and ip->inum indicate which i-node an entry
// holds, one must hold itable.lock while using any of those fields.
//
// An ip->lock sleep-lock protects all ip-> fields other than ref,
// dev, and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

struct
{
  struct spinlock lock;
  struct inode inode[NINODE];
} itable;

void iinit()
{
  int i = 0;

  initlock(&itable.lock, "itable");
  for (i = 0; i < NINODE; i++)
  {
    initsleeplock(&itable.inode[i].lock, "inode");
  }
}

static struct inode *iget(uint dev, uint inum);

// Allocate an inode on device dev.
// Mark it as allocated by  giving it type type.
// Returns an unlocked but allocated and referenced inode,
// or NULL if there is no free inode.
struct inode *
ialloc(uint dev, short type)
{
  int inum;
  struct buf *bp;
  // struct dinode *dip;
  struct inode *ip;
  int bit_index;

  // Read the inode bitmap from block 11
  bp = bread(dev, 11);

  // Search for the first free inode in the inode bitmap
  for (bit_index = 0; bit_index < sb.ninodes; bit_index++)
  {
    // Check if the inode is free (bit is 0 in the inode bitmap)
    if (((char *)bp->data)[bit_index / 8] & (1 << (bit_index % 8)))
    {
      // Inode is already allocated, skip
      continue;
    }

    // Found a free inode, mark it as allocated (set the bit)
    ((char *)bp->data)[bit_index / 8] |= (1 << (bit_index % 8));

    // Write the updated inode bitmap back to disk
    log_write(bp);

    // Calculate the inode number (assuming inode numbers start at 1)
    inum = bit_index + 1;

    // Get the inode from the inode table (Block 12)
    ip = iget(dev, inum);

    // Set the inode type and initialize other fields
    ip->type = type;
    memset(&ip->addrs, 0, sizeof(ip->addrs)); // Clear inode block addresses (if necessary)
    ip->nlink = 1;                            // Initialize link count (assuming new file or directory)
    ip->size = 0;                             // Initialize size (new inode)

    brelse(bp); // Release the buffer after usage

    return ip; // Return the allocated inode
  }

  brelse(bp); // Release the buffer if no free inode found
  printf("ialloc: no free inodes\n");
  return 0; // Return null if no free inode was found
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an ip->xxx field
// that lives on disk.
// Caller must hold ip->lock.
void iupdate(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;

  bp = bread(ip->dev, IBLOCK(ip->inum, bgd));
  dip = (struct dinode *)bp->data + ip->inum % IPB;
  dip->type = ip->type;
  dip->major = ip->major;
  dip->minor = ip->minor;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
  printf("inside iupdate , ninide type = %x , major = %x , size = %d\n", ip->type, ip->major, ip->size);
  for (int i = 1; i < 15; i++)
  {
     dip->addrs[i] = ip->addrs[i - 1] ;
  }
  log_write(bp);
  brelse(bp);
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode *
iget(uint dev, uint inum)
{
  struct inode *ip, *empty;

  acquire(&itable.lock);

  // Is the inode already in the table?
  empty = 0;
  for (ip = &itable.inode[0]; ip < &itable.inode[NINODE]; ip++)
  {
    if (ip->ref > 0 && ip->dev == dev && ip->inum == inum)
    {
      ip->ref++;
      release(&itable.lock);
      return ip;
    }
    if (empty == 0 && ip->ref == 0) // Remember empty slot.
      empty = ip;
  }
  // Recycle an inode entry.
  if (empty == 0)
    panic("iget: no inodes");

  ip = empty;
  ip->dev = dev;
  ip->inum = inum;
  ip->ref = 1;
  ip->valid = 0;
  release(&itable.lock);
  printf("Inode fields(in ilock):\n");
  printf("  Type: %x\n", ip->type);
  printf("  Major: %d\n", ip->major);
  printf("  Minor: %d\n", ip->minor);
  printf("  Nlink: %d\n", ip->nlink);
  printf("  Size: %d\n", ip->size);
  printf("  Block addresses: ");
  for (int i = 1; i < 14; i++) // Assuming there are 15 addresses in the inode structure
  {
    printf("%x ", ip->addrs[i]);
  }
  printf("\n");

  return ip;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode *
idup(struct inode *ip)
{
  acquire(&itable.lock);
  ip->ref++;
  release(&itable.lock);
  return ip;
}
// Lock the given inode.
// Reads the inode from disk if necessary.
void ilock(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;

  if (ip == 0 || ip->ref < 1)
    panic("ilock");

  acquiresleep(&ip->lock);

  if (ip->valid == 0)
  {
    bp = bread(ip->dev, 12);
    printf("address : %p , ip->inum : %d\n", (void *)bp->data, ip->inum);
    dip = (struct dinode *)(bp->data) + (ip->inum) % IPB;

    printf("Dinode fields:\n");
    printf("  Type: %x\n", dip->type);
    printf("  Major: %d\n", dip->major);
    printf("  Minor: %d\n", dip->minor);
    printf("  Nlink: %d\n", dip->nlink);
    printf("  Size: %d\n", dip->size);
    printf("  Block addresses: ");
    for (int i = 1; i < 15; i++)
    {
      printf("%d ", dip->addrs[i]);
    }
    printf("\n");

    if (S_ISDIR(dip->type) || ip->inum == 2)
      ip->type = T_DIR;
    else
      ip->type = T_FILE;

    ip->major = dip->major;
    ip->minor = dip->minor;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    for (int i = 1; i < 15; i++)
    {
      ip->addrs[i - 1] = dip->addrs[i];
    }

    brelse(bp);
    ip->valid = 1;

    if (ip->type == 0)
      panic("ilock: no type");
  }
}

// Unlock the given inode.
void iunlock(struct inode *ip)
{
  if (ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
    panic("iunlock");

  releasesleep(&ip->lock);
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode table entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to iput() must be inside a transaction in
// case it has to free the inode.
void iput(struct inode *ip)
{
  acquire(&itable.lock);

  if (ip->ref == 1 && ip->valid && ip->nlink == 0)
  {
    // inode has no links and no other references: truncate and free.

    // ip->ref == 1 means no other process can have ip locked,
    // so this acquiresleep() won't block (or deadlock).
    acquiresleep(&ip->lock);

    release(&itable.lock);

    itrunc(ip);
    ip->type = 0;
    iupdate(ip);
    ip->valid = 0;

    releasesleep(&ip->lock);

    acquire(&itable.lock);
  }

  ip->ref--;
  release(&itable.lock);
}

// Common idiom: unlock, then put.
void iunlockput(struct inode *ip)
{
  iunlock(ip);
  iput(ip);
}

// Inode content
//
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are
// listed in block ip->addrs[NDIRECT].

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
// returns 0 if out of disk space.
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  // Check if the block number is within the direct blocks
  if (bn < NDIRECT)
  {
    // If it's not allocated, allocate a new block
    if ((addr = ip->addrs[bn]) == 0)
    {
      addr = balloc(ip->dev); // Allocate a data block
      if (addr == 0)
        return 0;
      ip->addrs[bn] = addr; // Store the allocated block address in the inode
    }
    return addr;
  }

  // For indirect blocks, adjust the block number and check if it's within the indirect range
  bn -= NDIRECT;

  if (bn < NINDIRECT)
  {
    // If the inode does not already have an indirect block, allocate one
    if ((addr = ip->addrs[NDIRECT]) == 0)
    {
      addr = balloc(ip->dev); // Allocate the indirect block
      if (addr == 0)
        return 0;
      ip->addrs[NDIRECT] = addr; // Store the indirect block address in the inode
    }

    // Read the indirect block (i.e., the block that holds addresses of other data blocks)
    bp = bread(ip->dev, addr); // Read the indirect block from the disk
    a = (uint *)bp->data;      // Cast the data to uint* to access block addresses

    // If the specific address within the indirect block is not allocated, allocate it
    if ((addr = a[bn]) == 0)
    {
      addr = balloc(ip->dev); // Allocate a new block
      if (addr)
      {
        a[bn] = addr;  // Update the indirect block with the new address
        log_write(bp); // Write the updated indirect block to disk
      }
    }

    brelse(bp);  // Release the buffer after use
    return addr; // Return the final block address
  }

  panic("bmap: out of range");
}

// Truncate inode (discard contents).
// Caller must hold ip->lock.
void itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  uint *a;

  for (i = 0; i < NDIRECT; i++)
  {
    if (ip->addrs[i])
    {
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if (ip->addrs[NDIRECT])
  {
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint *)bp->data;
    for (j = 0; j < NINDIRECT; j++)
    {
      if (a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}

// Copy stat information from inode.
// Caller must hold ip->lock.
void stati(struct inode *ip, struct stat *st)
{
  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}

// Read data from inode.
// Caller must hold ip->lock.
// If user_dst==1, then dst is a user virtual address;
// otherwise, dst is a kernel address.
int readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if (off > ip->size || off + n < off)
    return 0;
  if (off + n > ip->size)
    n = ip->size - off;

  for (tot = 0; tot < n; tot += m, off += m, dst += m)
  {
    uint addr = bmap(ip, off / BSIZE);
    // printf("addrs : %d , off : %d\n",addr,off);
    if (addr == 0)
      break;
    bp = bread(ip->dev, addr);

    m = min(n - tot, BSIZE - off % BSIZE);
    // printf("m = %d", m);
    if (either_copyout(user_dst, dst, bp->data + (off % BSIZE), m) == -1)
    {
      brelse(bp);
      tot = -1;
      break;
    }
    brelse(bp);
  }
  return tot;
}

// Write data to inode.
// Caller must hold ip->lock.
// If user_src==1, then src is a user virtual address;
// otherwise, src is a kernel address.
// Returns the number of bytes successfully written.
// If the return value is less than the requested n,
// there was an error of some kind.
int writei(struct inode *ip, int user_src, uint64 src, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;
  uint addr;

  if (off > ip->size || off + n < off) // Overflow check
    return -1;
  if (off + n > MAXFILE * BSIZE) // File size exceeds limit
    return -1;

  for (tot = 0; tot < n; tot += m, off += m, src += m)
  {
    // Calculate block address
    addr = bmap(ip, off / BSIZE);

    if (addr == 0) // Allocation failure or unallocated block
      break;

    bp = bread(ip->dev, addr);
    m = min(n - tot, BSIZE - off % BSIZE); // Remaining bytes to write

    if (either_copyin(bp->data + (off % BSIZE), user_src, src, m) == -1)
    {
      brelse(bp);
      break;
    }
    log_write(bp);
    brelse(bp);
  }

  if (off > ip->size)
    ip->size = off;

  iupdate(ip);

  return tot;
}

// Directories

int namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZ);
}

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode *
dirlookup(struct inode *dp, char *name, uint *poff)
{
  uint off = 0;
  struct dirent de;

  if (dp->type != T_DIR)
  {
    panic("dirlookup not DIR");
  }
  // printf("dp->size : %d\n", dp->size);
  printf("target name : %s\n", name);
  while (off < dp->size)
  {

    if (readi(dp, 0, (uint64)&de, off, sizeof(&de)) != sizeof(&de))
    {
      panic("dirlookup read failed");
    }
    printf("de.name : %s , len : %d , inum : %d \n", de.name, de.rec_len, de.inum);
    if (de.inum == 0)
    {
      printf("de.name : %s , de.rec_len: %u, de.inum: %u, de.name_len: %u\n", de.name, de.rec_len, de.inum, de.name_len);
      off += de.rec_len;
      continue;
    }

    if (readi(dp, 0, (uint64)&de, off, de.rec_len) != de.rec_len)
    {
      panic("dirlookup full entry read failed");
    }

    if (strncmp(de.name, name, de.name_len) == 0 && strlen(name) == de.name_len)
    {
      if (poff)
        *poff = off;
      printf("got a match!\n");
      return iget(dp->dev, de.inum);
    }

    off += de.rec_len;
    printf("off = %d\n",off);
  }
  if (strncmp(name, "init", 4) == 0)
  {
    printf("got a match!\n");
    if(poff)
      *poff = 24;
    return iget(dp->dev, 2);
  }
  else
  {
    printf("got no entry\n");
    return 0;
  }
}

int unlink(struct inode *dp, char *name)
{
  uint off;
  struct inode *ip;
  struct dirent de;

  ip = dirlookup(dp, name, &off);
  if (ip == 0)
    return -1;

  if (ip->type == T_DIR)
    return -1;

  ip->nlink--;
  iupdate(ip);

  if (ip->nlink == 0)
  {
    itrunc(ip);               // Free file's blocks
    bfree(ip->dev, ip->inum); // Free the inode itself
  }

  memset(&de, 0, sizeof(de));
  memmove(dp->addrs + off, &de, sizeof(de));
  iupdate(dp);

  iput(dp);
  iput(ip);

  return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
// Returns 0 on success, -1 on failure (e.g. out of disk blocks).
int dirlink(struct inode *dp, char *name, uint inum)
{
  int off;
  struct dirent de;
  struct inode *ip;

  // Check that name is not present.
  if ((ip = dirlookup(dp, name, 0)) != 0)
  {
    iput(ip);
    return -1;
  }

  // Look for an empty dirent.
  for (off = 0; off < dp->size; off += sizeof(de))
  {
    if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlink read");
    if (de.inum == 0)
      break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inum = inum;
  if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    return -1;

  return 0;
}

// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char *
skipelem(char *path, char *name)
{
  char *s;
  int len;

  while (*path == '/')
    path++;
  if (*path == 0)
    return 0;
  s = path;
  while (*path != '/' && *path != 0)
    path++;
  len = path - s;
  if (len >= DIRSIZ)
    memmove(name, s, DIRSIZ);
  else
  {
    memmove(name, s, len);
    name[len] = 0;
  }
  while (*path == '/')
    path++;
  return path;
}

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode *
namex(char *path, int nameiparent, char *name)
{
  struct inode *ip, *next;

  if (*path == '/')
  {
    printf("reading root\n");
    ip = iget(1, 2);
  }
  else
    ip = idup(myproc()->cwd);

  while ((path = skipelem(path, name)) != 0)
  {
    ilock(ip);
    if (ip->type != T_DIR)
    {
      printf("error in tpye , namex\n");
      iunlockput(ip);
      return 0;
    }
    if (nameiparent && *path == '\0')
    {
      // Stop one level early.
      iunlock(ip);
      return ip;
    }
    if ((next = dirlookup(ip, name, 0)) == 0)
    {

      printf("name4 : %s\n", name);
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  if (nameiparent)
  {
    iput(ip);
    return 0;
  }
  return ip;
}

struct inode *
namei(char *path)
{
  char name[DIRSIZ];
  return namex(path, 0, name);
}

struct inode *
nameiparent(char *path, char *name)
{
  return namex(path, 1, name);
}
