/* hash_table.c - Generic Hash Table with 2-d linked lists for collisions
* and duplicates. Objects are mapped onto an unsigned long between 0 and
* number_of_buckets - 1. Not type safe
*
* Requires a predefined hash, compare & free function for
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
#include "hash_table.h"


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
	void(*free_fun)(void * object))
{
	hash_table_t * new_hash_table;
	hash_table_bucket_t ** buckets;

	assert(hash_fun != NULL);
	assert(compare_fun != NULL);
	assert(search_fun != NULL);

	new_hash_table = calloc(1, sizeof(hash_table_t));
	if (new_hash_table == NULL)
		return NULL;

	buckets = calloc(number_of_buckets, sizeof(hash_table_bucket_t *));
	if (buckets == NULL)
		return NULL;

	new_hash_table->buckets = buckets;
	new_hash_table->number_of_total_buckets = number_of_buckets;

	new_hash_table->number_of_buckets_filled = 0;
	new_hash_table->number_of_collisions = 0;
	new_hash_table->number_of_duplicates = 0;

	new_hash_table->hash_function = hash_fun;
	new_hash_table->compare_function = compare_fun;
	new_hash_table->search_function = search_fun;
	new_hash_table->free_function = free_fun;

	return new_hash_table;
}


/*
* Insert a new object into the hash table. Pattern should be a string that
* will be hashed for key
* Return 1 if successful - 0 if failure (memory allocation).
*/
int Hash_Table_Insert(hash_table_t * table, void * object, char * pattern)
{
	unsigned long hashedIndex;
	int compareVal, objectPlaced = 0;
	hash_table_bucket_t * new_bucket = NULL;
	hash_table_fill_t * new_bucket_fill = NULL, *current_bucket_fill = NULL,
		*prev_bucket_fill = NULL;
	hash_table_duplicate_t * new_node_duplicate = NULL, *current_dup = NULL;

	assert(table != NULL);
	assert(object != NULL);

	/* Hash pattern */
	/* printf("Hashing: %s\n", pattern); */
	
	hashedIndex = table->hash_function(pattern, 
		table->number_of_total_buckets);

	/* Now insert into table: go to hashed index, if there is already a record
	* test if collision and/or duplicate */

	if (table->buckets[hashedIndex] == NULL)
	{
		
		/* no bucket found, allocate and place */
		new_bucket = calloc(1, sizeof(hash_table_bucket_t));
		if (new_bucket == NULL)
			return 0;

		new_bucket_fill = calloc(1, sizeof(hash_table_fill_t));
		if (new_bucket_fill == NULL)
		{
			free(new_bucket);
			return 0;
		}

		new_bucket_fill->object = object;

		new_bucket->first_fill = new_bucket_fill;
		new_bucket->last_fill = new_bucket_fill;

		table->buckets[hashedIndex] = new_bucket;
		(table->number_of_buckets_filled)++;
		return 1;
	}
	else
	{

		/* Collision found - go through each one and see if any duplicates */

		current_bucket_fill = table->buckets[hashedIndex]->first_fill;
		while (current_bucket_fill != NULL && !objectPlaced)
		{
			/* See if duplicate */
			compareVal = table->compare_function(current_bucket_fill->object,
				object);
			if (compareVal == 0)
			{
				/* duplicate - add to duplicate list */

				current_dup = current_bucket_fill->last_duplicate;
				
				new_node_duplicate = calloc(1, sizeof(hash_table_duplicate_t));
				if (new_node_duplicate == NULL)
					return 0;

				new_node_duplicate->object = object;

				if (current_dup != NULL)
				{
					/* We traversed a list of duplicates, update second last */
					current_dup->next_duplicate = new_node_duplicate;
				}
				else
				{
					/* No duplicates found, add to start*/
					current_bucket_fill->first_duplicate = new_node_duplicate;
				}
				current_bucket_fill->last_duplicate = new_node_duplicate;

				(table->number_of_duplicates)++;
				objectPlaced = 1;

			}
			else if (compareVal < 0)
			{
				/* not duplicate - object we want to place is after current 
				- traverse collision list */
				prev_bucket_fill = current_bucket_fill;
				current_bucket_fill = current_bucket_fill->next_fill;
			}
			else
			{
				/* not duplicate - we want to place new object before current
				so we stop traversing list*/
				break;
			}

		}

		if (!objectPlaced)
		{
			/* not duplicate - add to current position of collision list */

			new_bucket_fill = calloc(1, sizeof(hash_table_fill_t));
			if (new_bucket_fill == NULL)
				return 0;

			new_bucket_fill->object = object;

			if (current_bucket_fill != NULL)
				new_bucket_fill->next_fill = current_bucket_fill;
			else
				table->buckets[hashedIndex]->last_fill = new_bucket_fill;

			if (prev_bucket_fill != NULL)
				prev_bucket_fill->next_fill = new_bucket_fill;
			else
				table->buckets[hashedIndex]->first_fill = new_bucket_fill;

			(table->number_of_collisions)++;
		}

		return 1;
	}
}

