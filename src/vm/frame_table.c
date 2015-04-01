#include "vm/frame_table.h"
#include <hash.h>
#include "threads/palloc.h"
#include <debug.h>

struct hash table;

void
frame_table_create ()
{
  hash_init (&table, page_hash, page_less, NULL);
}

void *
add_frame_entry(enum palloc_flags flags)
{
  struct page *p;
  p = malloc(sizeof(struct page));
  p->addr = palloc_get_page(PAL_USER | PAL_ASSERT | flags);
  if(p->addr == NULL) PANIC ("palloc_get: out of pages");
 
  
   
//  p->hash_elem = &p->addr;
//  printf("Testing\n\n");
//  printf("hash_elem is %d\n\n", &p->hash_elem == NULL);
  if(hash_insert (&table, &(p->hash_elem)) != NULL) PANIC ("Page already in use");
  return p->addr;
}

/* Returns the page containing the given virtual address, or a null pointer if no such page exists. */
struct page *
page_lookup (const void *address)
{
  struct page p;
  struct hash_elem *e;

  p.addr = address;
  e = hash_find (&table, &p.hash_elem);
  return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}

unsigned
page_hash (const struct hash_elem *p_, void *aux)
{
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->addr, sizeof p->addr);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux)
{
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->addr < b->addr;
}
