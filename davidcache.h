typedef struct
{
    int key;
    void *value;
} bucket;

typedef struct
{
    bucket **buckets;
    int size;
    int num_keys;
    unsigned int (*hash_function)(int);
    int (*equality_function)(void *, void *);
    void (*free_key)(int);
    void (*free_value)(void *);
} hash_map;

typedef struct node
{
	void *data;
	struct node *next;
	struct node *prev;
} node;

typedef struct linked_list
{
	node *head;
	node *tail;
	size_t num_nodes;
} linked_list;

typedef struct
{
    int key;
    int value;
    int freq;
} lfu_item;

typedef struct
{
    hash_map *freq_map;
    hash_map *item_map;
    int min_freq;
    int size;
    int capacity;
} LFUCache;

node *create_node(void *data)
{
    node *n = (node *)malloc(sizeof(node));
    n->data = data;
    n->next = NULL;
    n->prev = NULL;
    return n;
}

void free_node(node *n, void (*free_data)(void *))
{
    free_data(n->data);
    free(n);
}


lfu_item *create_lfu_item(int key, int value)
{
    lfu_item *new_item = malloc(sizeof(lfu_item));
    new_item->key = key;
    new_item->value = value;
    new_item->freq = 1;
    return new_item;
}

/**
 * Utility function to update an existing lfu_item with the specified key and value
 */
lfu_item *update_lfu_item(lfu_item *item, int key, int value)
{
    item->key = key;
    item->value = value;
    return item;
}

void print_lfu_item(void *item) {
    lfu_item* temp = ((lfu_item*)item);
    printf("{Key:%d, Value:%d, Freq: %d}", temp->key, temp->value, temp->freq);
}


linked_list *create_linked_list()
{
    //	linked_list *list = (linked_list *)malloc(sizeof(linked_list));
    //	list->head = list->tail = NULL;
    //	list->num_nodes = 0;
    linked_list *list = (linked_list *)calloc(1, sizeof(linked_list));
    return list;
}

/**
 * Pushes node into list at the back
 * Makes the node the new tail
 */
void push_back(linked_list *list, node *node)
{
    if (list->head == NULL)
    {
        list->head = list->tail = node;
    }
    else
    {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->num_nodes++;
}

/**
 * Pushes node into list at the front
 * Makes the node the new head
 */
void push_front(linked_list *list, node *node)
{
    if (list->head == NULL)
    {
        list->head = list->tail = node;
    }
    else
    {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->num_nodes++;
}

/**
 * Pops node at current head
 */
node *pop_front(linked_list *list)
{
    node *node = list->head;
    if (node == NULL)
    {
        return NULL;
    }
    if (node->next == NULL)
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        list->head = list->head->next;
        list->head->prev = NULL;
        node->next = NULL;
    }
    list->num_nodes--;
    return node;
}

/**
 * Pops node at current tail
 */
node *pop_back(linked_list *list)
{
    node *node = list->tail;
    if (node == NULL)
    {
        return NULL;
    }
    if (node->prev == NULL)
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        list->tail = list->tail->prev;
        list->tail->next = NULL;
        node->prev = NULL;
    }
    list->num_nodes--;
    return node;
}

/**
 * Pops node anywhere in list if possible
 */
void pop_node(linked_list *list, node* node) {
    if(node == NULL) {
        perror("Null node cannot be removed!");
        exit(EXIT_FAILURE);
    }
    if(list == NULL) {
        perror("Null linked_list cannot be removed from!");
        exit(EXIT_FAILURE);
    }

    //singleton
    if(node->next == NULL && node->prev == NULL) {
        pop_front(list);
        return;
    }

    //head
    else if(node->next != NULL && node->prev == NULL){
        pop_front(list);

        return;
    }

    //tail
    else if(node->next == NULL && node->prev != NULL) {
        pop_back(list);

        return;
    }
    
    //middle node
    else {

        node->next->prev = node->prev;
        node->prev->next = node->next;
        node->next = NULL;
        node->prev = NULL;
        list->num_nodes--;
        return;
    }

}
/**
 * Check is linked list is empty
 */
bool is_empty(linked_list *list) {
    return list->head == NULL && list->tail == NULL;
}

