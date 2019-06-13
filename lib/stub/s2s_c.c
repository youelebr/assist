#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <lualib.h>

#define s2s "s2s"
#define DEBUG 0
#define DBG_MAQAO printf("function : %s line : %d\n",__FUNCTION__,__LINE__);

typedef struct configLoop {
  int id;
  int line;
  char** label;
  int nbtransfo;
  char** transfo;
} configLoop;

typedef struct configFunc {
  int id;
  int line;
  char** label;
  int nbtransfo;
  char** transfo;
} configFunc;

typedef struct vprof_struct {
  int id;
  char* file;
  char* directive;
  int lineStart;
  int lineEnd;
  // int ite_min;
  // int ite_max;
  // int ite_avg;
} vprof_struct;
/**
 * Represent what to do for one loop
 * using CQA metrics
 */
typedef struct cqa_struct {
  int id;
  char* file;
  char* func_name;
  int lineStart;
  int lineEnd;
  float vect_ratio;
  char* transfo;
} cqa_struct;
/**
 * Represent what to do for one loop
 * using DECAN metrics
 */
typedef struct decan_struct {
  int id;
  char* file;
  int lineStart;
  int lineEnd;
  float dl1_ratio;
  char* transfo;
} decan_struct;

/**
 * Structure used to specialize a function
 * It contain an enum to specify which kind of specialization of variable is it,
 * the name of the variable to specify, and its bound.
 */
typedef struct variable_spe {
  enum spe { INTERVAL, EQUALS , INF, SUP};
  char* var_name;
  int inf_bound;
  int sup_bound;
  int value;
  char* label;
  enum spe specialization;
} variable_spe;

/**
 * Struct to use for group information
 * using all profiler
 */
typedef struct profilerInfo {
  int id;
  int lineStart;
  int lineEnd;
  char* file;
  char** label;
  int nbTransCQA;
  cqa_struct* cqa;
  int nbTransVPROF;
  vprof_struct* vprof;
  int nbTransDECAN;
  decan_struct* decan;
  int nb_variable_spe;
  variable_spe* var_spe;
} profilerInfo;


/**
 * New struct to use for group information
 * using all profiler from oneview
 * It will be easier to use if all information was group by loop
 */
typedef struct profilerLoopInfo {
  int id;
  int lineStart;
  int lineEnd;
  char* file;
  char** label;
  // CQA
  float vec_ratio;
  // DECAN
  float dl1_ratio_min;
  float dl1_ratio_max;
  float dl1_ratio_mean;
  // VPROF
  int nb_ite_min;
  int nb_ite_max;
  int nb_ite_avg;
  // SPECIALIZATION
  int nb_variable_spe;
  variable_spe* var_spe;
} profilerLoopInfo;

void print(struct vprof_struct l) {
  int i=0;
  printf("id =%d\nfile =%s\nline =%zu\n",
          l.id,l.file,l.lineStart);
    printf("Directive = %s \n", l.directive);
}

extern int intermediaire_cpp(char* input, char* output , int numberOfLoops, int loopsLines[], int noo, char** options);
extern int main_config_file (char* input, char* output , int numberOfLoops, configLoop loops[], int numberOffuncs, configFunc funcs[], int numberOfOptions, char** options);
extern int main_profilers   (char* input, char* outputprefix, profilerInfo * pi, int numberOfOptions, char** options);
extern int main_oneview     (char* input, char* outputprefix, int nb_pi, profilerInfo* pi, int numberOfOptions, char** options);

