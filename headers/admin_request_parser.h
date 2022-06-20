#ifndef PROTOS2022A_ADMIN_REQUEST_PARSER
#define PROTOS2022A_ADMIN_REQUEST_PARSER

#include "buffer.h"


#define ANSWER_LEN 255
#define USER_LEN 255
#define PASS_LEN 255
#define NEW_BUFF_SIZE 100

#define METRICS_CMD 0
#define ADD_USER_CMD 1 
#define LIST_USERS_CMD 2
#define CHANGE_BUFF_SIZE_CMD 3

//1
//2 name pass
//3
//4 buffsize


enum admin_req_state {
   req_reading_cmd,
   req_reading_metrics_cmd,
   req_reading_add_user_cmd,
   req_reading_list_user_cmd,
   req_reading_change_buff_size_cmd,
   req_reading_user,
   req_reading_pass,
   req_reading_buff_size,
   req_done,           
   req_bad_syntax,
   req_invalid_cmd       
};


struct req_parser {
   enum admin_req_state state;
   char cmd_selected;

   char * user;
   char * password;
   char * new_buff_size; 
   int index; 
};

void
admin_req_parser_init(struct req_parser * pars);

enum admin_req_state
admin_consume_req_buffer(buffer * buff, struct req_parser * pars);

enum admin_req_state
admin_req_parser_feed(uint8_t c, struct req_parser * pars);

void 
admin_req_fill_msg(buffer * buff, struct req_parser * pars);

#endif