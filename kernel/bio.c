// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define SLOTS 13

struct bucket{
  struct spinlock lock;
  struct buf head;   // virtul node
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct bucket hash[SLOTS];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  struct bucket *bu;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->next = bcache.hash[0].head.next;
    bcache.hash[0].head.next = b;
  }
  for (bu = bcache.hash; bu < bcache.hash+SLOTS; bu++) {
    initlock(&bu->lock, "bucket");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *bget(uint dev, uint blockno) {
  // printf("bget no is %d\n",blockno);
  struct buf *b;
  struct bucket *bucket = bcache.hash + (dev + blockno) % SLOTS;
  acquire(&bucket->lock);
  b = bucket->head.next;
  while (b) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }
  b = bucket->head.next;
  while (b) {
    if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }
  release(&bucket->lock);

  acquire(&bcache.lock);

  struct bucket *bu;
  for (bu = bcache.hash; bu < bcache.hash + SLOTS; bu++) {
    if (bu != bucket) {
      acquire(&bu->lock);
      struct buf *prev;
      b = bu->head.next;
      prev = &bu->head;
      while (b) {
        if (b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          prev->next = b->next; 
          acquire(&bucket->lock);
          b->next = bucket->head.next;
          bucket->head.next = b;
          release(&bucket->lock);
          release(&bu->lock);
          release(&bcache.lock);
          acquiresleep(&b->lock);
          return b;
        }
        b = b->next;
        prev = prev->next;
      }
      release(&bu->lock);
    }
  }
  release(&bcache.lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  b->refcnt--;
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


