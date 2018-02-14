#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linked_list.h"

//Create list
list* list_create()
{
    list* new_list = malloc(sizeof(list));

    if(new_list == NULL)
        return NULL;

    new_list->head = NULL;
    new_list->length = 0;
    return new_list;
}

//Create node
li_node* li_new(void* input, size_t in_size)
{

    if(input == NULL) return NULL;

    li_node* new_node = malloc(sizeof(li_node));
    if(new_node == NULL) return NULL;

    new_node->data = malloc(in_size);
    if(new_node->data == NULL) return NULL;


    memcpy(new_node->data,input,in_size);

    new_node->size = in_size;
    new_node->next = NULL;
    new_node->prev = NULL;
    return new_node;
}

//Add item to front of list
li_node* li_prepend(list* in_list, void* input, size_t in_size)
{
    if(in_list == NULL || input == NULL) return NULL;

    li_node* new_node = li_new(input,in_size);

    if(new_node == NULL) return NULL;

    if(in_list->head == NULL) 
    {
        in_list->head = new_node;
        in_list->length++;
    }
    else 
    {
        new_node->next = in_list->head;
        in_list->head->prev = new_node;
        in_list->head = new_node;
        in_list->length++;
    }
    
    return in_list->head;
}

//Add item to back of list
li_node* li_append(list* in_list, void* input, size_t in_size)
{   
    if(in_list == NULL || input == NULL) return NULL;

    li_node* new_node = li_new(input, in_size);
    if(new_node == NULL) return NULL;

    if(in_list->head == NULL) 
    {
        in_list->head = new_node;
    }
    else
    {
        li_node* cursor = in_list->head;

        while(cursor->next != NULL)
            cursor = cursor->next;

        cursor->next = new_node;
        new_node->prev = cursor;
    }
    
    in_list->length++;
    
    return in_list->head;
}

//Remove front of list (dequeue)
li_node* li_remove_front(list* in_list)
{
    if(in_list->head == NULL)
        return NULL;

    li_node* cursor = in_list->head;
    in_list->head = in_list->head->next;
    in_list->head->prev = NULL;
    in_list->length--;

    free(cursor->data);
    free(cursor);

    return in_list->head;
}

//Remove end of list (pop)
li_node* li_remove_end(list* in_list)
{
    if(in_list->head == NULL)
    return NULL;

    li_node* cursor = in_list->head;
    li_node* back = NULL;    
    
    while(cursor->next != NULL)
    {
        back = cursor;
        cursor = cursor->next;
    }

    if(back != NULL)
        back->next = NULL;

    in_list->length--;
    free(cursor->data);
    free(cursor);
    
    return in_list->head;
}

//Delete node from list and add links between previous and next nodes
void li_delete_node(list* in_list, li_node* in_node) {

    if(in_list == NULL || in_node == NULL)
        return;

    else if(in_node->next == NULL && in_node->prev == NULL) {
        ;
    }

    else if(in_node->next == NULL && in_node->prev != NULL) {
        in_node->prev->next = NULL;
    }

    else if(in_node->prev == NULL && in_node->next != NULL) {
        in_node->next->prev = NULL;
        in_list->head = in_node->next;
    }

    else if(in_node->prev != NULL & in_node->next != NULL) {
        in_node->prev->next = in_node->next;
        in_node->next->prev = in_node->prev;
    }

    free(in_node->data);
    free(in_node);
    in_node = NULL;

    in_list->length--;
}

//Print out list
void li_print(list* in_list, void* (*print_function)(void* data))
{
    if(in_list->head == NULL)
        return;

    printf("\n%d List Items:\n", in_list->length);

    li_node* temp = in_list->head;

    while(temp != NULL)
    {
        (*print_function)(temp->data);
        temp = temp->next;
    }
    printf("\n");

}

// Discard the entire list (free memory);
void li_discard(list* in_list)
{   
    if(in_list == NULL) {
        return;
    }

    //0 sized element list
    if(in_list->head == NULL) {
        free(in_list);
        return;
    }

    //Single element sized list
    if(in_list->head->next == NULL)
    {
        free(in_list->head->data);
        free(in_list->head);
        free(in_list);
        return;
    }
    
    li_node* temp = in_list->head;

    while(temp != NULL)
    {   
        li_node* to_free = temp;
        temp = temp->next;
        free(to_free->data);
        free(to_free);
    }

    free(in_list);
    return;
}
//Search string list for string
li_node* li_string_search(list* in_list, li_node* head, char* input_value)
{
    if(in_list->head == NULL)
        return NULL;

    if(head == NULL)
        head = in_list->head;

	li_node* temp = head;


    char* data_string = malloc(sizeof(temp->size));
    memcpy(data_string, temp->data, temp->size);
    memcpy(data_string,temp->data, temp->size);

	if(!strcmp(input_value, data_string))
	{
		//printf("%s is in list.\n", temp->value);
        free(data_string);
		return temp;
	}
    free(data_string);


	if(temp->next != NULL)
		return li_string_search(in_list, temp->next, input_value);

	//printf("%s is not in list\n", input_value);
	return NULL;

}

//Search string list for data
li_node* li_search(list* in_list, li_node* head, void* input_value, size_t input_size)
{
    if(in_list->head == NULL)
        return NULL;

    if(head == NULL)
        head = in_list->head;

	li_node* temp = head;

	if(temp->size == input_size && !memcmp(temp->data,input_value, input_size))
	{
		//printf("%s is in list.\n", temp->value);
		return temp;
	}

	if(temp->next != NULL)
		return li_search(in_list, temp->next, input_value, input_size);

	//printf("%s is not in list\n", input_value);
	return NULL;

}

//Run function for each element in string list (function takes node as input)
void li_foreach(list* in_list, void* (*func)(list* in_list, li_node* input, void* data), void* data)
{
    //printf("list_length: %d\n", in_list->length);
    if(in_list->head == NULL || in_list->length == 0)
        return;

    int count = 0;

    li_node* temp = in_list->head;
    while(temp != NULL) {
        
    
        li_node* next = temp->next; //Save next pointer because node might be deleted
 
        (*func)(in_list, temp, data);

        count++;

        if(next == NULL) return;
        temp = next;
    }
    return;
}