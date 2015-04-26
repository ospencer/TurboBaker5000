#include <hash.h>
#include "filesys/filesys.h"
#include "devices/block.h"
#include "filesys/cache.h"
#include "threads/malloc.h"
#include <string.h>
struct hash *block_cache;

/* Initializes the cache hash table. */
void
block_cache_create ()
{
  hash_init (block_cache, block_hash, block_hash_less, NULL);
}

/* Hash function for block cache. Does hash by sector. */
unsigned 
block_hash (const struct hash_elem *h, void *aux)
{
  const struct sector_block *b = hash_entry (h, struct sector_block, hash_elem);
  return hash_bytes (&b->sector, sizeof b->sector);
}

/* Less function for block cache. Does by sector location. */
bool
block_hash_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux)
{
  const struct sector_block *a = hash_entry (a_, struct sector_block, hash_elem);
  const struct sector_block *b = hash_entry (b_, struct sector_block, hash_elem);

  return a->sector < b->sector;
}

/* Evicts a block from the cache. Uses least recently used method. */
void
block_evict ()
{
  struct sector_block *evict_candidate;

  /* Find the least recently used block in the cache. */
  struct hash_iterator i;
  int c = 0;
  hash_first (&i, block_cache);
  while (hash_next (&i))
  {
    struct sector_block *s = hash_entry (hash_cur (&i), struct sector_block, hash_elem);
    if (c == 0) 
    { 
      evict_candidate = s;
      c += 1;
    }
    if (s->last_access < evict_candidate->last_access) evict_candidate = s;
  }
  
  /* If the block is dirty, write it to disk. */
  if (evict_candidate->dirty)
  {
    block_write (fs_device, evict_candidate->sector, evict_candidate->buffer);
  }

  /* Remove the block from the table. */
  hash_delete (block_cache, &evict_candidate->hash_elem);

  /* Free the memory allocated by malloc. */
  free (evict_candidate->buffer);
}

/* Grabs a block from the block cache.
   Returns NULL if the block isn't currently cached. */
struct sector_block *
get_block_from_cache (block_sector_t s)
{
  struct sector_block *sector;
  struct hash_elem *e;
  
  sector->sector = s;
  e = hash_find (block_cache, &sector->hash_elem);

  if (e != NULL)
  {
    sector = hash_entry (e, struct sector_block, hash_elem);
    sector->last_access = rtc_get_time ();
    return sector;
  }
  else return NULL;
}

/* Adds a block from sector on disk to the block cache. */
struct sector_block *
cache_block_from_sector (block_sector_t sector)
{
  /* Max of 64 blocks to cache. Evict a block if we're at the limit. */
  if (hash_size (block_cache) >= 64) 
    block_evict (); 

  /* Set up struct, malloc for sector data. */
  struct sector_block *sec_block;
  sec_block->sector = sector;
  sec_block->last_access = rtc_get_time ();
  sec_block->dirty = false;
  sec_block->buffer = malloc (BLOCK_SECTOR_SIZE);

  /* Read data from sector directly into struct. */
  block_read (fs_device, sector, sec_block->buffer);

  /* Insert struct into the hash table. */
  hash_insert (block_cache, &sec_block->hash_elem);

  return sec_block;
}

/* Writes to a block in cache. If the block isn't cached,
   it is first cached. */
void 
cache_and_write_block (block_sector_t sector, void* buffer)
{
  /* Check if block is in cache. */
  struct sector_block *sec_block = get_block_from_cache (sector);

  /* Put block in cache if it isn't already. */
  if (sec_block == NULL)
  {
    sec_block = cache_block_from_sector (sector);
  }

  /* Write to block in cache. */
  memcpy (sec_block->buffer, buffer, sizeof sec_block->buffer);

  /* Let that block know it's all dirty and stuff. */
  sec_block->dirty = true;

  /* Update last access time. */
  sec_block->last_access = rtc_get_time ();
}
