typedef enum{START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DATA_REC, STOP_RCV} state;

typedef enum{OTHER, FLAG, A, C, BCC, DATA} event;

struct state_machine;

struct state_machine* create_state_machine(char * type);

state getCurrentState(struct state_machine* st);

event getCurrentEvent(struct state_machine* st);

void message_handler(struct state_machine* st);

void setCurrentEvent(struct state_machine* st, event event);
