#ifndef VM_FRAME_TABLE_H
#define VM_FRAME_TABLE_H

#include <hash.h>
#include "threads/palloc.h"

//int * choose_page_to_evict(void);
//int add_frame(int * Ppage);

//unsigned hash_hash_func (const struct hash_elem *element, void *aux);
//bool hash_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);

typedef struct page
{
  struct hash_elem hash_elem;	/*Hash table element. */
  void *addr;			/* Virtual address. */
  int lastAccessTime;		/* Number of ticks since this entry was last accessed */
};

void frame_table_create (void);
void * add_frame_entry (enum palloc_flags flags);
struct page * page_lookup (const void *address);
unsigned page_hash (const struct hash_elem *p_, void *aux);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux);

#endif /* vm/frame_table.h */
