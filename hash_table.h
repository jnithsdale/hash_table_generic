/* hash_table.h - Generic Hash Table with 2-d linked lists for collisions
* and duplicates. Objects are mapped onto an unsigned long between 0 and 
* number_of_buckets - 1. Not type safe
* Collisions are listed in order so we can stop searching early if we go
* over a certain value.
*
* Requires a predefined hash, compare, search & free function for
* object in use.
*
* 
* Copyright 2014 Joshua Nithsdale
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
* 
*     http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/
#ifndef __HASH_TABLE_H
#define __HASH_TABLE_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct hash_table_t {
	struct hash_table_bucket_t ** buckets;
	unsigned long number_of_total_buckets;
	
	unsigned long number_of_buckets_filled;
	unsigned long number_of_collisions; 
	unsigned long number_of_duplicates;

	
	unsigned long(*hash_function)(char * string, unsigned long max_number);
	int(*compare_function)(void * object1, void * object2);	
	int(*search_function)(char * search_string, void * object);	
	void(*free_function)(void * object);
	
} hash_table_t;

typedef struct hash_table_bucket_t {
	struct hash_table_fill_t * first_fill;
	struct hash_table_fill_t * last_fill;
} hash_table_bucket_t;

typedef struct hash_table_fill_t {
	void * object;
	struct hash_table_fill_t * next_fill;
	struct hash_table_duplicate_t * first_duplicate;
	struct hash_table_duplicate_t * last_duplicate;
} hash_table_fill_t;

typedef struct hash_table_duplicate_t {
	void * object;
	struct hash_table_duplicate_t * next_duplicate;
} hash_table_duplicate_t;

/* 
* Returns a new allocated hash_table_t
* number_of_buckets limits the size of the array holding buckets. Cannot be
* changed.
*
* hash_fun should be a pointer to a function that takes an string
* and hashes it into a unsigned long no larger than max_number_of_buckets - 1.
*
* compare_fun should take two objects and return < 1 if object1 is before 
* object2, 0 if same/duplicate, > 1 if object1 is after object2
*
* search_fun should take a string and a object and return a 1 if it matches.
*
* free_fun should take a object and free it. Parse NULL if not freeing.
* (not heap allocated).
*
* returns NULL if there was an error allocating memory
*/
hash_table_t * Hash_Table_Init(unsigned long number_of_buckets, 
	unsigned long(*hash_fun)(char * string, unsigned long max_number),
	int(*compare_fun)(void * object1, void * object2),
	int(*search_fun)(char * search_string, void * object),
	void(*free_fun)(void * object));

/* 
* Insert a new object into the hash table. Pattern should be a string that 
* will be hashed for key
* Return 1 if successful - 0 if failure (memory allocation).
*/
int Hash_Table_Insert(hash_table_t * table, void * object, char * pattern);

/* 
* Insert a new object into the hash table only if it has no duplicates 
	(given pattern).
* Pattern should be a string that will be hashed for key 
	(and be unique for each object).
* Return 1 if successful, 0 already exists, -1 if failure (memory allocation).
* If 0 is returned, found_duplicate we become a pointer to the first duplicate 
*/
int Hash_Table_Insert_No_Duplicate(hash_table_t * table, void * object,
	char * pattern, void ** found_duplicate);

/* Free table and contained objects */
void Hash_Table_Free(hash_table_t * table);

/* Find an object in the table given pattern string, will hash key
* and then return array of pointers to matches (duplicates).
* number_of_objects_found will get changed to number found.
* Returns NULL if nothing found (or there was an error).
*/
void ** Hash_Table_Match(hash_table_t * table, char * pattern,
	unsigned long * number_of_objects_found, unsigned long max_num_records);
	
/* Find the first object in the table given pattern string.
* Returns NULL if nothing found (or there was an error).
*/
void * Hash_Table_First_Match(hash_table_t * table, char * pattern);

/* Returns number of bytes that the hash table currently has been 
* allocated not including dereferenced objects table holds. 
*/
unsigned long Hash_Table_Size(hash_table_t * table);
#endif


