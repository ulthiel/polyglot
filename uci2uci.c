// uci2uci.c

// includes

#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "board.h"
#include "engine.h"
#include "fen.h"
#include "gui.h"
#include "move.h"
#include "move_do.h"
#include "move_legal.h"
#include "parse.h"
#include "option.h"
#include "book.h"
#include "main.h"
#include "uci.h"

// defines

#define StringSize 4096

// variables

static board_t UCIboard[1];
static bool Init=TRUE;
static int SavedMove=MoveNone;

// prototypes

static void send_uci_options();


// normalize_type()

static void normalize_type(char *dst, const char* src){
  if(option_get_int(Option,"UCIVersion") <=2){
    if(IS_STRING(src)){
      strcpy(dst,"string");
      return;
    }else if(IS_SPIN(src)){
      strcpy(dst,"spin");
    }else if(IS_BUTTON(src)){
      strcpy(dst,"button");
    }else{
      strcpy(dst,src);
    }
  }else{
    strcpy(dst,src);
  }
}

// parse_position()

static void parse_position(const char string[]) {

/* This is borrowed from Toga II. This code is quite hacky and will be
   rewritten using the routines in parse.cpp.
*/

   const char * fen;
   char * moves;
   const char * ptr;
   char move_string[256];
   int move;
   char * string_copy;

   // init
   //printf("DEBUG: %s\n", string);

   string_copy=my_strdup(string);

   fen = strstr(string_copy,"fen ");
   moves = strstr(string_copy,"moves ");

   // start position

   if (fen != NULL) { // "fen" present

      if (moves != NULL) { // "moves" present
         ASSERT(moves>fen);
         moves[-1] = '\0'; // dirty, but so is UCI
      }

      board_from_fen(UCIboard,fen+4); // CHANGE ME

   } else {

      // HACK: assumes startpos

      board_from_fen(UCIboard,StartFen);
   }

   // moves

   if (moves != NULL) { // "moves" present

      ptr = moves + 6;

      while (*ptr != '\0') {

         while (*ptr == ' ') ptr++;

         move_string[0] = *ptr++;
         move_string[1] = *ptr++;
         move_string[2] = *ptr++;
         move_string[3] = *ptr++;

         if (*ptr == '\0' || *ptr == ' ') {
            move_string[4] = '\0';
         } else { // promote
            move_string[4] = *ptr++;
            move_string[5] = '\0';
         }
         move = move_from_can(move_string,UCIboard);

         move_do(UCIboard,move);

      }
   }
   free(string_copy);
}


// send_book_move()

static void send_book_move(int move){
    char move_string[256];
    my_log("POLYGLOT *BOOK MOVE*\n");
    move_to_can(move,UCIboard,move_string,256);
        // bogus info lines
    gui_send(GUI,"info depth 1 time 0 nodes 0 nps 0 cpuload 0");
    gui_send(GUI,"bestmove %s",move_string);
}

// format_uci_option_line()

static void format_uci_option_line(char * option_line,option_t *opt){
    char option_string[StringSize];
    char type[StringSize];
    int j;
    strcpy(option_line,"");
        // buffer overflow alert
    strcat(option_line,"option name");
    if(opt->mode&PG){
        strcat(option_line," Polyglot");
    }
    sprintf(option_string," %s",opt->name);
    strcat(option_line,option_string);
    normalize_type(type,opt->type);
    sprintf(option_string," type %s",type);
    strcat(option_line,option_string);
    if(!IS_BUTTON(opt->type)){
        sprintf(option_string," default %s",opt->value);
        strcat(option_line,option_string);
    }
    if(IS_SPIN(opt->type)){
        sprintf(option_string," min %s",opt->min);
        strcat(option_line,option_string);
    }
    if(IS_SPIN(opt->type)){
        sprintf(option_string," max %s",opt->max);
        strcat(option_line,option_string);
    }
    for(j=0;j<opt->var_nb;j++){
        sprintf(option_string," var %s",opt->var[j]);
        strcat(option_line,option_string);
    }
}

// send_uci_options()

static void send_uci_options() {

    option_t * opt;
    char option_line[StringSize]="";
    gui_send(GUI,"id name %s", Uci->name);
    gui_send(GUI,"id author %s", Uci->author);
    option_start_iter(Uci->option);
    while((opt=option_next(Uci->option))){
        format_uci_option_line(option_line,opt);
        gui_send(GUI,"%s",option_line);
    }
    option_start_iter(Option);
    while((opt=option_next(Option))){
        if(opt->mode &UCI){
            format_uci_option_line(option_line,opt);
            gui_send(GUI,"%s",option_line);
        }
    }
    gui_send(GUI,"uciok");
}

// parse_setoption()



static void parse_setoption(const char string[]) {
    char *name;
    char *pg_name;
    char *value;
    char * string_copy;
    string_copy=my_strdup(string);
    if(match(string_copy,"setoption name * value *")){
        name=Star[0];
        value=Star[1];
        if(match(name, "Polyglot *")){
            pg_name=Star[0];
            polyglot_set_option(pg_name,value);
        }else{
            engine_send(Engine,"%s",string);
        }
    }else{
        engine_send(Engine,"%s",string);
    }
    free(string_copy);
}


