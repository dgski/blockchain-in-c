#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "linkedliststring.h"

strlist* create_strlist() {
    strlist* list = malloc(sizeof(strlist));
    list->head = NULL;
    list->length = 0;

    return list;
}

strli_node* strli_create_node(char* input_string)
{
    strli_node* temp = malloc(sizeof(strli_node));
    temp->value = malloc(strlen(input_string));
    memset(temp->value, 0, strlen(temp->value));
    strcpy(temp->value, input_string);
    temp->next = NULL;
    return temp;
}

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
        in_list->length++;
    }
    
    in_list->head = temp;
    return in_list->head;
}

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
    }
    
    in_list->length++;
    
    return in_list->head;
}

strli_node* strli_remove_front(strlist* in_list)
{
    if(in_list->head == NULL)
        return NULL;

    strli_node* cursor = in_list->head;
    in_list->head = in_list->head->next;
    in_list->length--;
    free(cursor->value);
    free(cursor);
    return in_list->head;
}

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

void strli_discard(strlist* in_list)
{
    if(in_list->head == NULL)
        return;

    //Single element sized list
    if(in_list->head->next == NULL)
    {
        free(in_list->head->value);
        free(in_list->head);
        return;
    }
    
    
    strli_node* temp = in_list->head;
    temp = temp->next;

    while(in_list->head != NULL)
    {

        free(in_list->head->value);
        free(in_list->head);
        in_list->head = temp;
        if(temp != NULL)
            temp = temp->next;
    }
}

bool strli_search(strlist* in_list, strli_node* head, char* input_value)
{
    if(in_list->head == NULL)
        return NULL;

    if(head == NULL)
        head = in_list->head;

	strli_node* temp = head;

	if(!strcmp(input_value, temp->value))
	{
		printf("%s is in list.\n", temp->value);
		return true;
	}

	if(temp->next != NULL)
		strli_search(in_list, temp->next, input_value);

	printf("%s is not in list\n", input_value);
	return false;

}

void strli_foreach(strlist* in_list, void* (*func)(strli_node* input)) {

    if(in_list->head == NULL)
        return;

    strli_node* temp = in_list->head;
    while(temp != NULL) {
        (*func)(temp);
        temp = temp->next;
    }
    return;

}