/* 
* Insert a new object into the hash table only if it has no duplicates 
	(given pattern).
* Pattern should be a string that will be hashed for key 
	(and be unique for each object).
* Return 1 if successful, 0 already exists, -1 if failure (memory allocation).
* If 0 is returned, found_duplicate we become a pointer to the first duplicate 
*/
int Hash_Table_Insert_No_Duplicate(hash_table_t * table, void * object,
	char * pattern, void ** found_duplicate)
{
	unsigned long number_records = 0;
	void ** object_temp = NULL;
	
	assert(table != NULL);
	assert(object != NULL);
	assert(pattern != NULL);

	/* check if already in table */
	object_temp = Hash_Table_Match(table, pattern,
		&number_records, 1); 
		/* 1 means we only need to see if found > 0 */
		
	if(number_records == 0)
	{
		/* Isn't in table yet, add it */			
		if(Hash_Table_Insert(table, object, pattern) == 0)
		{
			/* error adding */
			return -1;
		}
		else
			return 1;
	}
	else
	{
		/* already exists */
		*found_duplicate = object_temp[0];
		free(object_temp);
		return 0;
	}

}

/* Free table and contained objects */
void Hash_Table_Free(hash_table_t * table)
{
	unsigned long i;
	hash_table_bucket_t * current_bucket;
	hash_table_fill_t * current_fill, * next_fill;
	hash_table_duplicate_t * current_dup, *next_dup;

	for (i = 0; i < table->number_of_total_buckets; i++)
	{
		/* for each bucket,   */
		current_bucket = table->buckets[i];

		if (current_bucket != NULL)
		{
			/* and then for each fill/collision */
			current_fill = current_bucket->first_fill;

			while (current_fill != NULL)
			{
				/* and for each duplicate */
				current_dup = current_fill->first_duplicate;
				while (current_dup != NULL)
				{
					next_dup = current_dup->next_duplicate;

					/* free duplicate */
					if (table->free_function != NULL)
						table->free_function(current_dup->object);

					free(current_dup);

					current_dup = next_dup;
				}

				next_fill = current_fill->next_fill;

				/* free fill */
				if (table->free_function != NULL)
					table->free_function(current_fill->object);

				free(current_fill);

				current_fill = next_fill;
			}
			

			/* free bucket */
			free(current_bucket);
		}
	}

	/* free table and bucket pointers */
	free(table->buckets);
	free(table);

}

/* Find an object in the table given pattern string, will hash key
* and then return array of pointers to matches (duplicates).
* number_of_objects_found will get changed to number found.
* Returns NULL if nothing found (or there was an error).
*/
void ** Hash_Table_Match(hash_table_t * table, char * pattern,
	unsigned long * number_of_objects_found, unsigned long max_num_records)
{
	unsigned long key;
	hash_table_bucket_t * bucket;
	hash_table_fill_t * current_fill;
	hash_table_duplicate_t * current_dup;
	void ** found_records;

	/* Hash key here */
	key = table->hash_function(pattern, table->number_of_total_buckets);

	/* Lookup table */
	bucket = table->buckets[key];

	/* See if exists */
	if (bucket == NULL)
	{
		*number_of_objects_found = 0;
		return NULL;
	}
	else
	{
		/* Found possible match - check fills/collisions */
		current_fill = bucket->first_fill;

		while (current_fill != NULL)
		{
			if (table->search_function(pattern, current_fill->object) == 1)
			{
				/* Found match, add it and duplicates to return array */
				found_records = calloc(max_num_records, 
					sizeof(void *));
				if (found_records == NULL)
				{
					printf("Error allocating memory for search.\n");
					return NULL;
				}

				found_records[0] = current_fill->object;
				(*number_of_objects_found)++;

				current_dup = current_fill->first_duplicate;
				while (current_dup != NULL &&
					*number_of_objects_found < max_num_records)
				{
					found_records[*number_of_objects_found] =
						current_dup->object;
					(*number_of_objects_found)++;

					current_dup = current_dup->next_duplicate;

				}

				return found_records;
			}
			else
			{
				/* Does not match current , go through collisions*/
				current_fill = current_fill->next_fill;
			}
		}

		/* never found any match */
		return NULL;
	}
}

/* Find the first object in the table given pattern string.
* Returns NULL if nothing found (or there was an error).
*/
void * Hash_Table_First_Match(hash_table_t * table, char * pattern)
{
	void ** found_objects, * found_object;
	unsigned long number_of_objects_found = 0;
	
	assert(table != NULL);
	assert(pattern != NULL);
	
	found_objects = Hash_Table_Match(table,pattern,
	&number_of_objects_found, 1);
		/* We only want one object */
	if(found_objects != NULL)
	{
		found_object = found_objects[0]; /* First one */
		free(found_objects);
		return found_object;
	}
	else
	{
		return NULL;
	}

}

/* Returns number of bytes that the hash table currently has been
* allocated not including objects table holds.
*/
unsigned long Hash_Table_Size(hash_table_t * table)
{
	unsigned long table_size, bucket_size, bucket_fill_size, 
		bucket_duplicate_size;


	table_size = sizeof(hash_table_t) +
		table->number_of_total_buckets * sizeof(hash_table_bucket_t *);

	bucket_size = table->number_of_buckets_filled * 
		sizeof(hash_table_bucket_t);

	bucket_fill_size = (table->number_of_buckets_filled + 
		table->number_of_collisions) *
		sizeof(hash_table_fill_t);

	bucket_duplicate_size = table->number_of_duplicates * 
		sizeof(hash_table_duplicate_t);



	return table_size + bucket_size + bucket_fill_size + bucket_duplicate_size;
}