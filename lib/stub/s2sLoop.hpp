#ifndef S2SLOOP
#define S2SLOOP

#include "rose.h"
#include "s2sDefines.hpp"
#include "api.hpp"

#include <sstream>
#include <string>
#include <iostream>
#include <vector>

class ASTLoops;
class ASTRoot;
class Loop;

class ASTRoot {
  protected:
    size_t id;                                // to match with maqao id loop
    SgGlobal* root;                           // Represent a node above the subtree of loops (it should be a SgNode*)
    std::vector<ASTLoops*> internalLoopsList; // Vector which contain list of all loops of the file

  public:
    //Constructor
    /**
     * Constructor from Rose pointer on the global scope of a file
     *
     * @param glob - Pointer on Rose representation of the global scope
     */
    ASTRoot(SgGlobal* glob);
    /**
     * Constructor from Rose pointer on a Basic block.
     *
     * @param body - Pointer on Rose representation of the global scope
     */
    ASTRoot(SgBasicBlock * body);
    
    //Destructor
    /**
     * Destructor of the Tree
     */
    ~ASTRoot();

    //Accessors
    /**
     * Getter of the list of all external loop of each function of the file.
     *
     * @return a vector of pointer on ASTLoops
     */
    std::vector<ASTLoops*> get_internalLoopsList();
    /**
     * Setter of the list of pointer on ASTLoops.
     *
     * @param vastloops - a vector of pointer on ASTLoops
     */
    void set_internalLoopsList(std::vector<ASTLoops*> vastloops);
    std::vector<Loop*> get_allInnermost(int deep = 1);

    //Miscellaneous
    /**
     * Print lines start and end of each loop.
     */
    void printASTLinesRanges();
    /**
     * Print loops which marks as not found 
     * Essentially use to compare which loops match with binary loops
     */
    void print_loop_not_found();
    /**
     * Print all information about all loops of the tree.
     * print the ID, the type of the loop and the start and the end line
     */
    void print();
    /**
     * Search, into the global tree, a loop near of the start/end lines
     */
    Loop* get_loop_from_line(int start,int end=-1);
    Loop* get_loop_from_label(std::string label);
    /**
     * Research, into the global tree, each loop containing a maqao directive and apply it.
     */
    void apply_directive();
    void apply_directive(std::vector<variable_spe*> varspe);
    /**
     * Return true if the gloabl tree is empty
     *
     * @return True if the tree is empty
     */
    bool empty();
    
  protected:
    /**
     * Create the tree from a basic block.
     * Only used in internal to register recursivly all loops from the file or from a specific basic block.
     */
    void createAST(SgBasicBlock *body);

};

static int identifierLoops=0;

/**
 * Loop class :
 * Represent all kind of loop, Do, For, While, etc
 * Some transformations are already implemented. 
 */
class Loop {
  public:
    enum loopType_ { DO, WHILE, DOWHILE, FOR }; // Enum to represent the loop type
  protected:
    size_t id;                                  // Used to match with maqao id loop, by default loops' id are set from 1 to n to the order of reading the source code  
    size_t parent_id;                           // Parent's id
    SgScopeStatement * loop;                    // Rose loop
    loopType_ typeOfLoop;                       // The loop type
    bool is_matching;                           // Only for internal used
    bool hasBeenTransformed;                     // To not try to apply a second transformation on a same loop

  public:
    //CONSTRUCTOR
    /**
     * Constructor of Loop 
     *
     * @param l - Pointer on Rose representation of a loop
     * @param pid - Id of the loop
     */
    Loop(SgScopeStatement* l, size_t pid);
    /**
     * Destructor
     */
    ~Loop(){}
    //ACCESSORS
    /**
     * Getter of the Rose representation of a loop
     * @return Pointer on Rose representation of loop
     */
    SgScopeStatement* get_loop_RosePtr();
    /**
     * Getter of the Rose representation of a loop
     * @return Pointer on Rose representation of loop
     */
    SgScopeStatement* get_loop_RosePtr() const;
    /**
     * Setter of the Rose representation of a loop
     * @paral l - Pointer on Rose representation of loop
     */
    void set_loop(SgScopeStatement* l);
    SgBasicBlock* get_body();
    SgExpression* get_bound();

    size_t get_id();
    size_t get_id() const;
    size_t get_parent_id() const;
    void set_parent_id(size_t);
    void set_id(size_t ID);

    loopType_ get_loopType();
    loopType_ get_loopType() const;
    void set_loopType(Loop::loopType_);
    void set_loopType(std::string loopKeyword);
    
