#ifndef S2SDEFINE
	#define S2SDEFINE

	#include "rose.h"

	#define MODNAME "ASSIST"
	//#define MODNAME "s2s"

	//Debug define. It will print the name of the current function and the line where is the call of this define
	// #define DBG_MAQAO std::cout<<"\033[1;36m function : "<<__FUNCTION__<<" line : "<<__LINE__<< "\033[0m" << std::endl;
	#define DBG_MAQAO s2s_api::printdebug("function : "+std::string(__FUNCTION__)+" line : "+s2s_api::itoa(__LINE__));
	
	// When we search a loop with binary information
	// It's possible that the loop was not exactly a the line indicate
	// So we have to search around a small delta
	// in 90% of the time it works
	#define S2S_DELTA 3
	
	// Try to define a tag to print a error msg indicating a part of the code is not implemented yet
	#define NOT_IMPLEMENTED_YET if(DEBUG){std::cout<<"\033[ This case is not implemented yet \nproblem from "<< __FILE__<< " function : "<< __FUNCTION__<<" l."<<__LINE__<< "\033[0m" << std::endl;}

	// Tag use to easily use Rose stuff
	#define TRANSFORMATION_FILE_INFO Sg_File_Info::generateDefaultFileInfoForTransformationNode()
	
	#define DL1_RATIO_MIN_TO_TILE 2
	#define VEC_RATIO_MIN_TO_BV 35
	#define NB_ITE_MAX_TO_BV 8

	// Global variable used to know what to print
	extern int DEBUG;  // = 0;
	extern int LOG;    // = 1;
	extern int VERBOSE;// = 1;
	extern bool clean_version;// = false;
  extern bool apply_directives; // = false;
	extern std::string currentFile; // = ""
	extern std::string currentFileType; // = ""
	// Used to insert call to the Vprof lib
	extern SgFunctionDefinition* vprofLibCallformain;

	/**
	 * Structure used to specialize a function
	 * It contain an enum to specify which kind of specialization of variable is it,
	 * the name of the variable to specify, and its bound.
	 */
	struct variable_spe {
  	enum spe { INTERVAL, EQUALS , INF, SUP};
  	char* var_name;
  	int inf_bound;
  	int sup_bound;
  	int value;
  	char* label;
  	enum spe specialization;

	  // void pretty_print() {
	  //   std::cout << " --- variable_spe --- \n"
	  //             << "var name : " << var_name 
	  //             << std::endl
	  //             << "SPECILIZATION : " ;
	  //   switch (specialization) {
	  //     case INTERVAL: {
	  //      std::cout << "INTERVAL"<< std::endl;
	  //      std::cout << "values between : {" 
	  //                << inf_bound
	  //                << "," << sup_bound
	  //                << "}" << std::endl;
	  //      break;
	  //    }
	  //     case EQUALS: {
	  //      std::cout << "EQUALS"    << std::endl;
	  //      std::cout << "var = " << value << std::endl;
	  //      break;
	  //    }
	  //     case INF: {
	  //      std::cout << "INF"<< std::endl;
	  //      std::cout << "var < " << sup_bound << std::endl;
	  //      break;
	  //    }
	  //     case SUP: {
	  //      std::cout << "SUP"<< std::endl;
	  //      std::cout << "var > " << inf_bound << std::endl;
	  //      break;
	  //    }
	  //   }
	  //   std::cout << std::endl;
	  // }
	};

	/**
	 * Structure used to define a loop from a configuration file
	 */
	struct configLoop {
	  int id;
	  int line;
	  char* label;
  	int nbtransfo;
	  char** transfo;

	  void prettyprint() {
	  	std::cout << "id : " << id << " | ";
	  	std::cout << "line : " << line << " | ";
	  	std::cout << "label : " << label << " | ";
	  	std::cout << "nbtransfo : " << nbtransfo << " | ";
	  	// std::cout << "transfo : " << transfo << " | ";
	  	std::cout << std::endl;
	  }
	};

	/**
	 * Structure used to define a function from a configuration file
	 */
	struct configFunc {
	  int id;
	  int line;
	  char* label;
  	  int nbtransfo;
	  char** transfo;
	};

	/**
	 * Structure used to use CQA informations
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

	void pretty_print(cqa_struct cs);

	typedef struct vprofLoop {
	  int id;
	  const char* file;
	  const char* path;
	  const char* directive;
	  int nbDir;
	  int lineStart;
	  int lineEnd;
	  // int ite_min;
	  // int ite_max;
	  // int ite_avg;

	  void pretty_print() {
       std::cout << "id " 	  << id 	<< std::endl;;
       std::cout << "file "   << file << std::endl;
       std::cout << "line "   << lineStart << std::endl;
       std::cout << "directive "   << directive << std::endl;
       // std::cout << "ite_min "<< ite_min << std::endl;
       // std::cout << "ite_max "<< ite_max << std::endl;
       // std::cout << "ite_avg "<< ite_avg << std::endl << std::endl;
	  }
	} vprofLoop;

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

	void pretty_print(vprof_struct vs);

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

	void pretty_print(decan_struct ds);

	typedef struct profilerInfo {
	  int id;
	  int lineStart;
	  int lineEnd;
	  char*  file;
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
	 * Struct to use for group information
	 * using all profiler from oneview
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

	/**
	 * Not finished
	 * We will use it to add internal intelligence
	 * To search if specialize a variable(varSym) is useful or not
	 */
	struct var_metric {
	  SgVariableSymbol * varSym;
	  unsigned int numberInIfCond;
	  unsigned int numberInLoopCond;
	  unsigned int numberInArithmStmt;
	  unsigned int numberVariablesImplies;
	  unsigned int nummberMisc;
	  //unsigned int weight;

	  unsigned int get_weight() {
	  	return numberInIfCond+
	  				 numberInLoopCond+
	  				 numberInArithmStmt+
	  				 numberVariablesImplies+
	  				 nummberMisc;
	  }
	  void printweight() {
	  	std::cout << varSym->get_name().getString() << " was in : \n";
	  	std::cout << numberInIfCond << "\tif conditions\n";
	  	std::cout << numberInLoopCond << "\tloop head (init; condition; incr)\n";
	  	std::cout << numberInArithmStmt << "\tarithmetic operations\n";
	  	std::cout << nummberMisc << "\tother statments (array declarations, etc)\n";
	  	std::cout << "and implies " << numberVariablesImplies << " variables\n";
	  	std::cout << "Total : " << get_weight() << std::endl;
	  }
	  var_metric(SgVariableSymbol * vs): varSym(vs),
	  						 numberInIfCond(0),
	  						 numberInLoopCond(0),
	  						 numberInArithmStmt(0),
	  						 numberVariablesImplies(0),
	  						 nummberMisc(0) {}
	};

#endif