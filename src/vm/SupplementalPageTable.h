#include <hash.h>

bool init_hash_table(struct hash *);

/* An enumeration of all the locations that a page may be. */
typedef enum
{
  IN_MEMORY,
  SWAPPED_OUT,
  IN_FILE
} location;

/* A page struct for the hash table. */
struct page 
{
  struct hash_elem hash_elem;
  void *addr;
  location location;
}
