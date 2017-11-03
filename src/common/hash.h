/***************************************************
 * hash.h
 * Created on Thu, 19 Oct 2017 11:59:45 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#ifndef HASH_H_
#define HASH_H_

#include <sys/types.h>

#define HASH_TABLE_SIZE 1024

struct hash_item_s {
  int hi_hash;
  char *hi_key;
  void *hi_item;
  struct hash_item_s *next;
};

struct hash_s {
  struct hash_item_s *h_table[HASH_TABLE_SIZE];
};

void hash_create(struct hash_s **hash);
void hash_destroy(struct hash_s **hash);
void hash_item_destroy(struct hash_item_s *item);

void hash_add(struct hash_s *hash, char *key, void *item);
void hash_remove_all(struct hash_s *hash, char *key);
void hash_remove_one(struct hash_s *hash, char *key, void *value);
void *hash_item_find(struct hash_s *hash, char *key);

void *hash_find_first(struct hash_s *hash, char *key);
int hash_find_all(struct hash_s *hash, char *key, void **found, size_t found_size);

#endif
