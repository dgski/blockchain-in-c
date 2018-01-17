#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "linked_list.h"

//Create list
list* list_create()
{
    list* our_list = malloc(sizeof(list));
    our_list->head = NULL;
    our_list->length = 0;
    return our_list;
}

//Create node in list
li_node* li_new(void* input, size_t in_size)
{
    li_node* temp = malloc(sizeof(li_node));
    temp->data = malloc(in_size);
    memcpy(temp->data,input,in_size);

    temp->size = in_size;
    temp->next = NULL;
    temp->prev = NULL;
    return temp;
}

//Add item to front of list
li_node* li_prepend(list* in_list, void* input, size_t in_size)
{
    li_node* temp = li_new(input,in_size);

    if(in_list->head == NULL) 
    {
        in_list->head = temp;
    }
    else 
    {
        temp->next = in_list->head;
        in_list->head->prev = temp;
        in_list->length++;
    }
    
    in_list->head = temp;
    return in_list->head;
}

//Add item to back of list
li_node* li_append(list* in_list, void* input, size_t in_size)
{
    li_node* temp = li_new(input, in_size);

    if(in_list->head == NULL) 
    {
        in_list->head = temp;
    }
    else
    {
        li_node* cursor = in_list->head;
        while(cursor->next != NULL)
            cursor = cursor->next;
        cursor->next = temp;
        temp->prev = cursor;
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

    if(in_node == NULL)
        return;

    if(in_node->next == NULL && in_node->prev == NULL)
        ;

    if(in_node->next == NULL && in_node->prev != NULL)
        in_node->prev->next = NULL;

    if(in_node->prev == NULL && in_node->next != NULL)
        in_node->next->prev = NULL;

    else if(in_node->prev != NULL & in_node->next != NULL) {
        in_node->prev->next = in_node->next;
        in_node->next->prev = in_node->prev;
    }

    in_list->length--;
    free(in_node);
}

//Print out list
void li_print(list* in_list, void* (*print_function)(void* data))
{
    if(in_list->head == NULL)
        return;

    printf("%d List Items:\n", in_list->length);

    li_node* temp = in_list->head;

    while(temp != NULL)
    {
        int a = (int)(*print_function)(temp->data);
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
void li_foreach(list* in_list, void* (*func)(list* in_list, li_node* input))
{
    if(in_list->head == NULL)
        return;

    li_node* temp = in_list->head;
    while(temp != NULL) {
        int to_delete = (int)(*func)(in_list, temp);

        if(to_delete)
            li_delete_node(in_list,temp);

        temp = temp->next;
    }
    return;
}