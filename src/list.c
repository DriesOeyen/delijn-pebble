#include "list.h"



typedef struct list_item
{
	data_ptr_t data;
	list_ptr_t prev;
	list_ptr_t next;
}list_t;


int list_errno = 0;
/*ERROR CODES
	0 - Pass
	1 - List could not be allocated
	2 - No match found during search
	3 - Can't remove from chain
	4 - No list present
*/

//Set the error code
void setErrno(int errno){
	list_errno = errno;
}

//Find the first part of a linked list chain
list_ptr_t list_find_first(list_ptr_t list){
	list_ptr_t curList = list;
	while(curList->prev != NULL){
		curList = curList->prev;
	}
	return curList;
}

//Allocate a list, return the pointer
list_ptr_t list_alloc(void){
	//Reset error code
	setErrno(0);

	//Allocate a list
	list_ptr_t list = (list_ptr_t) malloc(sizeof(list_t));
	list->data = NULL;
	list->prev = NULL;
	list->next = NULL;

	//Return pointer
	return list;
}

//Free an entire linked list chain
void list_free_all(list_ptr_t* list){
	//Reset error code
	setErrno(0);

	//Free list item chain
	list_ptr_t curList = list_find_first(*list);
	while(curList->next != NULL){
		free(curList->data);
		curList = curList->next;
		free(curList->prev);
	}
	free(curList->data);
	free(curList); //Free the last list too
	*list = NULL; //Finally, get rid of the pointer to the list
}

//Return the length of a linked list chain
int list_size(list_ptr_t list){
	//Reset error code
	setErrno(0);

	//Calculate chain length and return
	list_ptr_t curList = list_find_first(list);

	int i = 0;
	if(curList->data == NULL && curList->next == NULL){ //If there are no links, return length 0
		return 0;
	}
	while(curList->next != NULL){
		i++;
		curList = curList->next;
	}
	return ++i;
}

//Insert a list at a certain index
list_ptr_t list_insert_at_index(list_ptr_t list, data_ptr_t data, int index){
	//Reset error code
	setErrno(0);

	//Check if list exists
	if(list == NULL){
		setErrno(4);
		return NULL;
	}

	//Find the position
	list_ptr_t curList = list_get_reference_at_index(list, index);
	list_ptr_t prevList;
	list_ptr_t nextList;

	//Update chain
	if(list_size(curList) == 0){ //First insert
		//No updates to chain needed because there's no chain yet
	} else if(index <= 0){ //Insert at first position
		nextList = curList;
		curList->prev = list_alloc();
		curList = curList->prev;
		curList->next = nextList;
	} else if(index >= list_size(list)){ //Insert at last position
		prevList = curList;
		curList->next = list_alloc();
		curList = curList->next;
		curList->prev = prevList;
	} else{ //Insert at any other position
		prevList = curList->prev;
		nextList = curList;
		curList->prev = list_alloc();
		curList = curList->prev;
		curList->prev = prevList;
		curList->next = nextList;
		prevList->next = curList;
	}

	//Write pointer
	curList->data = data;

	//Return the first item of the list chain
	return list_find_first(curList);
}

//Remove an item from the list chain
list_ptr_t list_remove_at_index(list_ptr_t list, int index){
	//Reset error code
	setErrno(0);

	//Chain empty?
	if(list_size(list) <= 1){
		setErrno(3);
		return list;
	}

	//Find the position
	list_ptr_t curList = list_get_reference_at_index(list, index);
	list_ptr_t prevList = curList->prev;
	list_ptr_t nextList = curList->next;

	//Close the chain and return first item of leftover chain
	if(list_size(list) == 0){ //Just one element (which has just been removed)
		return list_find_first(curList);
	} else if(curList->prev == NULL){ //Delete at first position
		nextList->prev = NULL;
		return list_find_first(nextList);
	} else if(curList->next == NULL){ //Delete at last position
		prevList->next = NULL;
		return list_find_first(prevList);
	} else{ //Delete at any other position
		prevList->next = nextList;
		nextList->prev = prevList;
		return list_find_first(nextList);
	}
}

//Remove an item from the list chain and free the memory for the data
list_ptr_t list_free_at_index(list_ptr_t list, int index){
	//Reset error code
	setErrno(0);

	//Chain empty?
	if(list_size(list) == 0){
		setErrno(3);
		return list;
	}

	//Find the position
	list_ptr_t curList = list_get_reference_at_index(list, index);
	list_ptr_t prevList = curList->prev;
	list_ptr_t nextList = curList->next;

	//Free the data
	free(curList->data);
	curList->data = NULL;

	//Close the chain and return first item of leftover chain
	if(list_size(list) == 0){ //Just one element (which has just been removed)
		return list_find_first(curList);
	} else if(curList->prev == NULL){ //Delete at first position
		nextList->prev = NULL;
		return list_find_first(nextList);
	} else if(curList->next == NULL){ //Delete at last position
		prevList->next = NULL;
		return list_find_first(prevList);
	} else{ //Delete at any other position
		prevList->next = nextList;
		nextList->prev = prevList;
		return list_find_first(nextList);
	}
}

//Get a pointer to the list item at a certain index
list_ptr_t list_get_reference_at_index(list_ptr_t list, int index){
	//Reset error code
	setErrno(0);

	//Find list item and return pointer
	list_ptr_t curList = list_find_first(list);
	int i = 0;
	while(i < index && curList->next != NULL){
		i++;
		curList = curList->next;
	}
	return curList;
}

//Get a pointer to the data of a list item at a certain index
data_ptr_t list_get_data_at_index(list_ptr_t list, int index){
	//Reset error code
	setErrno(0);

	//Find list item and return pointer to its data
	list_ptr_t curList = list_get_reference_at_index(list, index);
	return curList->data;
}

//Search the list for data, return -1 in case of an error
int list_get_index_of_data(list_ptr_t list, data_ptr_t data){
	//Reset error code
	setErrno(0);

	//Start search and return index if match is found
	list_ptr_t curList = list_find_first(list);
	int i = 0;
	while(curList->next != NULL){
		if(curList->data == data){
			return i;
		} else{
			i++;
			curList = curList->next;
		}
	}
	if(curList->data == data){
		return i;
	}

	//No matches
	setErrno(2);
	return -1;
}