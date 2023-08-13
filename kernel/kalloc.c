// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int c[PHYSTOP / PGSIZE];
} rc;

void
incrc(uint64 pa)
{
  if((pa % PGSIZE) != 0 || (char*)pa < end || pa >= PHYSTOP)
    return;
  uint64 pi = pa / PGSIZE;
  acquire(&rc.lock);
  rc.c[pi] += 1;
  release(&rc.lock);
}

void
kinit()
{
  initlock(&rc.lock, "rc"); // initialize reference count lock
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    rc.c[(uint64)p / PGSIZE] = 1; // initialize reference count to 1
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  uint64 pi = (uint64)pa / PGSIZE;
  acquire(&rc.lock);
  rc.c[pi] -= 1;
  release(&rc.lock);
  if(rc.c[pi] > 0){
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;

    uint64 pi = (uint64)r / PGSIZE;
    acquire(&(rc.lock));
    rc.c[pi] = 1;
    release(&rc.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int
cowalloc(pagetable_t pagetable, uint64 va)
{
  if(va >= MAXVA){
    return -1;
  }
  pte_t *pte = walk(pagetable, va, 0);
  if((*pte & PTE_COW) == 0)
    return 0;
  if (pte == 0 || (*pte & PTE_U) == 0 || (*pte & PTE_V) == 0){
    return -1;
  }
  
  uint64 pa = PTE2PA(*pte);
  uint64 mem = (uint64)kalloc();
  if(mem == 0){
    return -1;
  }
  memmove((char*)mem,(char*)pa,PGSIZE);
  kfree((void*)pa);
  uint64 flags = PTE_FLAGS(*pte);
  *pte = PA2PTE(mem) | flags | PTE_W; // set PTE_W to allow user to write to the page
  *pte &= ~PTE_COW; // clear PTE_COW to indicate that the page is not copy-on-write anymore
  return 0;
}