/**
 * Print all nodes in linked list
 */
void print_list(linked_list *list, void (*print_function)(void *))
{
    putchar('[');
    node *cur = list->head;
    if (cur != NULL)
    {
        print_function(cur->data);
        cur = cur->next;
    }
    for (; cur != NULL; cur = cur->next)
    {
        printf(", ");
        print_function(cur->data);
    }
    puts("]");
}

/**
 * Utility function to print out a hash map with linked list keys
 */
void print_list_map(unsigned int index, int key, void *value)
{
    linked_list *temp = (linked_list *) value;
    printf("Index: %i - (Key:%d, Value:", index, key);
    print_list(temp, print_lfu_item);
    printf("\n");
    
}

//used as a marker for deleted cells
#define DELETED (void *)-1

//max number of places to look before giving up
#define MAX_PROBES 20

/**
 * Allocate space for a new hash map
 */
hash_map *allocate_map(unsigned int capacity,
                       unsigned int (*hash_function)(int),
                       int (*equality_function)(void *, void *),
                       void (*free_key)(int),
                       void (*free_value)(void *))
{
   // get the lowest power of 2 that is above capacity
   // this allows us to avoid costly modulo operations down the line
   unsigned int num_bits = 0,
                power_of_2_capacity = 0;
   while (capacity > 0)
   {
      capacity >>= 1;
      num_bits++;
   }
   power_of_2_capacity = 1 << (num_bits + 1);

   // allocate space for hash map struct and the array of pointer to the buckets
   hash_map *map = malloc(sizeof(hash_map));
   map->buckets = calloc(power_of_2_capacity, sizeof(bucket *));
   map->num_keys = 0;
   map->size = power_of_2_capacity;
   map->equality_function = equality_function;
   map->hash_function = hash_function;
   map->free_key = free_key;
   map->free_value = free_value;

   return map;
}

/**
 * Free the memory allocated for a hash map. Calls the free_value and free_key 
 * functions passed in on the map creation to properly free the object at the 
 * key and value pointers for each entry.
 */
void free_map(hash_map *map)
{
   for (int i = 0; i < map->size; i++)
   {
      if (map->buckets[i] != NULL && map->buckets[i] != DELETED)
      {
         if (map->free_key != NULL)
            map->free_key(map->buckets[i]->key);
         if (map->free_value != NULL)
            map->free_value(map->buckets[i]->value);
         free(map->buckets[i]);
      }
   }
   free(map->buckets);
   free(map);
}

/**
 * Hash an integer to an unsigned int with roughly equal probability that each bit is one
 * important because we do not control the size of the table and thus cannot ensure it is prime
 * (size is actually always a power of 2, not prime)
 * source: https://stackoverflow.com/a/12996028
 */
unsigned int hash_int(int ptr)
{
   int key = ptr;
   unsigned int x = (unsigned int)key;
   // x = ((x >> 16) ^ x) * 0x45d9f3b;
   // x = ((x >> 16) ^ x) * 0x45d9f3b;
   // x = (x >> 16) ^ x;
   return x;
}

/**
 * Returns 1 if the integers at each pointer are equal, else 0.
 */
int equal_int(void *a, void *b)
{
   return *(int *)a == *(int *)b;
}

/**
 * Utility function to print out a hash map with integer keys
 */
void print_lfu_map(unsigned int index, int key, void *value)
{
   lfu_item *temp = ((node*)value)->data;
   printf("Index: %i - (Key: %d, Values: [Key:%d, Value:%d, Freq:%d])\n", index, key, temp->key, temp->value, temp->freq);
}

/**
 * Internal function to find the index of a key
 */
int _find_key(hash_map *map, int key)
{
   unsigned int h = map->hash_function(key);
   for (int j = 0; j < MAX_PROBES; j++)
   {
      unsigned int next_index = (h + (j * j) + (23 * j)) & (map->size - 1);
      if (map->buckets[next_index] == NULL)
      {
         return -1;
      }
      else if (map->buckets[next_index] == DELETED)
      {
         continue;
      }
      else if (map->hash_function(map->buckets[next_index]->key) == h &&
               map->buckets[next_index]->key == key)
      {
         return next_index;
      }
   }
   return -1;
}