    int  get_line_start();
    std::string get_line_start_str();
    int  get_line_end();
    std::string get_line_end_str();
    /**
     * Return true or false if the loop was already meets the binary loop conrresponding or not.
     * @return Return true if the loop was already meets the binary loop conrresponding
     */
    bool is_matching_with_bin_loop();
    void set_matching_with_bin_loop(bool);
    bool alreadyTransformed();
    /**
     * Read directives added from VPROF analyze
     * Extract information from directives
     *
     * Currently it return value if min = max = avg
     * and only return the first time all conditions are respected
     */
    int get_vprof_ite ();

    //miscellaneous
    /**
     * Return a list of directives attached to the loop.
     * @return A vector of string contaning each directive of the loop.
     */
    std::vector<std::string> get_maqao_directives();

    /**
     * Return the inner most loop.
     */
    // Loop* get_innermost_loop();

    /**
     * Print the type of the loop (Fortran do; C/C++ "for" ; while ; etc)
     * @return a string containing the type of the loop: "DO", "WHILE", "DOWHILE", "FOR" or "NOTHING" if the user didn't take a loop in the contructor.
     */
    std::string loopType_toString();
    /**
     * Print lines start and end of each loop.
     */
    void printLines();
    /**
     * Print all information about all loops of the tree.
     * print the ID, the type of the loop and the start and the end line
     */
    void print();
    /**
     * Attach a directive to the loop
     * @param s - string containing the directive line without !DIR$ 
     */
    void add_directive(std::string s);
    /**
     * Look for a maqao directive,
     * then apply the transformation requested.
     */
    void apply_directive();
    void apply_directive(std::vector<variable_spe*> varspe);
    bool apply_directive(std::string directive, std::vector<variable_spe*> *varspe=NULL);
    /**
     * Check if the loop has fix bounds
     */
    bool hasFixBounds();
    /**
     * Unroll the loop of the size indicate by the parameter.
     * Add a reminder loop after the unrolled loop
     * 
     * @param unrollSize - size of the unroll
     * @param reminder - Boolean to know if wehave to unroll and jam
     * @return bool - return true if the unroll was apply correctly
     */
    bool unroll (size_t unrollSize, bool reminder=false);

    /**
     * FullUnroll the loop but don't delete the loop to our the tree
     * @param unrollSize - size of the unroll
     * @return bool - return true if the fullunroll was apply correctly
     */
    bool fullUnroll(size_t unrollSize);
    /** Transform a loop with the "vectorization of the poor" patern
     * Example :
     *  Do i=1, X           |  Do i = 1, X 
     *    Do j=1, 7         |    !DIR$ SIMD
     *      <loopBody>      |    Do j = 1, 4 
     *    enddo             |      <loopBody>
     *  enddo               |    enddo
     *                      |    !DIR$ SIMD
     *                      |    Do j = 5, 6
     *                      |      <loopBody> 
     *                      |    enddo
     *                      |    j = 7
     *                      |    <loopBody>
     * @param useAVX - Boolean to indicate if the user want to use AVX instruction in the directive
     * @return bool - return true if the transformation was apply correctly
     */
    bool shortVecto(bool useAVX=true) ;
    /**
     * Generic version of the Block vecto 
     * use the rest of the modulo to detect in which case we are
     */
    bool shortVectoGen(int value);
    bool interchange(size_t depth, size_t lexicoOrder);
    /**
     * particular case of the tiling (apply on vector)
     */
    bool strip_mining(size_t tileSize);
    //! Tile the n-level (starting from 1) of a perfectly nested loop nest using tiling size s
    /** 
     * Translation
     * Before:
     *  for (i = 0; i < 100; i++)
     *    for (j = 0; j < 100; j++)
     *      for (k = 0; k < 100; k++)
     *        c[i][j]= c[i][j]+a[i][k]*b[k][j];
     * 
     *  After tiling i loop nest's level 3 (k-loop) with size 5, it becomes
     * 
     *  // added a new controlling loop at the outer most level
     *  int _lt_var_k;
     *  for (_lt_var_k = 0; _lt_var_k <= 99; _lt_var_k += 1 * 5) {
     *    for (i = 0; i < 100; i++)
     *      for (j = 0; j < 100; j++)
     *        // rewritten loop header , normalized also
     *        for (k = _lt_var_k; k <= (99 < (_lt_var_k + 5 - 1))?99 : (_lt_var_k + 5 - 1); k += 1) {
     *          c[i][j] = c[i][j] + a[i][k] * b[k][j];
     *        }
     *  }
     *  // finally run constant folding
    */
    bool tile(size_t tileSize);
    /**
     * Tile the inner loop only 
     */
    bool tile_inner_v2(size_t tileSize);
    bool isSplitable ();
    bool split ();
    /**
     * Check if a jam is possible, 
     * Used for the unroll and jam.
     * 
     * @return bool, return true if the loop is jamable.
     */
    bool isJamable();
    /**
     * jam a body after unroll
     * 
     * @param body - Body of a loop to jam
     * @return a besic block which is the body jamed
     */
    SgBasicBlock* jam_body(SgBasicBlock* body);
    /**
     * The name of the function says all you need to know
     * It specialize the loop progating constante, eliminate dead code
     * by using a list of variable and information we know about :
     * if their value is constant or its bounds, etc.
     */
    bool specialize(std::vector<variable_spe*> var);
    /**
     * look th number of iteration and eventually full unroll the loop
     * if the number of iteration is lower than 10
     */
    bool clean(int nb_ite);
    /**
     * Not implemented yet
     * The goal is simple : replace the loop by a text that the user wrote
     */
    bool replaceByText(std::string text);
    /**
     * Return a list of variable we want to instrument to analyze 
     * for an eventual specialization
     */
    void whatSpecialize (std::vector<SgVariableSymbol*> & varList);
};