static int l_s2s_start ( lua_State * L ) {
  //printf("%s\n",__FUNCTION__);
  int i=0;
  int* loopsLines = NULL;
  char** options = NULL;

  //Store the input file
  luaL_checktype(L, 1,LUA_TSTRING);
  char* input = strdup(lua_tostring (L, 1));

  //store the output name if it exist
  luaL_checktype(L, 2,LUA_TSTRING);
  char* output = strdup(lua_tostring (L, 2));

  //check the number of loop to analyze for this file
  luaL_checktype(L, 3, LUA_TNUMBER);
  int numberOfLoops = lua_tointeger (L, 3);

  loopsLines = malloc(sizeof(int) * numberOfLoops);
  if( lua_istable( L, 4 ) && lua_objlen(L,4) != 0 ) {
    loopsLines = malloc(sizeof(int) * lua_objlen(L, 4));
    lua_pushnil(L);                       // put the first index (0) on the stack for lua_next
    while( lua_next( L, 4 ) ) {            // pop the index, read table[index+1], push index (-2) and value (-1)
      loopsLines[i++] = lua_tointeger (L,-1); // store the value
      //printf("loop line : %d\n",loopsLines[i-1] );
      lua_pop( L, 1 );                    // remove the value from the stack (keep the next index)
    }
  }

  //Options
  i=0;
  if( lua_istable( L, 5 ) && lua_objlen(L,5) != 0 ) {
    options = malloc(sizeof(char**) * lua_objlen(L, 5));
    lua_pushnil(L);                       // put the first index (0) on the stack for lua_next
    while( lua_next( L, 5 ) ) {            // pop the index, read table[index+1], push index (-2) and value (-1)
      options[i++] = lua_tostring (L,-1); // store the value
      //printf("Options[%d]:%s\n",i-1,options[i-1] );
      lua_pop( L, 1 );                    // remove the value from the stack (keep the next index)
    }
  }
  // for(i=0; i < lua_objlen(L, 5); ++i ) {
  //   printf("Options[%d]= %s\n",i,options[i] );

  // }
  intermediaire_cpp(input, output, numberOfLoops, loopsLines,lua_objlen(L, 5), options);
  return 0;
}

