#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Binary tree node
typedef struct bt_node {
   char* key;
   int value;
   struct bt_node* left;
   struct bt_node* right;
} bt_node;

//Create binary tree node
bt_node* bt_node_create(char* in_key, int in_value) {
    
    bt_node* temp = malloc(sizeof(bt_node));
    temp->key = malloc(strlen(in_key));
    strcpy(temp->key, in_key);
    temp->value = in_value;
    temp->right = NULL;
    temp->left = NULL;

    return temp;
}

//Return node with matching key, other wise return node key should go under
bt_node* bt_node_search(bt_node* head, char* key) {

    if(strcmp(head->key,key) == 0)
        return head;

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

//Parent of node with matching key
bt_node* bt_node_parsearch(bt_node* head, char* key) {

    if(strcmp(head->left->key,key) == 0 || strcmp(head->right->key,key) == 0)
        return head;

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
        printf("[%s: %d]: ", in_node->key, in_node->value);
        if(in_node->left != NULL) printf("->[%s: %d]",in_node->left->key, in_node->left->value); else printf("->[]");
        if(in_node->right != NULL) printf("->[%s: %d]",in_node->right->key, in_node->right->value); else printf("->[]");
        printf("\n");
    }

    bt_nodes_print(in_node->left, verbosity);
    bt_nodes_print(in_node->right, verbosity);

}

//Find minimum under given node in BST
bt_node* bt_node_find_min(bt_node* in_node) {
    if(in_node->left == NULL)
        return in_node;
    return bt_node_find_min(in_node->left);
}


//Remove node with given key (left: 0, right: 1)
int bt_node_remove(bt_node* in_node, bt_node* parent, int left_or_right) {

    if(in_node->left == NULL && in_node->right == NULL) {
        free(in_node);
    }

    if(parent == NULL)
        printf("root_node");


    if(left_or_right) printf("Pointer from right\n"); else printf("Pointer from left\n");




    return 0;
}

//Dictionary structure
typedef struct dict {
    bt_node* head;
    int size;
} dict;

//Create dictionary
dict* dict_create() {
    dict* temp = malloc(sizeof(dict));
    temp->size = 0;
    temp->head = NULL;
    return temp;
}

//Insert given key: value pair into dictionary
int dict_insert(dict* in_dict, char* in_key, int in_value) {

    if(in_dict->head == NULL) {
        in_dict->head = bt_node_create(in_key, in_value);
        in_dict->size++;
        return 1;
    }
    
    bt_node* temp = bt_node_search(in_dict->head, in_key);

    if(!strcmp(temp->key, in_key)) {
        return 0;
    }
    if(strcmp(temp->key, in_key) < 0) {
        temp->right = bt_node_create(in_key, in_value);
        in_dict->size++;
        return 1;
    }
    if(strcmp(temp->key, in_key) > 0) {
        temp->left = bt_node_create(in_key, in_value);
        in_dict->size++;
        return 1;
    }

    return 0;
}

//Get value at key in dictionary
int* dict_access(dict* in_dict, char* in_key) {

    bt_node* temp = bt_node_search(in_dict->head, in_key);
    if(!strcmp(temp->key, in_key))
        return &temp->value;
    else
        return NULL;
}

//Delete element in dictionary
int dict_del_elem(dict* in_dict, char* in_key) {

    printf("TO DELETE: %s\n", in_key);

    bt_node* parent = bt_node_parsearch(in_dict->head, in_key);
    bt_node* to_delete;
    int left_or_right = 2;

    if(parent == NULL) {
        printf("no parent!\n");
        to_delete = bt_node_search(in_dict->head, in_key);
    }
    else if(parent->left != NULL) {
        to_delete = (!strcmp(parent->left->key, in_key)) ? parent->left : parent->right;
        left_or_right = (!strcmp(parent->left->key, in_key)) ? 0 : 1;
    }
    else if(parent->right != NULL) {
        to_delete = (!strcmp(parent->right->key, in_key)) ? parent->right : parent->left;
        left_or_right = (!strcmp(parent->right->key, in_key)) ? 1 : 0;
    }

    printf("FOUND...\n");

    /*
    if(to_delete == NULL || strcmp(to_delete->key, in_key))
        return 0;*/

    if(parent != NULL) printf("FOUND PARENT: %s\n", parent->key);
    printf("FOUND TO DELETE: %s\n", to_delete->key);

    bt_node_remove(to_delete,parent, left_or_right);

    return 0;
}

//Print out dictionary
void dict_print(dict* in_dict, int verbosity) {
    printf("PRINTING DICT WITH %d ELEMENTS: \n", in_dict->size);
    bt_nodes_print(in_dict->head, verbosity);
    printf("\n");
    return;
}

//Discard entire dictionary
int dict_discard(dict* in_dict) {

    bt_node* temp = in_dict->head;

    if(temp == NULL)
        return 0;
    


    return 0;
}

int main(void) {

    //Create dictionary and add elements
    dict* my_dict = dict_create();
    dict_insert(my_dict, "S", 80);
    dict_insert(my_dict, "E", 58);
    dict_insert(my_dict, "A", 75);
    dict_insert(my_dict, "R", 34);
    dict_insert(my_dict, "H", 63);
    dict_insert(my_dict, "C", 27);
    dict_insert(my_dict, "X", 99);
    dict_insert(my_dict, "M", 33);

    dict_print(my_dict, 0);

    //Try to access values
    int* ret = dict_access(my_dict,"S");
    if(ret != NULL) printf("VALUE: %d\n", *ret); else printf("NOT IN DICT\n"); 
    ret = dict_access(my_dict,"C");
    if(ret != NULL) printf("VALUE: %d\n", *ret); else printf("NOT IN DICT\n");
    ret = dict_access(my_dict,"Y");
    if(ret != NULL) printf("VALUE: %d\n", *ret); else printf("NOT IN DICT\n");
    printf("\n");

    //Delete element
    dict_del_elem(my_dict, "C");
    
    
    dict_print(my_dict, 0);

    return 0;

};

