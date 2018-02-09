//Doubly-linked list holding void* data by DG

//Node in List
typedef struct li_node {
    void* data;
    size_t size;
    struct li_node* next;
    struct li_node* prev;
} li_node;

//List structure
typedef struct list {
    li_node* head;
    int length;
} list;

//Holds both
typedef struct list_and_node_combo {
    list* the_list;
    li_node* the_node;
    void* the_data;
} list_and_node_combo;

//list functions
list* list_create();
li_node* li_new(void* input, size_t in_size);
li_node* li_prepend(list* in_list, void* input, size_t in_size);
li_node* li_append(list* in_list, void* input, size_t in_size);
li_node* li_remove_front(list* in_list);
li_node* li_remove_end(list* in_list);
void li_delete_node(list* in_list, li_node* in_node);
void li_print(list* in_list, void* (*print_function)(void* data));
void li_discard(list* in_list);
li_node* li_string_search(list* in_list, li_node* head, char* input_value);
li_node* li_search(list* in_list, li_node* head, void* input_value, size_t input_size);
void li_foreach(list* in_list, void* (*func)(list* in_list, li_node* input, void* data), void* data);

