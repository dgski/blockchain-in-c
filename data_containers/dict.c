#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dict.h"

//Create binary tree node
bt_node* bt_node_create(char* in_key, void* in_data, size_t in_size) {

    if(in_key == NULL || in_data == NULL) return NULL;
    
    bt_node* temp = malloc(sizeof(bt_node));
    temp->key = malloc(strlen(in_key) +1);
    strcpy(temp->key, in_key);
    temp->data = malloc(in_size);
    memcpy(temp->data, in_data, in_size);
    temp->size = in_size;
    temp->right = NULL;
    temp->left = NULL;

    return temp;
}


//Return node with matching key, other wise return node key should go under
bt_node* bt_node_search(bt_node* head, char* key) {

    if(head == NULL || head->key == NULL || key == NULL) return NULL;

    if(!strcmp(head->key,key)) {
        return head;
    }

    if(strcmp(head->key, key) < 0) {
        if(head->right == NULL)
            return head;
        return bt_node_search(head->right, key);
    }

    if(strcmp(head->key, key) > 0) {
        if(head->left == NULL)
            return head;
        return bt_node_search(head->left, key);
    }

    return NULL;
}


//Print each node and its two subnodes
void bt_nodes_print(bt_node* in_node, int verbosity) {

    if(in_node == NULL)
        return;

    if(!verbosity) {
        printf("%s: ", in_node->key);
        if(in_node->left != NULL) printf("%s",in_node->left->key); else printf("_");
        if(in_node->right != NULL) printf("%s",in_node->right->key); else printf("_");
        printf("\n");
    }
    else {
        printf("[%s]: ", in_node->key);
        if(in_node->left != NULL) printf("->[%s]",in_node->left->key); else printf("->[]");
        if(in_node->right != NULL) printf("->[%s]",in_node->right->key); else printf("->[]");
        printf("\n");
    }

    bt_nodes_print(in_node->left, verbosity);
    bt_nodes_print(in_node->right, verbosity);

}

int bt_node_free(bt_node* in_node, void* data) {

    if(in_node == NULL) return 0;

    free(in_node->data);
    free(in_node->key);
    free(in_node);

    return 1;
}

int bt_node_free_keep_data(bt_node* in_node, void* data) {

    if(in_node == NULL) return 0;

    free(in_node->key);
    free(in_node);

    return 1;
}

//Remove node with given key (left: 0, right: 1)
bt_node* bt_node_remove(bt_node* in_head, char* in_key, int keep_data) {

    if(in_head == NULL) return in_head;
    else if(strcmp(in_head->key,in_key) < 0) in_head->right = bt_node_remove(in_head->right, in_key, keep_data);
    else if(strcmp(in_head->key,in_key) > 0) in_head->left =  bt_node_remove(in_head->left, in_key, keep_data);
    else {

        //Both Children are NULL
        if(in_head->left == NULL && in_head->right == NULL) {
            if(keep_data == 0) bt_node_free(in_head, NULL);
            else bt_node_free_keep_data(in_head, NULL);
            in_head = NULL;
        }

        //One Child is NULL
        else if(in_head->left == NULL) {
            bt_node* temp = in_head;
            in_head = in_head->right;
            if(keep_data == 0) bt_node_free(temp, NULL);
            else bt_node_free_keep_data(in_head, NULL);

        }
        //One Child is NULL
        else if(in_head->right == NULL) {
            bt_node* temp = in_head;
            in_head = in_head->left;
            if(keep_data == 0) bt_node_free(temp, NULL);
            else bt_node_free_keep_data(in_head, NULL);

        }
        //Both children are real
        else  {
            bt_node* temp = in_head->right;

            //Find minimum value on right side
            while(temp->left != NULL) temp = temp->left;
            

            //Copy that minimums values to 'in_head'
            bt_node* replacement = bt_node_create(temp->key,temp->data,temp->size);
            replacement->left = in_head->left;
            replacement->right = in_head->right;
            if(keep_data == 0) bt_node_free(in_head, NULL);
            else bt_node_free_keep_data(in_head, NULL);

            in_head = replacement;

            //Delete minumum
            in_head->right = bt_node_remove(in_head->right, in_head->key, keep_data);
        }
    }
    return in_head;
}


//Create dictionary
dict* dict_create() {
    dict* temp = malloc(sizeof(dict));
    temp->size = 0;
    temp->head = NULL;
    return temp;
}

//Insert given key: value pair into dictionary
int dict_insert(dict* in_dict, char* in_key, void* in_data, size_t in_size) {

    if(in_dict->head == NULL) {
        in_dict->head = bt_node_create(in_key, in_data, in_size);
        in_dict->size++;
        return 1;
    }
    
    bt_node* temp = bt_node_search(in_dict->head, in_key);

    if(temp == NULL) return 0;

    //Replace value already in there
    if(!strcmp(temp->key, in_key)) {
        free(temp->data);
        temp->data = malloc(in_size);
        temp->size = in_size;
        memcpy(temp->data,in_data, in_size);
        return 1;
    }
    if(strcmp(temp->key, in_key) < 0) {
        temp->right = bt_node_create(in_key, in_data, in_size);
        in_dict->size++;
        return 1;
    }
    if(strcmp(temp->key, in_key) > 0) {
        temp->left = bt_node_create(in_key, in_data, in_size);
        in_dict->size++;
        return 1;
    }

    return 0;
}

//Get value at key in dictionary
void* dict_access(dict* in_dict, char* in_key) {

    bt_node* temp = bt_node_search(in_dict->head, in_key);
    if(temp == NULL)
        return NULL;

    if(!strcmp(temp->key, in_key)) {
        int* info = (int*)temp->data;
        return temp->data;
    }
    else
        return NULL;
}

//Delete element in dictionary
int dict_del_elem(dict* in_dict, char* in_key, int keep_data) {

    if(in_dict == NULL || in_key == NULL || dict_access(in_dict, in_key) == NULL) return 0;

    in_dict->head = bt_node_remove(in_dict->head,in_key, keep_data);
    in_dict->size--;

    return 1;
}

//Discard entire dictionary
int dict_discard(dict* in_dict) {

    //Free all nodes and their memory
    dict_foreach(in_dict,bt_node_free,NULL);

    free(in_dict);

    return 0;
}

//Print out dictionary
void dict_print(dict* in_dict, int verbosity) {
    printf("PRINTING DICT WITH %d ELEMENTS: \n", in_dict->size);
    bt_nodes_print(in_dict->head, verbosity);
    printf("\n");
    return;
}

int bt_print_key(bt_node* in_node) {

    printf("%s, ",in_node->key);

    return 1;
}


int bt_foreach(bt_node* in_node, int (*func)(bt_node* current_node, void* data),void* data) {

    //Run function on left side
    if(in_node->left != NULL)
        bt_foreach(in_node->left,func,data);

    //Save right side location
    bt_node* temp_right = in_node->right;
    
    //Run function on your self
    if(in_node != NULL) func(in_node,data);

    //Run function on right side
    if(temp_right != NULL)
        bt_foreach(temp_right,func,data);

    return 1;
}



int dict_foreach(dict* in_dict, int (*func)(bt_node* current_node, void* data), void* data) {

    if(in_dict == NULL || in_dict->head == NULL) return 0;

    //Run for each for head
    bt_foreach(in_dict->head,func,data);

    return 1;

}