/**
 * Returns 1 if key is in the map, 0 otherwise
 */
int map_contains(hash_map *map, int key)
{
   return _find_key(map, key) == -1 ? 0 : 1;
}

/**
 * Returns the value associated with key in the map, or null if the key is not
 * in the map.
 */
void *map_get(hash_map *map, int key)
{
   int index = _find_key(map, key);
   return index == -1 ? NULL : map->buckets[index]->value;
}

/**
 * Update the value associated with a key, deleting freeing the old key and 
 * value if they are different pointers than the new one.
 * Returns 1 if key is in the map, 0 otherwise
 */
int map_update(hash_map *map, int key, void *value)
{
   int index = _find_key(map, key);

   if (index == -1)
   {
      return 0;
   }

   if (key != map->buckets[index]->key)
      map->free_key(map->buckets[index]->key);
   if (value != map->buckets[index]->value)
      map->free_value(map->buckets[index]->value);

   map->buckets[index]->key = key;
   map->buckets[index]->value = value;

   return 1;
}

/**
 *  Inserts a key into the hash table. Returns 0 if unsuccessful, and 1 if successful.
 *  Postcondition: the memory pointed at by key and value is never freed before the 
 *  pair is removed from the hash set (i.e. be careful with stack variables)
 */
int map_insert(hash_map *map, int key, void *value)
{
   if (map_contains(map, key))
   {
      return 0;
   }

   unsigned int h = map->hash_function(key);

   for (int j = 0; j < MAX_PROBES; j++)
   {
      unsigned int next_index = (h + (j * j) + (23 * j)) & (map->size - 1);
      if (map->buckets[next_index] == NULL || map->buckets[next_index] == DELETED)
      {
         bucket *elem = malloc(sizeof(bucket));
         elem->key = key;
         elem->value = value;

         map->buckets[next_index] = elem;
         (map->num_keys)++;
         return 1;
      }
   }

   return 0;
}

/**
 * Deletes a key from the hash table. Returns 0 if unsuccessful, and 1 if successful.
 * Calls the free_value and fre_key functions passed in on the map creation to properly
 * free the objects at the key and value pointers.
 */
int map_delete(hash_map *map, int key)
{
   int index = _find_key(map, key);

   if (index == -1)
   {
      return 0;
   }

   if (map->free_key != NULL)
      map->free_key(map->buckets[index]->key);
   if (map->free_value != NULL)
      map->free_value(map->buckets[index]->value);
   free(map->buckets[index]);

   map->buckets[index] = DELETED;
   (map->num_keys)--;

   return 1;
}

/**
 *  Prints out the hash table. 
 */
void print_map(hash_map *map)
{
   printf("Size: %i\n", map->num_keys);
   for (int i = 0; i < map->size; i++)
   {
      if (map->buckets[i] != NULL && map->buckets[i] != DELETED)
      {
         printf("%i: (%i, %p)\n", i, map->buckets[i]->key, map->buckets[i]->value);
      }
   }
}

/**
 *  Prints out the hash table, using the supplied function to print each 
 *  key value pair.
 */
void pretty_print_map(hash_map *map, void (*print_function)(unsigned int index, int key, void *value))
{
   printf("Pretty print - Size: %i\n", map->num_keys);
   for (int i = 0; i < map->size; i++)
   {
      if (map->buckets[i] != NULL && map->buckets[i] != DELETED)
      {
         print_function(i, map->buckets[i]->key, map->buckets[i]->value);
      }
   }
}

/**
 * Utility function to free a linked list node that has an lfu_item struct as its data
 */
void free_item_node(void *ptr)
{
    free_node((node *)ptr, free);
}

/** 
 * Allocate space for the LFUCache data structure.  Returns a pointer to the new
 * structure.
 */