static int l_s2s_start_config_file ( lua_State * L ) {
  //printf("%s\n",__FUNCTION__);
  int i=0, j=0, k = 0;
  int numberOfLoops;  //arg 3
  int numberOfFuncs;  //arg 5
  const char* input;  //arg 1
  const char* output; //arg 2
  configLoop* loops;  //arg 4 
  configFunc* funcs;  //arg 6
  char** options = NULL;//arg 7
  int nbTransfoL=0, nbTransfoF=0;

  //Store the input file
  luaL_checktype(L, 1,LUA_TSTRING);
  input = strdup(lua_tostring (L, 1));

  //store the output prefix name if it exist
  luaL_checktype(L, 2,LUA_TSTRING);
  output = strdup(lua_tostring (L, 2));

  //check the number of loop to analyze for this file
  luaL_checktype(L, 3, LUA_TNUMBER);
  numberOfLoops = lua_tointeger (L, 3);

  /***************
   **   Loops   **
   ***************/
  loops = malloc(sizeof(configLoop) * numberOfLoops);
  luaL_checktype(L, 4, LUA_TTABLE);
  lua_pushnil(L);

  while (lua_next(L, 4) != 0)
  {
    luaL_checktype (L, 8, LUA_TNUMBER);
    int loopidx = lua_tointeger(L, 8);

    luaL_checktype (L, 9, LUA_TTABLE);
    nbTransfoL = luaL_getn (L, 9);
    loops[loopidx].nbtransfo = nbTransfoL - 1;
    lua_pushnil (L);

    //First element is the line of the loop to transform
    //Then the label (one or the other is == to nil)
    if (lua_next (L, 9) != 0) {
      // get index of the array
      luaL_checktype (L, 10, LUA_TNUMBER);
      int id = lua_tointeger (L, 10);

      if (lua_type(L, 11) == LUA_TSTRING) { 
        loops[loopidx].label = lua_tostring (L, 11);
        loops[loopidx].line = -1;
      } else {
        // get the value
        luaL_checktype (L, 11, LUA_TNUMBER);

        loops[loopidx].line = atoi(lua_tostring (L, 11));
        loops[loopidx].label = "";
      }

      // printf(" -- loops[%d].line = %d \n", loopidx, loops[loopidx].line);
      // printf(" -- loops[%d].label = %s \n", loopidx, loops[loopidx].label);
      lua_pop (L, 1);

      loops[loopidx].transfo = malloc(sizeof(char**) * nbTransfoL);
      while (lua_next (L, 9) != 0)
      {
        luaL_checktype (L, 10, LUA_TNUMBER);
        int transIdx = lua_tointeger (L, 10) - 2 ;

        luaL_checktype (L, 11, LUA_TSTRING);
        loops[loopidx].transfo[transIdx] = lua_tostring (L, 11);
        lua_pop (L, 1);
      }
    }
    lua_pop (L, 1);
  }

  luaL_checktype(L, 5, LUA_TNUMBER);
  numberOfFuncs = lua_tointeger (L, 5);
  /***************
   **   Funcs   **
   ***************/
  funcs = malloc(sizeof(configFunc) * numberOfFuncs);
  luaL_checktype(L, 6, LUA_TTABLE);
  lua_pushnil(L);

  while (lua_next(L, 6) != 0)
  {
    luaL_checktype (L, 8, LUA_TNUMBER);
    int funcidx = lua_tointeger(L, 8);

    luaL_checktype (L, 9, LUA_TTABLE);
    nbTransfoF = luaL_getn (L, 9);
    funcs[funcidx].nbtransfo = nbTransfoF - 1;
    lua_pushnil (L);

    //First element is the line of the loop to transform
    //Then the label (one or the other is == to nil)
    if (lua_next (L, 9) != 0) {
      // get index of the array
      luaL_checktype (L, 10, LUA_TNUMBER);
      int id = lua_tointeger (L, 10);

      if (lua_type(L, 11) == LUA_TSTRING) { 
        funcs[funcidx].label = lua_tostring (L, 11);
        funcs[funcidx].line = -1;
      } else {
        // get the value
        luaL_checktype (L, 11, LUA_TNUMBER);

        funcs[funcidx].line = atoi(lua_tostring (L, 11));
        funcs[funcidx].label = "";
      }

      // printf(" -- funcs[%d].line = %d \n", funcidx, funcs[funcidx].line);
      // printf(" -- funcs[%d].label = %s \n", funcidx, funcs[funcidx].label);
      lua_pop (L, 1);

      funcs[funcidx].transfo = malloc(sizeof(char**) * nbTransfoF);
      while (lua_next (L, 9) != 0)
      {
        luaL_checktype (L, 10, LUA_TNUMBER);
        int transIdx = lua_tointeger (L, 10) - 2 ;

        luaL_checktype (L, 11, LUA_TSTRING);
        funcs[funcidx].transfo[transIdx] = lua_tostring (L, 11);
        // printf(" -- funcs[%d].transfo = %s\n",funcidx, funcs[funcidx].transfo[transIdx] );
        lua_pop (L, 1);
      }
    }
    lua_pop (L, 1);
  }

  //Options
  i=0;
  if( lua_istable( L, 7 ) && lua_objlen(L,7) != 0 ) {
    options = malloc(sizeof(char**) * lua_objlen(L, 7));
    lua_pushnil(L);                       // put the first index (0) on the stack for lua_next
    while( lua_next( L, 7 ) ) {            // pop the index, read table[index+1], push index (-2) and value (-1)
      options[i++] = lua_tostring (L,-1); // store the value
      printf("Options[%d]:%s\n",i-1,options[i-1] );
      lua_pop( L, 1 );                    // remove the value from the stack (keep the next index)
    }
  }

  main_config_file(input, output, numberOfLoops, loops, numberOfFuncs, funcs, lua_objlen(L,7) , options);
  return 0;
}

