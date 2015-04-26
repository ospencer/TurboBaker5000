#include <hash.h>
#include "devices/rtc.h"

typedef struct sector_block 
{
  struct hash_elem hash_elem;
  block_sector_t sector;
  void* buffer;
  bool dirty;
  time_t last_access;
};

unsigned block_hash (const struct hash_elem *, void *);
bool block_hash_less (const struct hash_elem *, const struct hash_elem *, void *);
void block_evict (void);
void block_cache_create (void);
struct sector_block * get_block_from_cache (block_sector_t);
struct sector_block * cache_block_from_sector (block_sector_t);
void cache_and_write_block (block_sector_t, void *);
