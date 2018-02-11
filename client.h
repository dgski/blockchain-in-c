//Message item structure
typedef struct client_message_item {
    char toWhom[300];
    char message[2000];
    unsigned int tries;
    int status; //0 - unsent, 1 - sent, 2 - accepted
    char sig[513];
    int type;
} client_message_item;

//Holds a sent object
typedef struct p_thing {
    int status;
    int amount;
    dict* confirmed_by;
} p_thing;

p_thing* create_p_thing();
void destroy_p_thing(p_thing* in_thing);
void setup_message(client_message_item* in_message);
void display_help();
void quit_program();
int val_trans_format(char* recipient, char* amount);
int get_signature(char* output, char* input);
int register_new_node(char* input);
void* send_to_all(list* in_list, li_node* in_item, void* data);
void post_post(char* input);
void post_transaction(char* input);
int read_config2();
void* process_outbound(list* in_list, li_node* input, void* data);
int check_on_unaccepted(bt_node* current_node, void* data);
void* out_server();
int print_posts_trans(bt_node* current_node, void* data);
void print_posted_items();
int trans_or_post_acceptance( char* input);
void process_message(const char* in_msg);
void* in_server();