/**
 * Middleclass of ASTRoot and Loop,
 * ASTLoop represent a loop and all possibles loops that can be contains in the body.
 *
 * Somme operation are overload to access to the loops eazily
 * As the operator [] or <<
 */
class ASTLoops {
  protected:    
    size_t parent_id;                          // parent's id
    std::vector<ASTLoops*> internalLoopsList;  // List of node which represent loops
    Loop * loop;                               // Loop

  public:
    //CONSTRUCTOR
    /**
     * Constructor of ASTLoops 
     * ASTLoops is build from a scope which represent a loop and look up inside the body 
     * to ather loops to fill the vector of Loops.
     */
    ASTLoops(SgScopeStatement* l, size_t parent_id=0);
    /**
     * Constructor by copy
     */
    ASTLoops(ASTLoops* astl);
    ~ASTLoops(); 
    //void fill(SgScopeStatement* scp);
        
    //ACCESSORS
    size_t get_parentId();
    size_t get_parentId() const;
    void set_parentId(size_t pid);
    void set_loop_id(size_t id);

    std::vector<ASTLoops*> get_internalLoopsList();
    std::vector<ASTLoops*> get_internalLoopsList() const;
    void set_internalLoopsList(std::vector<ASTLoops*>);
    std::vector<Loop*> get_allInnermost(int & deep);

    Loop* get_loop();
    Loop* get_loop() const;
    void set_loop(Loop* l, size_t parent_id);

    //OVERLOAD
    ASTLoops* operator [](size_t i) { return internalLoopsList[i]; }
    const ASTLoops* operator [](size_t i) const { return internalLoopsList[i]; }
    friend std::ostream& operator<<(std::ostream& out, const ASTLoops& ast);

    //MISCELLANEOUS
    /**
     * Print loops which are marks as not found 
     * Essentially use to compare which loops match with binary loops
     */
    void print_loop_not_found();
    /**
     * Print the start line and the end line of each loop of this subtree 
     */
    void printLines();
    /**
     * Print information about all loops of the subtree,
     * only print information about the current loop and loop which are in its body.
     */
    void print();
    /**
     * Return the depth of the loop nest
     */
    int get_depth();
    /**
     * Return the inner most loop.
     * If they're most than one loop in the body, return the first one
     */
    Loop* get_innermost_loop();
    /**
     * Search into the tree to search a loop near of the start and end lines.
     *
     * @param int start - the line number where the loop have to start.
     * @param int end - the line number where the loop have to end.
     * @return Loop* - return a pointer on the Loop if a loop where found at this lines, else return NULL.
     */
    Loop* get_loop_from_line(int start,int end=-1);
    Loop* get_loop_from_label(std::string label);
    /**
     * Look for which loop contains a maqao directive and apply it
     */
    void apply_directive();
    void apply_directive(std::vector<variable_spe*> varspe);
    /**
     * Return the number of loop at one level under this one (plus this one, and only at one level under and not recursively).
     *
     *@return int, return the size of Loop vector
     */
    int length() { return internalLoopsList.size()+1;}
    /**
     * Return a list of variable we want to instrument to analyze 
     * for an eventual specialization
     */
    void whatSpecialize (std::vector<SgVariableSymbol*> & varList, int deepth = 1);
  protected:
    /**
     * Create the tree from a basicBlock
     * Only used in internal to register recursivly all loops from the file.
     */
    void createAST(SgScopeStatement *body, size_t id);
};