static int l_s2s_start_oneview ( lua_State * L ) {
  if(DEBUG) printf("%s\n",__FUNCTION__);
  int i=0, nb_args=7, nbOptions=0;
  int itpi = 0;
  int* loopsLines = NULL;
  char** options = NULL;

  //Store the input file
  luaL_checktype(L, 1,LUA_TSTRING);
  char* input = strdup(lua_tostring (L, 1));
  if (DEBUG) printf("[C] input %s\n", input);
  //store the output name if it exist
  luaL_checktype(L, 2,LUA_TSTRING);
  char* output = strdup(lua_tostring (L, 2));
  if (DEBUG) printf("[C] output %s\n", output);

  //check the number of loop to analyze for this file
  luaL_checktype(L, 3, LUA_TNUMBER);
  int numberOfInfo = lua_tointeger (L, 3);
  if (DEBUG) printf("[C] numberOfLoops %d\n", numberOfInfo);

  // Not ready yet
  luaL_checktype(L, 5, LUA_TNUMBER);
  int numberOfVarSpe = lua_tointeger (L, 5);
  if (DEBUG) printf("[C] numberOfVarSpe %d\n", numberOfVarSpe);
  
  /********************
   **   management   **
   ** of the struct  **
   ********************/
  /* 
    typedef struct profilerLoopInfo {
      int id;
      int lineStart;
      int lineEnd;
      char* file;
      char** label;
      // CQA
      float vec_ratio;
      // DECAN
      float dl1_ratio_min;
      float dl1_ratio_max;
      float dl1_ratio_mean;
      // VPROF
      int nb_ite_min;
      int nb_ite_max;
      int nb_ite_avg;
      // SPECIALIZATION
      int nb_variable_spe;
      variable_spe* var_spe;
    } profilerLoopInfo;
  */
  profilerLoopInfo * pi;
  if (numberOfInfo > 1)
    pi = malloc(sizeof(profilerLoopInfo) * numberOfInfo);
  else 
    pi = malloc(sizeof(profilerLoopInfo));

  if (numberOfInfo > 0) {
    if (DEBUG) printf("FILLING THE STRUCTURE\n");
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_pushnil(L);
    //Browse the struct
    while (lua_next(L, 4) != 0) {
      luaL_checktype(L, nb_args+2, LUA_TTABLE);
      lua_pushnil(L);
      // initialize struct
      pi[itpi].id = -1;
      pi[itpi].lineStart = -1;
      pi[itpi].lineEnd = -1;
      pi[itpi].file = "";
      pi[itpi].label = "";
      // CQA
      pi[itpi].vec_ratio = -1;
      // DECAN
      pi[itpi].dl1_ratio_min = -1.0;
      pi[itpi].dl1_ratio_max = -1.0;
      pi[itpi].dl1_ratio_mean = -1.0;
      // VPROF
      pi[itpi].nb_ite_min = -1;
      pi[itpi].nb_ite_max = -1;
      pi[itpi].nb_ite_avg = -1;
      // SPECIALIZATION
      pi[itpi].nb_variable_spe = 0;
      pi[itpi].var_spe = NULL;

      //Browse the sub-struct
      while (lua_next(L, nb_args+2) != 0) {
        luaL_checktype (L, nb_args+3, LUA_TSTRING);
        const char* key = lua_tostring (L, nb_args+3);
        if(DEBUG) printf("key = %s\n ", key);
        if (strcmp (key, "id") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].id = lua_tointeger(L, nb_args+4);
          if(DEBUG) printf("[C] id : %d\n",pi[itpi].id);
        } else if (strcmp (key, "file")      == 0) {
          luaL_checktype (L, nb_args+4, LUA_TSTRING);
          pi[itpi].file = lua_tostring(L, nb_args+4);
          if(DEBUG) printf("[C] file : %s\n",pi[itpi].file);
        } else if (strcmp (key, "lineStart") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].lineStart = lua_tointeger(L, nb_args+4);
          if(DEBUG) printf("[C] lineStart : %d\n",pi[itpi].lineStart);
        } else if (strcmp (key, "lineStop")  == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].lineEnd = lua_tointeger(L, nb_args+4);
          if(DEBUG) printf("[C] lineEnd : %d\n",pi[itpi].lineEnd);
        } else if (strcmp (key, "label")     == 0) {
          luaL_checktype (L, nb_args+4, LUA_TSTRING);
          pi[itpi].label = lua_tostring(L, nb_args+4);
          if(DEBUG) printf("[C] label : %s\n",pi[itpi].label);
        } else if (strcmp (key, "vec_ratio") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].vec_ratio = lua_tonumber(L, nb_args+4);
          if(DEBUG) printf("[C] vect_ratio : %f\n",pi[itpi].vec_ratio);
        } else if (strcmp (key, "dl1_ratio_min") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].dl1_ratio_min = lua_tonumber(L, nb_args+4);
          if(DEBUG) printf("[C] dl1_ratio_min : %f\n",pi[itpi].dl1_ratio_min);
        } else if (strcmp (key, "dl1_ratio_max") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].dl1_ratio_max = lua_tonumber(L, nb_args+4);
          if(DEBUG) printf("[C] dl1_ratio_max : %f\n",pi[itpi].dl1_ratio_max);
        } else if (strcmp (key, "dl1_ratio_mean") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].dl1_ratio_mean = lua_tonumber(L, nb_args+4);
          if(DEBUG) printf("[C] dl1_ratio_mean : %f\n",pi[itpi].dl1_ratio_mean);
        } else if (strcmp (key, "nb_ite_min") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].nb_ite_min = lua_tonumber(L, nb_args+4);
          if(DEBUG) printf("[C] nb_ite_min : %f\n",pi[itpi].nb_ite_min);
        } else if (strcmp (key, "nb_ite_max") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].nb_ite_max = lua_tonumber(L, nb_args+4);
          if(DEBUG) printf("[C] nb_ite_max : %f\n",pi[itpi].nb_ite_max);
        } else if (strcmp (key, "nb_ite_avg") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[itpi].nb_ite_avg = lua_tonumber(L, nb_args+4);
          if(DEBUG) printf("[C] nb_ite_avg : %f\n",pi[itpi].nb_ite_avg);
        }
        lua_pop (L, 1);
      } // end while
      itpi++;
      lua_pop (L, 1);
    } //end while 
  } // end if 

  /****************************
   **       management       **
   ** of variable_spe struct **
   ****************************/
  /**
   * typedef struct variable_spe {
   *  enum spe { INTERVAL, EQUALS , INF, SUP};
   *  char* var_name;
   *  int inf_bound;
   *  int sup_bound;
   *  int value;
   *  char* label;
   *  enum spe specialization;
   * } variable_spe;
  **/
  // Put spe info only in the first element of the array
  pi[0].var_spe = malloc(sizeof(variable_spe) * numberOfVarSpe);
  pi[0].nb_variable_spe = numberOfVarSpe;

  if (numberOfVarSpe > 0) {
    if (DEBUG) printf("VAR SPE INFO FILLING\n");
    if (DEBUG) printf("Number of var to spe is : %d\n", numberOfVarSpe);
    int itevarspe = 0;
    luaL_checktype(L, 6, LUA_TTABLE);
    lua_pushnil(L);
    //Browse the struct
    while (lua_next(L, 6) != 0) {
      luaL_checktype(L, nb_args+2, LUA_TTABLE);
      lua_pushnil(L);
      //Browse the sub-struct
      while (lua_next(L, nb_args+2) != 0) {
        luaL_checktype (L, nb_args+3, LUA_TSTRING);
        const char* key = lua_tostring (L, nb_args+3);
        if(DEBUG) printf("key %s\n", key);

        if (strcmp (key, "file") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TSTRING);
          pi[0].file = lua_tostring(L, nb_args+4);
          if(DEBUG) printf("[C] file : %s\n",pi[0].file);
        } else if (strcmp (key, "kind") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TSTRING);
          //   pi[0].var_spe[itevarspe].kind = lua_tointeger(L, nb_args+4);
          // if(DEBUG) printf("[C] (NOT ADDED) kind : %d\n",lua_tostring(L, nb_args+4));
        } else if (strcmp (key, "label") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TSTRING);
          pi[0].var_spe[itevarspe].label = lua_tostring(L, nb_args+4);
          if(DEBUG) printf("[C] label : %s\n",pi[0].var_spe[itevarspe].label);
        } else if (strcmp (key, "var_name") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TSTRING);
          pi[0].var_spe[itevarspe].var_name = lua_tostring(L, nb_args+4);
          if(DEBUG) printf("[C] var_name : %s\n",pi[0].var_spe[itevarspe].var_name);
        } else if (strcmp (key, "value") == 0) {
          luaL_checktype (L, nb_args+4, LUA_TNUMBER);
          pi[0].var_spe[itevarspe].value = lua_tointeger(L, nb_args+4);
          if(DEBUG) printf("[C] val : %d\n",pi[0].var_spe[itevarspe].value);
        }
        lua_pop (L, 1);
      } // end while
      pi[0].var_spe[itevarspe].specialization = EQUALS;      
      itevarspe++;
      lua_pop (L, 1);
    } // end while
  } // end if 
  else {
    // printf("[Warning] numberOfVarSpe <= 0 ! \n");
  }

  //Options
  nbOptions=0;
  if( lua_istable( L, 7 ) && lua_objlen(L,7) != 0 ) {
    options = malloc(sizeof(char**) * lua_objlen(L, 7));
    lua_pushnil(L);                       // put the first index (0) on the stack for lua_next
    while( lua_next( L, 7 ) ) {            // pop the index, read table[index+1], push index (-2) and value (-1)
      options[nbOptions++] = lua_tostring (L,-1); // store the value
      if(DEBUG) printf("Options[%d]:%s\n",nbOptions-1,options[nbOptions-1] );
      lua_pop( L, 1 );                    // remove the value from the stack (keep the next index)
    }
  }
  if (DEBUG) {
    printf("[DEBUG INFO] START\n");
    printf("Profiler loop infos : \n");
    printf("===  :\n");
    for(i=0; i < numberOfInfo; ++i ) {
      printf("=======================\n");
      printf("[C] id : %d\n",pi[i].id);
      printf("[C] file : %s\n",pi[i].file);
      // printf("[C] path : %s\n",pi[i].path);
      printf("[C] lineStart : %d\n",pi[i].lineStart);
      printf("[C] lineEnd : %d\n",pi[i].lineEnd);
      printf("[C] nb_ite_min : %d\n",pi[i].nb_ite_min);
      printf("[C] nb_ite_max : %d\n",pi[i].nb_ite_max);
      printf("[C] nb_ite_avg : %d\n",pi[i].nb_ite_avg);
      printf("[C] dl1_ratio_min : %f\n",pi[i].dl1_ratio_min);
      printf("[C] dl1_ratio_max : %f\n",pi[i].dl1_ratio_max);
      printf("[C] dl1_ratio_mean : %f\n",pi[i].dl1_ratio_mean);
      printf("[C] vec_ratio : %f\n",pi[i].vec_ratio);
      printf("=======================\n");
    }

    printf("\n=== OPTIONS : \n ");
    for(i=0; i < nbOptions; ++i ) {
      printf("[C] Options[%d]= %s\n",i,options[i] );
    }
    printf("[DEBUG INFO] END\n");
    if (DEBUG) printf("===================== fin s2s_c =====================\n");
  }

  main_oneview(input, output, numberOfInfo, pi, nbOptions, options);
  return 0;
}

