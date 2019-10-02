/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include "states.h"

struct state_machine{
	state current_state;
	event current_event;
};

struct state_machine stm;
struct state_machine* create_state_machine()
{
	struct state_machine* st =  malloc(sizeof(stm));
	st->current_state = START;
	st->current_event = OTHER;

	return st;
}


state getCurrentState(struct state_machine* st){
	return st->current_state;
}

event getCurrentEvent(struct state_machine* st){
	return st->current_event;
}

void setCurrentEvent(struct state_machine* st, event event){
	st->current_event = event;
}

void message_handler(struct state_machine* st){

	switch(st->current_state){
	
		case START:
			if(st->current_event == FLAG)
				st->current_state = FLAG_RCV;

			if(st->current_event == OTHER)
				st->current_state = START;
			
			break;

		case FLAG_RCV:
			if(st->current_event == FLAG)
				st->current_state = FLAG_RCV;

			if(st->current_event == OTHER)
				st->current_state = START;

			if(st->current_event == A)
				st->current_state = A_RCV;

			break;

		case A_RCV:
			if(st->current_event == FLAG)
				st->current_state = FLAG_RCV;

			if(st->current_event == OTHER)
				st->current_state = START;

			if(st->current_event == C)
				st->current_state = C_RCV;

			break;


		case C_RCV:
			if(st->current_event == FLAG)
				st->current_state = FLAG_RCV;

			if(st->current_event == OTHER)
				st->current_state = START;

			if(st->current_event == BCC)
				st->current_state = BCC_OK;

			break;


		case BCC_OK:
			if(st->current_event == FLAG)
				st->current_state = STOP_RCV;

			if(st->current_event == OTHER)
				st->current_state = START;

			break;
	}
	
}
