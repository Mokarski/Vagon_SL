/***************************************************
 * hash.c
 * Created on Fri, 20 Oct 2017 17:36:26 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <string.h>
#include <stdlib.h>
#include "hash.h"

int hash_string(char *s) {
  int i, hash = 7;
  for(i = 0; s[i] != 0; i++) {
    hash = (hash * 31 + s[i]) % HASH_TABLE_SIZE;
  }
  return hash;
}

void hash_create(struct hash_s **hash) {
  *hash = malloc(sizeof(struct hash_s));
  memset((*hash)->h_table, 0, sizeof((*hash)->h_table));
}

void hash_destroy(struct hash_s **hash) {
  if(*hash)
    free(*hash);

  *hash = NULL;
}

void hash_item_destroy(struct hash_item_s *item) {
  free(item->hi_key);
  free(item);
}

void hash_add(struct hash_s *hash, char *key, void *item) {
  struct hash_item_s *hitem;
  hitem = malloc(sizeof(struct hash_item_s));
  hitem->hi_hash = hash_string(key);
  hitem->hi_key = strdup(key);
  hitem->hi_item = item;

  hitem->next = hash->h_table[hitem->hi_hash];
  hash->h_table[hitem->hi_hash] = hitem;
}

void hash_remove_all(struct hash_s *hash, char *key) {
  int hf = hash_string(key);
  struct hash_item_s *hitem, **prev;

  prev = &hash->h_table[hf];
  hitem = *prev;

  while(hitem) {
    if(!strcmp(hitem->hi_key, key)) {
      *prev = hitem->next;
      hash_item_destroy(hitem);
    } else {
      prev = &hitem->next;
    }

    hitem = *prev;
  }
}

void hash_remove_one(struct hash_s *hash, char *key, void *item) {
  int hf = hash_string(key);
  struct hash_item_s *hitem, **prev;

  prev = &hash->h_table[hf];
  hitem = *prev;

  while(hitem) {
    if(!strcmp(hitem->hi_key, key) && hitem->hi_item == item) {
      *prev = hitem->next;
      hash_item_destroy(hitem);
      return;
    }

    prev = &hitem->next;
    hitem = *prev;
  }
}

void *hash_item_find(struct hash_s *hash, char *key) {
  int hf = hash_string(key);
  struct hash_item_s *hitem;
  int i = 1;

  hitem = hash->h_table[hf];

  while(hitem) {
    if(!strcmp(hitem->hi_key, key)) {
      return hitem->hi_item;
    }

    i ++;
    hitem = hitem->next;
  }

  return NULL;
}

void *hash_find_first(struct hash_s *hash, char *key) {
  return hash_item_find(hash, key);
}

int hash_find_all(struct hash_s *hash, char *key, void **found, size_t found_size) {
  int hf = hash_string(key), result = 0;
  struct hash_item_s *hitem;
  int i = 0;

  hitem = hash->h_table[hf];

  while(hitem) {
    if(!strcmp(hitem->hi_key, key)) {
      if(result < found_size) {
        found[result] = hitem->hi_item;
      }
      result ++;
    }

    i ++;
    hitem = hitem->next;
  }

  return result;
}
