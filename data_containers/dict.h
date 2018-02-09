//Binary tree node
typedef struct bt_node {
   char* key;
   void*  data;
   size_t size;
   struct bt_node* left;
   struct bt_node* right;
} bt_node;

//Dictionary structure
typedef struct dict {
    bt_node* head;
    int size;
} dict;

//Binary Tree Node Functions
bt_node* bt_node_create(char* in_key, void* in_data, size_t in_size); //Create node with key and value
bt_node* bt_node_search(bt_node* head, char* key); //Search for node with key, if not in list, return where value should be
void bt_nodes_print(bt_node* in_node, int verbosity); //Prints nodes
bt_node* bt_node_remove(bt_node* in_head, char* in_key, int keep_data); //Removes node properly from tree
int bt_node_free(bt_node* in_node, void* data); //Frees node memory
int bt_node_free_keep_data(bt_node* in_node, void* data); //Frees node memory - keeps data
int bt_foreach(bt_node* in_node, int (*func)(bt_node* current_node, void* data),void* data); //Run given function for 
int bt_print_key(bt_node* in_node); //Print key in node

//Dictinary Functions
dict* dict_create(); //Create Dictionary
int dict_insert(dict* in_dict, char* in_key, void* in_data, size_t in_size); //Insert key:value pair
void* dict_access(dict* in_dict, char* in_key); //Access element with key
int dict_del_elem(dict* in_dict, char* in_key, int keep_data); //Delete element with key
int dict_discard(dict* in_dict); //Delete entire dictionary 
void dict_print(dict* in_dict, int verbosity); //Print out dictionary
int dict_foreach(dict* in_dict, int (*func)(bt_node* current_node, void* data), void* data); //Run given function on all dict element

#pragma once