// uci2uci_gui_step()

void uci2uci_gui_step(char string[]) {
    int move;
     if(FALSE){
     }else if(match(string,"uci")){
         send_uci_options();
         return;
     }else if(match(string,"setoption *")){
         parse_setoption(string);
         return;
     }else if(match(string,"position *")){
         parse_position(string);
         Init=FALSE;
     }else if(match(string,"go *")){
         //UT: detect if user limit was set
         //If so, we'll modify the go command
         bool user_limit = FALSE;
         if (!my_string_equal(option_get_string(Option,"NodesLimit"),"<empty>") || !my_string_equal(option_get_string(Option,"DepthLimit"),"<empty>") || !my_string_equal(option_get_string(Option,"Movetime"),"<empty>") ||
         !my_string_equal(option_get_string(Option,"AverageMovetime"),"<empty>")) {
             user_limit = TRUE;
         }

         if(Init){
             board_from_fen(UCIboard,StartFen);
             Init=FALSE;
         }
         SavedMove=MoveNone;
         //UT: if user_limit is active, we also consider "go infinite" with
         //limit and we'll do book lookup.
         if( (!strstr(string,"infinite") || user_limit)
	    && UCIboard->move_nb<option_get_int(Option,"BookDepth")){
             move=book_move(UCIboard,option_get_bool(Option,"BookRandom"));
             if (move != MoveNone && move_is_legal(move,UCIboard)) {
                 if(strstr(string,"ponder")){
                     SavedMove=move;
                     return;
                 }else{
                     send_book_move(move);
                     return;
                 }
             }
         }

         //UT: handle user limit
         if(user_limit){
             const char* nodes_limit = option_get_string(Option,"NodesLimit");
             const char* depth_limit = option_get_string(Option,"DepthLimit");
             const char* movetime = option_get_string(Option,"Movetime");
             const char* avg_movetime = option_get_string(Option,"AverageMovetime");
             float hostperf = atof(option_get_string(Option,"HostPerformanceFactor"));
             if (!my_string_equal(nodes_limit,"<empty>")){
                 sprintf(string, "go nodes %s", nodes_limit);
             }
             if (!my_string_equal(depth_limit,"<empty>")){
                 sprintf(string, "go depth %s", depth_limit);
             }
             if (!my_string_equal(movetime,"<empty>")){
                 int movetime_cal = (int) (atof(movetime)*hostperf);
                 sprintf(string, "go movetime %d", movetime_cal);
             }
             if (!my_string_equal(avg_movetime,"<empty>")){
                 int avg_movetime_win = atoi(option_get_string(Option,"AverageMovetimeWindow"));
                 int avg_movetime_cal = (int) (atof(avg_movetime)*hostperf);
                 int avg_movetime_final = avg_movetime_win*avg_movetime_cal;
                 sprintf(string, "go wtime %d btime %d movestogo %d", avg_movetime_final, avg_movetime_final, avg_movetime_win);
             }
        }

     }else if(match(string,"ponderhit") || match(string,"stop")){
         if(SavedMove!=MoveNone){
         	send_book_move(SavedMove);
         	SavedMove=MoveNone;
         	return;
         }
     }else if(match(string,"quit")){
         my_log("POLYGLOT *** \"quit\" from GUI ***\n");
         quit();
     }

     //UT: added this
     else if(match(string,"test")){
         uci_send_ucinewgame(Uci);
         //This is the position after 23. exd5 Qd6 in the "Immortal Game", Topalov vs Kasparov (1999). The correct move is the rook sacrifice 24. Rxd4.
         parse_position("position startpos moves e2e4 d7d6 d2d4 g8f6 b1c3 g7g6 c1e3 f8g7 d1d2 c7c6 f2f3 b7b5 g1e2 b8d7 e3h6 g7h6 d2h6 c8b7 a2a3 e7e5 e1c1 d8e7 c1b1 a7a6 e2c1 e8c8 c1b3 e5d4 d1d4 c6c5 d4d1 d7b6 g2g3 c8b8 b3a5 b7a8 f1h3 d6d5 h6f4 b8a7 h1e1 d5d4 c3d5 b6d5 e4d5 e7d6");
         Init=FALSE;
         engine_send(Engine, "position startpos moves e2e4 d7d6 d2d4 g8f6 b1c3 g7g6 c1e3 f8g7 d1d2 c7c6 f2f3 b7b5 g1e2 b8d7 e3h6 g7h6 d2h6 c8b7 a2a3 e7e5 e1c1 d8e7 c1b1 a7a6 e2c1 e8c8 c1b3 e5d4 d1d4 c6c5 d4d1 d7b6 g2g3 c8b8 b3a5 b7a8 f1h3 d6d5 h6f4 b8a7 h1e1 d5d4 c3d5 b6d5 e4d5 e7d6");
         printf("test position loaded\n");
         return;
     }


     engine_send(Engine,"%s",string);
}

void uci2uci_engine_step(char string[]) {
    gui_send(GUI,string);
}

// end of uci2uci.cpp
