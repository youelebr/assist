#ifndef S2SAPI
#define S2SAPI

#include "rose.h"
#include "Outliner.hh"
#include "s2sDefines.hpp"
#include "s2sLoop.hpp"
#include "s2sFunction.hpp"

#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

namespace s2s_api {
/**
 * Load the project from a string which represent compiler options and files you have to compile.
 * It will add all right options for Rose and use the Rose function "frontend(char ** argv, int argc)"
 *
 * @param argv - Vector of string which represent le list of compilation options
 * @return SgProject* - Return a pointer on the Rose's class SgProject
 */
SgProject* load_project(std::vector<std::string> argv);

/**
 * Get a pointer on source file (Rose representation) from any node
 *
 * @param astNode - Node from which we start to go up in the tree.
 * @return srcFile - Return a file where the node come from.
 */
SgSourceFile * getEnclosingsourceFile_RosePtr( const SgNode* astNode );

bool isFortranFile(std::string f);
bool isCCXXFile(std::string f);
bool isCCXXHeader(std::string f);

void printAllStmt(SgGlobal*     root);
void printAllStmt(SgBasicBlock* body);
void printAllStmt(SgDeclarationStatement *decl);
void printdebug  (std::string   s);
void printerror  (std::string   s);

/**
 * My own version of itoa
 */
std::string itoa(int i);
/**
 * Split a string by given delimiter "c"
 */
std::vector<std::string> split(const std::string& s, const std::string& delimiters);

/**
 * Browse the global scope searching function declaration
 * When functions are founded, we search directives to apply on it. 
 * Then we look inside functions bodies to do the same thing on each statement.
 */
void browseFile_and_apply_directives(SgGlobal* globalScope);

/**
 * Browse each statement of a basic block looking for directives to apply.
 * This function is called recursively each time we met a new basic block.
 */
void browseFile_and_apply_directives(SgBasicBlock *body);
bool if_in_specialized_block (std::string varname,std::string compareType);

/**
 * Get a pointer on source file (Rose representation) from any node
 *
 * @param astNode - Node from which we start to go up in the tree.
 * @return FuncDef - Return the function definition (pointer on the Rose representation) where the node come from.
 */
SgFunctionDefinition * get_function_RosePtr(SgNode* astNode);

/**
 * Return a vector of std::string which contains all comments and directives attached to a statement
 *
 * @param astNode - Node of the AST where comments and directives are attached.
 * @return vector<string> - Vector which contains all comments and directives in string format
 */
std::vector<std::string> get_comment_and_directives(SgStatement * astNode);

/**
 * Return a vector of std::string which contains only directives which are attached to a statement
 *
 * @param astNode - Node of the AST where comments and directives are attached.
 * @return vector<string> - Vector which contains directives in string format
 */
std::vector<std::string> get_directives(SgStatement * astNode);

/**
 * Remove all MAQAO directive for a statement
 *
 * @param astNode - Node of the AST where to find comments and directives.
 * @param dirToRemove - substring if you want to remove a specific MAQAO directive on a statement.
 * @return int  - Return the number of MAQAO directives which have been removed; 0 else.
 */
int remove_MAQAO_directives(SgStatement * astNode, std::string dirToRemove="");
std::vector<std::string> get_MAQAO_directives(SgStatement * astNode);

/**
 * Add a directive to a Statement and automatically add the "!DIR$" the begining of the line.
 * The directive will look like : !DIR$ <directive>
 *
 * @param stmt - Statement (pointer on the Rose representation) to apply the directive.
 * @param std::string directive - a string which represent the body of the directive, it automatically add the "!DIR$" 
 * @param bool nodoublon - a boolean to known if the comment to add will be added if already exist 
 */
void add_directive(SgStatement* stmt, std::string directive, bool nodoublon=true);

/**
 * Copy all directives from a Statement to another.
 *
 * @param from - Statement (pointer on the Rose representation) where directives are taken.
 * @param to   - Statement (pointer on the Rose representation) where directives are copied. 
 * @return bool - Return true if everything was ok. (Currently always return true)
 */
bool copy_directives(SgStatement * from, SgStatement * to);
/**
 * Check if the Header <header> already existe in the same file as stmt
 */
bool isHeaderAlreadyExist (SgStatement* stmt, std::string header);

/**
 * Search the first occurence of a number in a string
 * Used for cutting a directive and extract some information
 *
 * @param std::string str - a string where to find the position 
 * @return int - the localisation of the first occurence of a number; -1 if no number in the string
 */
int find_first_number(std::string str);

/**
 * Generate a PDF with all information about nodes and a DOT graph of a Project
 * Use : dot -Tps <file>.dot -o <file>.pdf
 * To transform the dot file to a picture with the AST representation.
 *
 * @param project - Pointer on the Rose representation of the global project.
 */
void generatePDF(SgProject* project);

/** 
 * Print informations about loops in source code and not via the AST,
 * currently, print the start and ending lines.
 * Avoid the creation of Loop AST.
 *
 * @param project - Pointer on Rose representation of a project to print.
 */
void printAllSrcLoops(SgProject* project);
void printAllSrcLoops(SgBasicBlock *body);

/** 
 * Build a binary operation from a variable and an expression with an operation code. 
 * For example, it can build i+1, i*j, i >= y, etc
 *
 * @param var - The reference to the variable which will be at the left of the binary operation
 * @param incr - The expression which will be at the right of the binary operation
 * @param op - The operation code to determine what operation to build (please use Rose representation as V_SgAddOp, to build an addition)
 * @param paren - boolean to determine if parenthesis are needed, by default set to false
 */
SgBinaryOp * myBuildBinOp(SgExpression * var, SgExpression * incr, int op,  bool paren=false);

/**
 * Replace a variable reference by an new expression throught all statement in a BasicBlock.
 *
 * @param body - Basic block (pointer on the Rose Basic block representation) where to search
 * @param var - Variable symbol (pointer on the Rose variable symbol representation) to search
 * @param newExpr - New expression (pointer on the Rose Expression representation) to replace the var
 * @return bool - return true if the replacement was correctly applied
 */
bool replace_expr (SgBasicBlock* body, SgVariableSymbol * var, SgExpression * newExpr);

/**
 * Replace a variable reference by an new expression throught an expression .
 *
 * @param expstmt - Expression (pointer on the Rose Expression representation) where to search
 * @param var - Variable symbol (pointer on the Rose variable symbol representation) to search
 * @param newExpr - New expression (pointer on the Rose Expression representation) to replace the var
 * @return bool - return true if the replacement was correctly applied
 */
bool replace_expr (SgExpression* expstmt, SgVariableSymbol * var, SgExpression * newExpr);

/**
 * Replace a variable reference by an new expression throught an expression .
 *
 * @param bo - Expression (pointer on the Rose Binary operation representation) where to search
 * @param var - Variable symbol (pointer on the Rose variable symbol representation) to search
 * @param newExpr - New expression (pointer on the Rose Expression representation) to replace the var
 * @return bool - return true if the replacement was correctly applied
 */
bool replace_expr (SgBinaryOp* bo, SgVariableSymbol * var, SgExpression * newExpr);

/**
 * Propagate an Integer throught an Expression.
 * 
 * @param expstmt - Expression (pointer on the Rose Expression representation) where to search
 * @param var - Variable symbol (pointer on the Rose variable symbol representation) to search
 * @param constToPropagate - Integer to replace the variable
 * @return bool - return true if the replacement was correctly applied
 */
bool constantPropagation (SgExpression* expstmt, SgVariableSymbol * var, int constToPropagate);

/**
 * Propagate an Integer throught a Basic Block.
 * 
 * @param body - Basic block (pointer on the Rose Basic block representation) where to search
 * @param var - Variable symbol (pointer on the Rose variable symbol representation) to search
 * @param constToPropagate - Integer to replace the variable
 * @return bool - return true if the replacement was correctly applied
 */
bool constantPropagation (SgBasicBlock* body, SgVariableSymbol * var, int constToPropagate);

/**
 * Propagate an Integer throught a Basic Block.
 * 
 * @param stmt - Statement (pointer on the Rose Statement representation) where to search
 * @param var - Variable symbol (pointer on the Rose variable symbol representation) to search
 * @param constToPropagate - Integer to replace the variable
 * @return bool - return true if the replacement was correctly applied
 */
bool constantPropagation (SgStatement* stmt, SgVariableSymbol * var, int constToPropagate);

/**
 * \brief{Delete dead code into a Basic block.}
 * Search into all statement of a Basic Block if there a condtion where is always true or always false, or a sub basic block which is empty
 * and remove statement affected. This allows to keep a code clean specially when a function was specialize for example.
 * 
 * @param body - Basick Block (pointer on the Rose Basic block representation) to cross in search of dead code
 */
void dead_code_elimination(SgBasicBlock* body);

/**
 * \brief{Delete dead code into a Basic block.}
 * Search into all statement of a Basic Block if there a condtion where is always true or always false, or a sub basic block which is empty
 * and remove statement affected. This allows to keep a code clean specially when a function was specialize for example.
 * 
 * @param body - Basick Block (pointer on the Rose Basic block representation) to cross in search of dead code
 * @param varspe - vector of variable_spe to help to determine if an expression is always true or false.
 */
void dead_code_elimination(SgBasicBlock* body, std::vector<variable_spe*> varspe);
void dead_code_elimination (SgStatement* stmt, std::vector<variable_spe*> varspe);

/** Please use bool isAlwaysTrue (SgExpression* expr, std::vector<variable_spe*> varspe);
 * Check if the expression was always true; 
 * For the moment only usable for integer values
 *
 * @param expr - Expression (pointer on Rose Expression representation) to check
 */
bool isAlwaysTrue (SgExpression* expr);

/** 
 * Check if the expression was always true; 
 * For the moment only usable for integer values
 *
 * @param expr - Expression (pointer on Rose Expression representation) to check
 * @param varspe - vector of variable_spe to help to determine if the expression is always true.
 */
bool isAlwaysTrue (SgExpression* expr, std::vector<variable_spe*> varspe);

/** Please use bool isAlwaysTrue (SgExpression* expr, std::vector<variable_spe*> varspe);
 * Check if the expression was always false; 
 * For the moment only usable for integer values
 *
 * @param expr - Expression (pointer on Rose Expression representation) to check
 * @param varspe - vector of variable_spe to help to determine if the expression is always false.
 */
bool isAlwaysFalse(SgExpression* expr);

/**
 * Check if the expression was always false; 
 * A vector of variable specialization is use to help to determine 
 * 
 * For the moment only usable for integer values
 *
 * @param expr - Expression (pointer on Rose Expression representation) to check
 * @param varspe - vector of variable_spe to help to determine if the expression is always false.
 */
bool isAlwaysFalse(SgExpression* expr, std::vector<variable_spe*> varspe);

/**
 * Search in the same function as the reference of the variable where the variable was assigned of the last time. (to know the value if it's easy to handle)
 *
 * @param vre - Variable reference (pointer on the Rose representation of ht variable reference representation) to trace.
 * @return IntegerValue - Return an pointer on the Rose representation of the Integer value, which represent the last value known; 
 return NULL if not found or undeterminable.
 */
SgIntVal* trace_back_to_last_affectation(SgVarRefExp * vre);
bool isinExpr(SgExpression * expr, SgVariableSymbol* vsymb);

/**
 * My version of copy statement.
 * take better care of declarations an their types than SageInterface::copyStatement()
 * Do not copy declaration in the vactor of symbol (which represent some variable of which we already know the value),
 * and propagate values know throught other declarations (instead of the variable where it was used).
 *
 * @param body - BasickBlock (pointer on the Rose Basic block representation) to copy 
 * @param v_symbol - Vector of symbols (pointer on the Rose symbols representation) which don't have to be copied (for the declaration only)
 * @param val - Vector of integer which represent value of each symbol (one value per symbol)
 * @return basicblock - Return a copy from th body without some declaration of which we already know the value
 */
SgBasicBlock* copyBasicBlock(SgBasicBlock* body, std::vector<SgSymbol*> v_symbol, std::vector<int> val);

/**  
 * Create a basick block which only contains declaration from an other basic block 
 *
 * @param frombody - 
 */
// SgBasicBlock* extractDeclarations(SgBasicBlock* frombody);
void insert_declaration_into_function(SgNode * node, SgVariableDeclaration* decl);

/**
 * Search into a vector of variable_spe, a specific variable
 *
 * @param vre - 
 * @param varspe - 
 * @return variable_spe - 
 */
variable_spe* find(SgVarRefExp* vre, std::vector<variable_spe*> varspe);

void analyze_variables(SgGlobal * globalScope);
void addvprofCall (SgGlobal * globalScope);
void add_init_call (SgFunctionDefinition* main, int nbThreads, int model);
// void add_lib_call (SgFunctionDefinition* function, std::string varNames);
void add_lib_call (SgStatement* stmt, std::string varNames, std::string uniqueName="-1");
/**
 * return the number of element removed
 */
int remove_lib_call(SgStatement* stmt);
void use_lib_results(SgScopeStatement* scope, int nb_variable_spe, variable_spe* var_spe);

std::vector<SgStatement*> lookingForMarkedStmt(SgGlobal * root);
void lookingForMarkedStmt(SgBasicBlock *body, std::vector<SgStatement*> &stmtMarkedList);

// bool if_in_specialized_block (std::string varname,std::string compareType) { return false; }

/**
 * Create or use a file where all log where wrote
 *
 * @param what_change - 
 * @param node - 
 * @return writeInFile - 
 */
void log (std::string what_change, SgLocatedNode* node, bool writeInFile=true);

/**
 * Create or use a file where all log where wrote
 *
 * @param what_change - 
 * @param node - 
 * @return writeInFile - 
 */
void log (std::string what_change, std::string filename="");


//////////////////////////////////////////
//                                      //
//               FUNCTION               //
//                                      //
//////////////////////////////////////////
/**
 * Build a function declaration at the end of the file where func_to_cpy was found.
 *
 * @param func_to_cpy - Rose pointer on  a function declaration (header)
 * @param varl - vector of variable spe, which contains 
 * @param otherBody - Rose pointer on a basic block; if you want a special body for this function
 */
SgFunctionDeclaration * build_functionDecl(SgFunctionDeclaration* func_to_cpy, std::vector<variable_spe*> varl, SgBasicBlock* otherBody=NULL);

/**
 * Genretae a function and place it at the end of the file
 *
 * @param paramList - Rose pointer of a list of function parameters
 * @param func_name_str - Name of the function
 * @param scope - global scope where to insert the function
 */
SgFunctionDeclaration * generateFunction  (SgFunctionParameterList* paramList, const std::string& func_name_str, SgScopeStatement* scope);

/**
 * Generate a function skeleton 
 *
 * @param name - Name of the function 
 * @param ret_type - The type of the return
 * @param params - Possible parameters
 * @param scope - global scope where to insert the function
 */
SgFunctionDeclaration * createFuncSkeleton(const std::string& name, SgType* ret_type, SgFunctionParameterList* params, SgScopeStatement* scope);

//////////////////////////////
//      Static analyse      //
//////////////////////////////
/**
 * Analytic function to mesure the impact of a variable in a Basic Block
 */
void weightingVariables(SgBasicBlock* body, std::vector<var_metric*> &var_m_list);
int thereIsAStopStmt(SgStatement* stmt);
void whatSpecialize (SgStatement* sc   , std::vector<SgVariableSymbol*> &varList);
void whatSpecialize (SgExpression* expr, std::vector<SgVariableSymbol*> &varList);

} //end namespace s2s_api

#endif