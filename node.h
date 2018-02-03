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

//Outbound thread
int mine();

//Setup functions
int setup_ip_address(char* in_ip_address);
int setup_pri_key(char* in_pri_key);
int setup_pub_key(char* in_pub_key);
int setup_node_list(char* in_node_list);
int setup_chain_file(char* in_chain_file);
int command_line_parser(int argc, char* argv[]);
int read_chain_from_file(blockchain* in_chain, char* file_name);
int verify_file_block(const char* in_block, int* curr_index);

int create_socket(const char* input);

int destroy_sockets_in_dict(bt_node* current_node);


//Inbound thread
int compare_length(const char* input);
int verify_foreign_block(const char* input);
int request_chain(const char* address);
int send_our_chain(const char* address);
int save_chain_to_file(blockchain* in_chain, char* file_name);

//Workers
void* in_server();
void* out_server();
void* inbound_executor();

//List iteration functions
void* process_inbound(list* in_list, li_node* input, void* data);
void* process_outbound(list* in_list, li_node* input, void* data);
void* print_queue(void* data);
void* announce_length(list* in_list, li_node* in_item, void* data);
void* announce_existance(list* in_list, li_node* in_item, void* data);
void* announce_message(list* in_list, li_node* in_item, void* data);


int insert_trans(const char* input);
int insert_post(const char* input);
void register_new_node(const char* input);
int verify_post_format(const char* post);



void process_message(const char* in_msg);
int read_node_list();
void graceful_shutdown(int dummy);
void setup_message(message_item* in_message);
int print_balance(bt_node* current_node);
int print_keys(bt_node* current_node);





