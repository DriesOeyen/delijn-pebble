#pragma once

#include <stdio.h>
#include <stdlib.h>

extern int list_errno;
typedef void* data_ptr_t;
typedef struct list_item list_t;
typedef list_t* list_ptr_t;

list_ptr_t list_alloc ( void );
void list_free_all( list_ptr_t* list );
int list_size( list_ptr_t list );
list_ptr_t list_insert_at_index( list_ptr_t list, data_ptr_t data, int index);
list_ptr_t list_remove_at_index( list_ptr_t list, int index);
list_ptr_t list_free_at_index( list_ptr_t list, int index);
list_ptr_t list_get_reference_at_index( list_ptr_t list, int index );
data_ptr_t list_get_data_at_index( list_ptr_t list, int index );
int list_get_index_of_data( list_ptr_t list, data_ptr_t data );