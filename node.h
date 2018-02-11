//Message item structure
typedef struct message_item {
    char toWhom[300];
    char message[30000];
    unsigned int tries;
} message_item;

//Socket struct
typedef struct socket_item {
    int socket;
    unsigned int last_used;
} socket_item;

//UTILITY FUNCTIONS
int print_balance(bt_node* current_node, void* data);
int print_keys(bt_node* current_node, void* data);
void* print_queue(void* input);
void setup_message(message_item* in_message);
int destroy_chains_in_dict(bt_node* current_node, void* data);
int destroy_sockets_in_dict(bt_node* current_node, void* data);
void* print_string(void* data);

//CORE FUNCTIONS
int mine();
void* announce_length(list* in_list, li_node* in_item, void* data);
void* announce_existance(list* in_list, li_node* in_item, void* data);
void* announce_message(list* in_list, li_node* in_item, void* data);
void* send_node_list_to(list* in_list, li_node* in_item, void* data);
int insert_trans(const char* input);
int verify_post_format(const char* post);
int insert_post(const char* input);
int create_socket(const char* input);
int register_new_node(const char* input);
int save_chain_to_file(blockchain* in_chain, const char* file_name);
int verify_file_block(const char* input, int* curr_index);
int read_chain_from_file(blockchain* in_chain, const char* file_name);
int request_chain(const char* address);
int compare_length(const char* input);
int send_our_chain(const char* address);
int prune_chains(bt_node* current_node, void* data);
int prune_sockets(bt_node* current_node, void* data);
int verify_foreign_block(const char* input);
void process_message(const char* in_msg);
int verify_acceptance_trans_or_post(const char* input);
int ping_function();
int client_existance_announcement(const char* input);
void* in_server();
void* destroy_items_in_dict(list* in_list, li_node* input , void* data);
void* inbound_executor();
void* process_inbound(list* in_list, li_node* input, void* data);
void* out_server();
int rare_socket(message_item* in_message);
void* process_outbound(list* in_list, li_node* input, void* data);
int read_node_list();
void* li_write_string_file(list* in_list, li_node* in_node, void* data);
int write_config();
void graceful_shutdown(int dummy);
int setup_ip_address(const char* in_ip_address);
int setup_pri_key(const char* in_pri_key);
int setup_pub_key(const char* in_pub_key);
int setup_node_list(const char* in_node_list);
int setup_chain_file(const char* in_chain_file);
int command_line_parser(int argc, const char* argv[]);








