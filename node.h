typedef struct message_item {
    char toWhom[120];
    char message[2400];
    unsigned int tries;
} message_item;

//Outbound thread
int mine();

//Inbound thread
int compare_length(char* input);
int verify_foreign_block(char* input);
int request_chain(char* address);
int send_our_chain(char* address);

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


int insert_trans(char* input);
int insert_post(const char* input);
void register_new_node(char* input);


void process_message(const char* in_msg);
int read_config();
void graceful_shutdown(int dummy);
void setup_message(message_item* in_message);