bool handleStopKeyword (SgBasicBlock* body);

/**
 * Unroll a "for" Statement (C/C++)
 * Please use the ASTLoop tu perform transformations on loops.
 *
 * @param loop - Loop of type "for" (C/C++ style) 
 * @param nest - It's possible to choose the nest of the loop to unroll
 * @param unrollSize - the size of the unroll
 * @param reminder - to know if we have to unroll and jam
 * @return bool: return true if unroll transformation was applied
 */
bool forLoopUnroll(SgForStatement  * loop, size_t nest, size_t unrollSize, bool reminder);

/** 
 * Return a C/C++ For loop Unrolled in function of the unrollsize parameter 
 * take care to make a copy of the loop before, so please don't forget to set new loop's parents.
 *
 * @param SgForStatement  * loop: the loop to unroll
 * @param size_t unrollSize: Number of time to unroll the loop
 * @return a unroll copy of the loop
 */
SgForStatement * loopUnroll(SgForStatement  * loop, size_t unrollSize, bool reminder);

/** 
 * unroll a Fortran DO loop
 * Please use the ASTLoop tu perform transformations on loops.
 *
 * @param loop: the DO loop to unroll
 * @param unrollSize: the size of the unroll
 */
SgFortranDo * loopUnroll(SgFortranDo * loop, size_t unrollSize, bool reminder);

/**
 *
 */
bool tile_fortran_do(SgFortranDo* loop,size_t tileSize);
/**
 * Search the stride into an binary operation and return a pointer of it.
 * For Assign Operation //i = i + 1 
 *                        |    \_/
 *                        |     |
 *                       lhs   rhs
 *
 * In this example it will analyze the (i + 1) in this binaryOp, 
 * beceause it's the rhs of the affectation
 * @param binOp - Poiner on Rose representation of Binary operation
 * @return expression - return a pointer on the Rose representation of an Expression, this last one has his lhs modified
 */
SgExpression * get_stride_from_rhs_expression(SgExpression * binOp);

/**
 * Search the stride into two expression, usually represent the lhs and the rhs of a binary operation.

 * @param lhs - Poiner on Rose representation of an Expression
 * @param rhs - Poiner on Rose representation of an Expression 
 * @return expression - return a pointer on the Rose representation of an Expression, this last one has is the Expression modified
 */
SgExpression * get_stride_from_rhs_expression(SgExpression * lhs, SgExpression * rhs);

/**
 * Search the stride into an expression and return the stride modified for the unroll transformation.
 *
 * param increment - Pointer on the Rose representation of an expression, 
 this expression represent the stride of a loop.
 * @param unrollSize - represent the size of the unroll.
 * @return - return the expression modified for the unroll transformation.
 */
SgExpression * get_unroll_stride(SgExpression * increment, size_t unrollSize);

/** 
 * Verify if the condition can be handle and return the modified condition(for unroll) or NULL if it can't.
 *
 * @param loop - the loop to handle (for C/C++ style).
 * @param unrollSize - the size of the unroll
 * @return - return the new condition for the unroll
 */
SgExpression * get_unroll_cond(SgForStatement * loop, size_t unrollSize);

/** 
 * My version of the build of a "for" statement (C/C++ style)
 * We take care of the initialization of the for contrary to Rose.
 *
 * @param initialize_stmt - Pointer on the Rose representation of an Expression statement( initialisation of a for loop)
 * @param test - Pointer on the Rose representation of an Expression (The stop condition of the loop)
 * @param loop_body - Pointer on the Rose representation of a scope (the body of the loop)
 * @return A pointer on the Rose representation of a For statement (C/C++ style)
 */
SgForStatement * myBuildForStatement(SgStatement* initialize_stmt, SgStatement * test, SgExpression * increment, SgStatement * loop_body/*, SgStatement * else_body = NULL*/);


//////////////////////////////////////////
//                                      //
//               LOOP                   //
//                                      //
//////////////////////////////////////////
/**
 * Search into a vector of variable_spe, a specific variable
 *
 * @param vre - 
 * @param varspe - 
 * @return variable_spe - 
 */
Loop* get_loop_from_line(ASTRoot* ast, int lstart, int lend=-1, int delta=0);
#endif
