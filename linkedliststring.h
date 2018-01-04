#include <stdbool.h>

//Node in String List
typedef struct strli_node {
    char* value;
    struct strli_node* next;
    struct strli_node* prev;
} strli_node;

//String List
typedef struct strlist {
    strli_node* head;
    int length;
} strlist;

//String list functions
strlist* create_strlist();
strli_node* strli_create_node(char* input_string);
strli_node* strli_prepend(strlist* in_list, char* input_value);
strli_node* strli_append(strlist* in_list, char* input_value);
strli_node* strli_remove_front(strlist* in_list);
strli_node* strli_remove_end(strlist* in_list);
void strli_delete_node(strlist* in_list, strli_node* in_node);
void strli_print(strlist* in_list);
void strli_discard(strlist* in_list);
strli_node* strli_search(strlist* in_list, strli_node* head, char* input_value);
void strli_foreach(strlist* in_list, void* (*func)(strli_node* input));

