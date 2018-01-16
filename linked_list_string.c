#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "linked_list_string.h"

//Create list of strings
strlist* create_strlist()
{
    strlist* list = malloc(sizeof(strlist));
    list->head = NULL;
    list->length = 0;
    return list;
}

//Create node in string list
strli_node* strli_create_node(char* input_string)
{
    strli_node* temp = malloc(sizeof(strli_node));
    temp->value = malloc(strlen(input_string) + 1);
    memset(temp->value, 0, strlen(input_string));
    strcpy(temp->value, input_string);
    temp->next = NULL;
    temp->prev = NULL;
    return temp;
}

//Add string to front of list
strli_node* strli_prepend(strlist* in_list, char* input_value)
{
    strli_node* temp = strli_create_node(input_value);

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

//Add string to back of string list
strli_node* strli_append(strlist* in_list, char* input_value)
{
    strli_node* temp = strli_create_node(input_value);

    if(in_list->head == NULL) 
    {
        in_list->head = temp;
    }
    else
    {
        strli_node* cursor = in_list->head;
        while(cursor->next != NULL)
            cursor = cursor->next;
        cursor->next = temp;
        temp->prev = cursor;
    }
    
    in_list->length++;
    
    return in_list->head;
}

//Remove front of string list (dequeue)
strli_node* strli_remove_front(strlist* in_list)
{
    if(in_list->head == NULL)
        return NULL;

    strli_node* cursor = in_list->head;
    in_list->head = in_list->head->next;
    in_list->head->prev = NULL;
    in_list->length--;
    free(cursor->value);
    free(cursor);
    return in_list->head;
}

//Remove end of string list (pop)
strli_node* strli_remove_end(strlist* in_list)
{
    if(in_list->head == NULL)
    return NULL;

    strli_node* cursor = in_list->head;
    strli_node* back = NULL;    
    
    while(cursor->next != NULL)
    {
        back = cursor;
        cursor = cursor->next;
    }

    if(back != NULL)
        back->next = NULL;

    in_list->length--;
    free(cursor->value);
    free(cursor);
    
    return in_list->head;
}

//Delete node from string list and add links between previous and next nodes
void strli_delete_node(strlist* in_list, strli_node* in_node) {

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

//Print out string list
void strli_print(strlist* in_list)
{
    if(in_list->head == NULL)
        return;

    printf("%d List Items:\n", in_list->length);

    strli_node* temp = in_list->head;

    while(temp != NULL)
    {
        printf("- %s\n",temp->value);
        temp = temp->next;
    }
    printf("\n");

}

// Discard the entire string list (free memory);
void strli_discard(strlist* in_list)
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
        free(in_list->head->value);
        free(in_list->head);
        free(in_list);
        return;
    }
    
    strli_node* temp = in_list->head;

    while(temp != NULL)
    {   
        strli_node* to_free = temp;
        temp = temp->next;
        free(to_free->value);
        free(to_free);
    }

    free(in_list);
}
//Search string list for string
strli_node* strli_search(strlist* in_list, strli_node* head, char* input_value)
{
    if(in_list->head == NULL)
        return NULL;

    if(head == NULL)
        head = in_list->head;

	strli_node* temp = head;

	if(!strcmp(input_value, temp->value))
	{
		//printf("%s is in list.\n", temp->value);
		return temp;
	}

	if(temp->next != NULL)
		return strli_search(in_list, temp->next, input_value);

	//printf("%s is not in list\n", input_value);
	return NULL;

}

//Run function for each element in string list (function takes node as input)
void strli_foreach(strlist* in_list, void* (*func)(strlist* in_list, strli_node* input))
{
    if(in_list->head == NULL)
        return;

    strli_node* temp = in_list->head;
    while(temp != NULL) {
        int to_delete = (int)(*func)(in_list, temp);

        //printf("SHOULE WE DELETE? %d\n", to_delete);

        if(to_delete)
            strli_delete_node(in_list,temp);

        temp = temp->next;
    }
    return;
}