//-----------------------------------------------------------------------------
//                              LUA functions
//-----------------------------------------------------------------------------

static const luaL_reg s2s_methods[] = {
   {"start",            l_s2s_start},
   {"start_config_file",l_s2s_start_config_file},
   {"start_oneview",l_s2s_start_oneview},
   {NULL, NULL}
};

//-----------------------------------------------------------------------------
//                              Meta functions
//-----------------------------------------------------------------------------
static int l_s2s_gc (lua_State *L) {
   (void)L;
   return 0;
}

static int l_s2s_tostring (lua_State *L) {
   lua_pushfstring (L, "s2s Library Object");
   return 1;
}

static const luaL_reg s2s_meta[] = {
   { "__gc", l_s2s_gc},
   { "__tostring", l_s2s_tostring},
   {NULL, NULL}
};

typedef struct
{
  const luaL_reg *methods;
  const luaL_reg *meta;
  const char *id;
} bib_t;

static const bib_t bibs[] = {
  {s2s_methods, s2s_meta, s2s},
  {NULL, NULL, NULL}
};

int luaopen_s2s_c(lua_State *L) {
  const bib_t *b = NULL;

  //DBGMSG0("Registering s2s module\n");
  for (b = bibs; b->id != NULL; b++)
  {
    luaL_register (L, b->id, b->methods);
    luaL_newmetatable (L, b->id);
    luaL_register (L, 0, b->meta);
    lua_pushliteral (L, "__index");
    lua_pushvalue (L, -3);
    lua_rawset (L, -3);
    lua_pushliteral (L, "__metatable");
    lua_pushvalue (L, -3);
    lua_rawset (L, -3);
    lua_pop (L, 1);
  }

  return 1;
}
