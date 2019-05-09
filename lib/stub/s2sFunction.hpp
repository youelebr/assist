#ifndef S2SFUNC
#define S2SFUNC

#include "rose.h"
#include "s2sDefines.hpp"
#include "api.hpp"

#include <sstream>
#include <string>
#include <iostream>
#include <vector>

//int MAQAO_DEBUG_LEVEL=0;

static int identifierFunctions=0;

class ASTFunction;
class Function;
struct variable_spe;

/**
 * ASTFunction class :
 * Represent the top of the tree
 * A tree root is the global scope of a file and all nodes are functions definitions
 */
class ASTFunction {
  protected:
    // size_t id;        // to match with maqao id function
    // std::string name; // Name of the function
    SgGlobal* root;   // Represent a node above the subtree of functions (it should be a SgNode*)
    std::vector<Function*> internalFunctionsList; // Vector which contain list of all functions of the file

  public:
    //Constructor
    ASTFunction(SgSourceFile*);
    ASTFunction(SgBasicBlock*);
    ASTFunction(SgGlobal* root);
    //Destructor
    ~ASTFunction();

    //Accessors
    std::vector<Function*> get_internalFunctionsList();
    void set_internalFunctionsList(std::vector<Function*> );
    
    //Miscellaneous
    /**
     * Print functions which are marks as not found 
     * Essentially use to compare which functions match with binary loops
     */
    void printASTLines();
    /**
     * Print functions which marks as not found 
     * Essentially use to compare which functions match with binary functions
     */
    void print_function_not_found();
    /**
     * Print information about all functions of the subtree,
     * only print information about all functions in the file.
     */
    void print();
    /**
     * Print all functions names of the file where they're comme from.
     */
    void print_names();
    /**
     * Search into the global tree a function near of the start/end lines
     *
     * @param start - Line where the function start
     * @param end - Line where the function end
     * @return - Pointer on a Function which correspond to the lines
     */
    Function* get_function_from_line(int start,int end=-1);
    /**
     * Search into the global tree a function with the name name
     *
     * @param name - The name of the function
     * @return - Pointer on a Function which correspond to the name
     */
    Function* get_func_from_name(std::string name);
    /**
     * Search into the global tree which function contains a maqao directive and apply it
     */
    void apply_directive();
    /**
     * Return true if the gloabl tree is empty
     * @return - Return true if the gloabl scope doesn't have any function definition
     */
    bool empty();
};

/** 
 * Function class:
 * represent a function definition with its body and original body (before any transformation), 
   its pointer on the Rose representation of the Function definition, the type of the function 
   and its id.
 *
 */
class Function {
  public:
    enum functionType_ { SUBROUTINE, FUNCTION, UNKNOWN }; // Enum to represent the function type
  protected:
    size_t id;                       // Used to match with maqao id function, by default functions' id are set from 1 to n to the order of reading the source code  
    SgFunctionDefinition * function; // Rose function
    SgBasicBlock * body_without_trans;
    functionType_ typeOfFunction;    // The function type
    bool is_matching;                // Only for internal used
  public:
    //CONSTRUCTOR
    Function(SgFunctionDefinition* f);
    ~Function();
    //ACCESSORS
    SgFunctionDefinition* get_function();
    SgFunctionDefinition* get_function() const;
    void set_function(SgFunctionDefinition* f);

    size_t get_id();
    size_t get_id() const;
    void   set_id(size_t ID);

	std::string get_name() const;
    std::string get_file_name() const;

    functionType_ get_functionType();
    functionType_ get_functionType() const;
    void set_functionType(Function::functionType_);
    void set_functionType(std::string functionKeyword);

    int  get_line_start();
    int  get_line_end();

    /**
     * Return true or false if the function was already meets the binary function conrresponding or not.
     * @return Return true if the function was already meets the binary function conrresponding
     */
    bool is_matching_with_bin_loop();
    void set_matching_with_bin_loop(bool);
    
    //Prints
    /**
     * Print the type of the function (Fortran do; C/C++ "for" ; while ; etc)
     */
    std::string print_functionType();
    void print_statement_class();
    /**
     * Print the function begining and ending lines.
     */
    void printLines();
    /**
     * Print information about the function
     */
    void print();
    /**
     * Print functions which marks as not found 
     * Essentially use to compare which functions match with binary functions
     */
    void print_function_not_found();

    //miscellaneous
    /**
     * Attach a directive to the function
     *
     * @param s - string containing the directive line without the "!DIR$"
     */
    void add_directive(std::string s);
    /**
     * Look for all maqao directive,
     * then apply transformations requested.
     */
    bool apply_directive();
    bool apply_directive(std::string dirctive);
    // void apply_directive2();

    /**
     * Sepcialize a function for a special value,
     * this value can be specified explicitly than equals to a fix value.
     * Or it's possible to indicate what it's its limits,
     * for example, it's possible to indicate than a variable is < to 3 or is between {2,10}
     *
     * This function will analyze vectors of variable_spe and will create special function with there indications
     */
    void specialize(std::vector<variable_spe*> var, bool create_call); // /!\ Deprecated
    void specialize_v2(std::vector<variable_spe*> var, bool create_call);

    /**
     * Add the call to the function of our library to modify prefect guided by a configuration define in the string s.
     * String s : The configuration name for prefetch.
     */
    bool modify_prefetch(std::string s);
};

/**
 * Build a conditionnal of a if stmt
 */
SgExprStatement* build_if_cond(variable_spe* var, SgSymbol* v_symbol, SgScopeStatement* function);
/**
 * Continue to build the conditionnal of the if stmt
 */
SgExprStatement* build_if_cond_from_ifcond(variable_spe* var, SgSymbol* v_symbol,  SgScopeStatement* function, SgExprStatement* ifcond);
/**
 * Return a pointer on function corresponding to the function at line lstart
 */
Function* get_function_from_line(ASTFunction* ast, int lstart, int lend = -1, int delta=0);

#endif