LFUCache *lFUCacheCreate(int capacity)
{
    // ensure capacity in hash set is 150% of the requested size
    // ensures that load rate never goes above 66%
    int size = capacity + (capacity >> 1);

    LFUCache *obj = malloc(sizeof(LFUCache));

    // allocate the freq -> linked list map
    // * note that deleting a key from the freq map only frees the linked list
    // * struct itself, and not the actual nodes of the list
    // * since the lfu cache never deletes keys from the freq list and just leaves
    // * the linked lists empty if there are no nodes of that frequency, this is not a problem
    obj->freq_map = allocate_map(size, hash_int, equal_int, NULL, free);
    // allocate the item key -> linked list node map
    obj->item_map = allocate_map(size, hash_int, equal_int, NULL, free_item_node);
    // initialize sizes
    obj->size = 0;
    obj->min_freq = 1;
    obj->capacity = capacity;

    // ensure the list at freq 1 is not null, useful so no checking
    // is required when inserting
    map_insert(obj->freq_map, 1, create_linked_list());

    return obj;
}

/**
 * Returns the value associated with the integer key and updates it frequency, 
 * increasing it by one.
 */
int lFUCacheGet(LFUCache *obj, int key)
{   
    if(obj->capacity == 0) {
        return -1;
    }

    node *temp;
    if((temp = (node *)map_get(obj->item_map, key)) == NULL) {
        return -1;
    }

    pop_node(map_get(obj->freq_map, ((lfu_item *)temp->data)->freq), temp);
    if(is_empty(map_get(obj->freq_map, ((lfu_item *)temp->data)->freq))) {
        map_delete(obj->freq_map, ((lfu_item *)temp->data)->freq);
    }
    

    ((lfu_item *)temp->data)->freq++;

    linked_list *update;
    if((update = map_get(obj->freq_map, ((lfu_item *)temp->data)->freq)) == NULL) {
        update = create_linked_list();
        push_back(update, temp);
        map_insert(obj->freq_map, ((lfu_item *)temp->data)->freq, update);
    } else {
        push_back(update, temp);
    }

    while(1) {
        if(map_contains(obj->freq_map, obj->min_freq)) {
            if(((linked_list*)map_get(obj->freq_map, obj->min_freq))->num_nodes == 0) {
                obj->min_freq++;
            } else {
                break;
            }
        } else {
            obj->min_freq++;
        }
    }

    // while(!map_contains(obj->freq_map, &obj->min_freq) || ((linked_list*)map_get(obj->freq_map, &obj->min_freq))->num_nodes == 0) {
    //     obj->min_freq++;
    // }
    return ((lfu_item *)temp->data)->value;
}

/**
 * Internal function to evict the least frequently used member of the cache
 */
void lFUCacheEvict(LFUCache *obj)
{
    // remove the node from the min_freq linked list
    linked_list *min_freq_list = map_get(obj->freq_map, obj->min_freq);
    node *removed_node = pop_front(min_freq_list);

    // remove the node from the item map
    // this call also frees the node as well
    map_delete(obj->item_map, ((lfu_item *)removed_node->data)->key);
    obj->size--;
}

/**
 * Put the specified integer key value pair into the cache.  Evicts the least 
 * frequently used member if at capacity.  Ties are broken between members with
 * the same frequency by evicting the least recently used member.
 */
void lFUCachePut(LFUCache *obj, int key, int value)
{
    if(obj->capacity == 0) {
        return;
    }

    node *old_node = map_get(obj->item_map, key);
    if (old_node != NULL)
    {
        update_lfu_item((lfu_item *)old_node->data, key, value);
        lFUCacheGet(obj, key);
        return;
    }

    lfu_item *new_item = create_lfu_item(key, value);
    if (obj->size == obj->capacity)
    {
        lFUCacheEvict(obj);
    }

    node *new_node = create_node(new_item);

    // need to check for nullptr now since we are deleting linked
    //lists in freq_map as we go
    linked_list *freq_list;
    if ((freq_list = map_get(obj->freq_map, new_item->freq)) == NULL) {
        freq_list = create_linked_list();
        map_insert(obj->freq_map, 1, freq_list);
    }
    
    push_back(freq_list, new_node);

    map_insert(obj->item_map, new_item->key, new_node);

    obj->size++;
    obj->min_freq = 1;
}

/**
 * Free the space used by the LFUCache data structure at obj pointer.
 */
void lFUCacheFree(LFUCache *obj)
{
    // free the item map and the associated nodes
    free_map(obj->item_map);
    // now free the freq map and the now empty linked list structs
    free_map(obj->freq_map);
    // free the lfu struct itself
    free(obj);
}