#include "s2sLoop.hpp"
#include "api.hpp"

ASTRoot createASTLoop(SgSourceFile* file) {
  SgGlobal *root = file->get_globalScope();
  
  ASTRoot * ast = new ASTRoot(root);
}

//! A helper function to return a permutation order for n elements based on a lexicographical order number
std::vector<size_t> getPermutationOrder_s2s( size_t n, size_t lexicoOrder)
{
  size_t k = lexicoOrder;
  std::vector<size_t> s(n);
  int result = 1;
  // initialize the permutation vector
  for (size_t i=0; i<n; i++)
    s[i]=i;

  for (size_t i=2; i<=n-1; i++) result*=i;
  //compute (n- 1)!
  size_t factorial = result;
  //check if the number is not in the range of [0, n! - 1]
  if (k/n>=factorial)
  {
    printf("Error: in getPermutationOrder(), lexicoOrder is larger than n!-1\n");
    ROSE_ASSERT(false);
  }
  // Algorithm:
  //check each element of the array, excluding the right most one.
  //the goal is to find the right element for each s[j] from 0 to n-2
  // method: each position is associated a factorial number
  //    s[0] -> (n-1)!
  //    s[1] -> (n-2)! ...
  // the input number k is divided by the factorial at each position (6, 3, 2, 1 for size =4)
  //   so only big enough k can have non-zero value after division
  //   0 value means no change to the position for the current iteration
  // The non-zero value is further modular by the number of the right hand elements of the current element.
  //     (mode on 4, 3, 2 to get offset 1-2-3, 1-2, 1 from the current position 0, 1, 2)
  //  choose one of them to be moved to the current position,
  //  shift elements between the current and the moved element to the right direction for one position
  for (size_t j=0; j<n-1; j++)
  {
    //calculates the next cell from the cells left
    //(the cells in the range [j, s.length - 1])
    int tempj = (k/factorial) % (n - j);
    //Temporarily saves the value of the cell needed
    // to add to the permutation this time
    int temps = s[j+tempj];
    //shift all elements to "cover" the "missing" cell
    //shift them to the right
    for (size_t i=j+tempj; i>j; i--)
    {
      s[i] = s[i-1]; //shift the chain right
    }
    // put the chosen cell in the correct spot
    s[j]= temps;
    // updates the factorial
    factorial = factorial /(n-(j+1));
  }
  return s;
}

//////////////////////////////////////////
//                                      //
//              ASTRoot                //
//                                      //
//////////////////////////////////////////

void ASTRoot::createAST(SgBasicBlock *body) {
  if(DEBUG) DBG_MAQAO
  SgStatementPtrList & stmtsList = body->get_statements();
  /**
   * Browse recursively all stmt looking for all kind of loops
   * Push back a node into the internal list create a node with the loop found
   * The creation of the node will trigger the search of internal loop recursively too
   */
  for (SgStatementPtrList::iterator p = stmtsList.begin(); p != stmtsList.end(); ++p) 
  {
    if(SgForStatement  * loop = isSgForStatement(*p)) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgFortranDo * loop = isSgFortranDo(*p)) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgDoWhileStmt * loop = isSgDoWhileStmt(*p)) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgWhileStmt  * loop = isSgWhileStmt(*p)) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgIfStmt * ifstmt = isSgIfStmt(*p)) {
      if(SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ()))
        createAST(bodytrue);

      if(SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ()))
        createAST(bodyfalse);
  
    } else if (SgSwitchStatement* switchStmt = isSgSwitchStatement(*p)) {
        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body()))
          createAST(subbody);
    } else if (SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body()))
          createAST(subbody);
    } else if (SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body()))
          createAST(subbody);
    } else if (SgBasicBlock* subbody = isSgBasicBlock(*p)) {
      createAST(subbody);
    } else if (SgScopeStatement * scp = isSgScopeStatement(*p)) {
        // if(DEBUG > 1)
          std::cout << "-----THIS SCOPE WAS NOT HANDLED YET: " << scp->class_name() << std::endl;
        // /!\
        // This function do its job, 
        // but if we launch an other analyze just after it crash because it doesn't delete something in the memory pool
        // so, don't use it plz !
        //createAST(SageBuilder::buildBasicBlock_nfi(scp->getStatementList()));
    } 
    #if 0
    // Not handle yet
    else if (SgExprStatement* exprstmt = isSgExprStatement(*p)) { // array(:) or array (:,:,...,:)
      if (SgAssignOp * assign = isSgAssignOp(exprstmt->get_expression())) {
        if (SgPntrArrRefExp* arrayRef = isSgPntrArrRefExp(assign)) {
          if (SgExprListExp* exlex = isSgExprListExp(arrayRef->get_rhs_operand())) {
            SgExpressionPtrList & exList = exlex->get_expressions ();
            for(int i=0; i < exList.size(); i++) {
              if (SgSubscriptExpression * subscrpt = isSgSubscriptExpression(exList[i])) {
                // Not handle Yet
                // STLoops* newNode = new ASTLoops(exprstmt, id);
                // internalLoopsList.push_back(newNode);
              }
            }

          } else if (SgSubscriptExpression * subscrpt = isSgSubscriptExpression(arrayRef->get_rhs_operand())) {
            // Not handle yet.
            // ASTLoops* newNode = new ASTLoops(exprstmt, id);
            // internalLoopsList.push_back(newNode);
            // No body to analyze
          }
        }
      }
    }
    #endif
  }
}
/**
 * Constructor which will create an AST of only loops
 */
ASTRoot::ASTRoot(SgGlobal* globalScope):root(globalScope) {
  if(DEBUG) DBG_MAQAO
  identifierLoops = 0;
  id = identifierLoops++;

  SgDeclarationStatementPtrList& declList = globalScope->get_declarations ();

  for (SgDeclarationStatementPtrList::iterator it = declList.begin(); it != declList.end(); ++it) {
    if (SgModuleStatement* modstmt = isSgModuleStatement(*it)) {
      SgClassDefinition * modDef =  modstmt->get_definition ();
      SgDeclarationStatementPtrList & funcList =  modDef->getDeclarationList ();
      for (SgDeclarationStatementPtrList::iterator it = funcList.begin(); it != funcList.end(); ++it) {
        SgFunctionDeclaration *func = isSgFunctionDeclaration(*it);
        if (func == NULL)
          continue;
        SgFunctionDefinition *defn = func->get_definition();
        if (defn == NULL)
          continue;

        SgBasicBlock *body = defn->get_body();
        createAST(body);
      }
    } else 
    {
      SgFunctionDeclaration *func = isSgFunctionDeclaration(*it);
      if (func == NULL)
        continue;
      SgFunctionDefinition *defn = func->get_definition();
      if (defn == NULL)
        continue;

      SgBasicBlock *body = defn->get_body();
      createAST(body);
    }
  }
}

//Search the global scope ...
ASTRoot::ASTRoot(SgBasicBlock * body) {
  if(DEBUG) DBG_MAQAO
  identifierLoops = 0;
  id = identifierLoops++;

  if (SgGlobal* g = isSgGlobal(body)) 
    ASTRoot(isSgGlobal(body));
  else {
    root = isSgGlobal(body->get_parent());
    while(!root) {
      root = isSgGlobal(root->get_parent());
    }
    createAST(body);
  }
}

/**
 * Destructor of the tree
 * Do not delete pointer, because the pool into Rose will make it at the end
 */
ASTRoot::~ASTRoot() {
  if(DEBUG) DBG_MAQAO
  root = NULL;
  for(int i=0; i < internalLoopsList.size(); ++i) {
    if(internalLoopsList[i]) {
      internalLoopsList[i] = NULL;
      delete(internalLoopsList[i]);
    }
  }
}

/**
 * Accessor 
 */
std::vector<ASTLoops*> ASTRoot::get_internalLoopsList() { return internalLoopsList; }
void ASTRoot::set_internalLoopsList(std::vector<ASTLoops*> vectorList) {
  if(DEBUG) DBG_MAQAO 
  int i;
  for(i=0; i < vectorList.size(); i++) {
    internalLoopsList[i] = vectorList[i];
  }
  for (int j = internalLoopsList.size(); j > i; --j) {
    internalLoopsList.pop_back();
  }
}

std::vector<Loop*> ASTRoot::get_allInnermost(int deep/*=1*/) {
  if(DEBUG) DBG_MAQAO 
  std::vector<Loop*> innermostLoops;

  for(int i=0; i < internalLoopsList.size(); i++) {
    std::vector<Loop*> internalInnermost = internalLoopsList[i]->get_allInnermost(deep);
    
      if (innermostLoops.empty()) {
        innermostLoops = internalInnermost;
        deep -= 1;
      } else if (deep > 0) {
        innermostLoops.insert(innermostLoops.end(), internalInnermost.begin(), internalInnermost.end());
      }
  }
  return innermostLoops;
}

/**
 * Print all finromation about loop lines (start and end lines)
 */
void ASTRoot::printASTLinesRanges() {
  if(DEBUG) DBG_MAQAO
  for(int i=0; i < internalLoopsList.size(); ++i) {
    internalLoopsList[i]->printLines();
  }
  std::cout << std::endl;
}

/** 
 * Print lines of loops which marked as not founded
 * Durning the search of loop from binary information we mark loops which matched with bin info
 */
void ASTRoot::print_loop_not_found(){
  if(DEBUG) DBG_MAQAO
  for(int i=0; i < internalLoopsList.size(); ++i) {
    internalLoopsList[i]->print_loop_not_found();
  }
}

void ASTRoot::print() {
  if(DEBUG) DBG_MAQAO
  std::cout << "ROOT id = " << id << std::endl;

  for(int i=0; i < internalLoopsList.size(); ++i) {
    internalLoopsList[i]->print();
  }
}

bool ASTRoot::empty() {
  if(DEBUG) DBG_MAQAO
  return internalLoopsList.empty();
}

Loop* ASTRoot::get_loop_from_line(int start, int end/*=-1*/) {
  if(DEBUG) DBG_MAQAO
  Loop* loopFound = NULL;
  for(int i=0; i < internalLoopsList.size(); ++i) {
    loopFound = internalLoopsList[i]->get_loop_from_line(start,end);
    if (loopFound) return loopFound;
  }

  return loopFound;
}

Loop* ASTRoot::get_loop_from_label(std::string label) {
  if(DEBUG) DBG_MAQAO
  Loop* loopFound = NULL;
  for(int i=0; i < internalLoopsList.size(); ++i) {
  if(DEBUG) DBG_MAQAO
    loopFound = internalLoopsList[i]->get_loop_from_label(label);
    if (loopFound) return loopFound;
  }

  return loopFound;
}

void ASTRoot::apply_directive() {
  if(DEBUG) DBG_MAQAO
  for(int i=0; i < internalLoopsList.size(); ++i) {
    internalLoopsList[i]->apply_directive();
  }
}

//////////////////////////////////////////
//                                      //
//              ASTLoops                //
//                                      //
//////////////////////////////////////////
std::ostream& operator<<(std::ostream& out, const ASTLoops& ast) {
  for(size_t i=0; i < ast.get_internalLoopsList().size(); ++i) {
    out << "(" << ast[i]->get_loop()->get_loopType() << ")  ";
  }
  out << std::endl;
  
  return out;
}

void ASTLoops::createAST(SgScopeStatement *body, size_t id) {
  if(DEBUG) DBG_MAQAO
  SgStatementPtrList & stmtsList = body->getStatementList();
  //Same as for ASTRoot
  for( int i = 0; i < stmtsList.size(); i++)
  {
    if(SgForStatement  * loop = isSgForStatement(stmtsList[i])) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgFortranDo * loop = isSgFortranDo(stmtsList[i])) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgDoWhileStmt * loop = isSgDoWhileStmt(stmtsList[i])) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgWhileStmt  * loop = isSgWhileStmt(stmtsList[i])) {
      ASTLoops* newNode = new ASTLoops(loop, id);
      internalLoopsList.push_back(newNode);

    } else if (SgIfStmt * ifstmt = isSgIfStmt(stmtsList[i])) {
      if(SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ()))
        createAST(bodytrue, id);

      if(SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ()))
        createAST(bodyfalse, id);
  
    } else if (SgSwitchStatement* switchStmt = isSgSwitchStatement(stmtsList[i])) {
        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body()))
          createAST(subbody, id);
    } else if (SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(stmtsList[i])) {
      if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body()))
          createAST(subbody, id);
    } else if (SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(stmtsList[i])) {
      if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body()))
          createAST(subbody, id);
    } else if (SgBasicBlock* subbody = isSgBasicBlock(stmtsList[i])) {
      createAST(subbody, id);
    } else if (SgScopeStatement * scp = isSgScopeStatement(stmtsList[i])) {
        // if(DEBUG > 1)
          std::cout << "-----THIS SCOPE WAS NOT HANDLED YET: " << scp->class_name() << std::endl;
        // /!\
        // This function do its job, 
        // but if we launch an other analyze just after it crash because it doesn't delete something in the memory pool
        // so, don't use it plz !
        //createAST(SageBuilder::buildBasicBlock_nfi(scp->getStatementList()));
    } 
    #if 0
    // Not handle yet
    else if (SgExprStatement* exprstmt = isSgExprStatement(stmtsList[i])) { // array(:) or array (:,:,...,:)
      if (SgAssignOp * assign = isSgAssignOp(exprstmt->get_expression())) {
        if (SgPntrArrRefExp* arrayRef = isSgPntrArrRefExp(assign)) {
          if (SgExprListExp* exlex = isSgExprListExp(arrayRef->get_rhs_operand())) {
            SgExpressionPtrList & exList = exlex->get_expressions ();
            for(int i=0; i < exList.size(); i++) {
              if (SgSubscriptExpression * subscrpt = isSgSubscriptExpression(exList[i])) {
                // Not handle Yet
                // STLoops* newNode = new ASTLoops(exprstmt, id);
                // internalLoopsList.push_back(newNode);
              }
            }

          } else if (SgSubscriptExpression * subscrpt = isSgSubscriptExpression(arrayRef->get_rhs_operand())) {
            // Not handle yet.
            // ASTLoops* newNode = new ASTLoops(exprstmt, id);
            // internalLoopsList.push_back(newNode);
            // No body to analyze
          }
        }
      }
    }
    #endif
  }
}

ASTLoops::ASTLoops(SgScopeStatement* l, size_t parent_id):parent_id(parent_id) {
  loop = new Loop(l,parent_id);
  createAST(l,parent_id);
}

ASTLoops::ASTLoops(ASTLoops* ast) {
  if(this != ast) {
    //set_loopType(ast->get_loop_RosePtr()->get_loopType());
    set_loop(ast->get_loop(), ast->get_parentId());
    set_internalLoopsList(ast->get_internalLoopsList());
    set_parentId(ast->get_parentId());
  }
}

ASTLoops::~ASTLoops() {
  if( loop ) {
    loop = NULL;
    delete(loop);
  }
  for(int i=0; i < internalLoopsList.size(); ++i) {
    if(internalLoopsList[i]) {
    internalLoopsList[i] = NULL;
        delete(internalLoopsList[i]);
    }
  }
}

//ACCESSORS
size_t ASTLoops::get_parentId() { return parent_id; }
size_t ASTLoops::get_parentId() const { return parent_id; }
void ASTLoops::set_parentId(size_t pid) { parent_id = pid; }
void ASTLoops::set_loop_id(size_t id) { 
  loop->set_id(id);
  for(size_t i=0; i < internalLoopsList.size(); ++i) {
   internalLoopsList[i]->get_loop()->set_parent_id(id);
  }
}

std::vector<ASTLoops*> ASTLoops::get_internalLoopsList(){ return internalLoopsList; }
std::vector<ASTLoops*> ASTLoops::get_internalLoopsList() const{ return internalLoopsList; }
void ASTLoops::set_internalLoopsList(std::vector<ASTLoops*>){
  // Not implemented yet
}

std::vector<Loop*> ASTLoops::get_allInnermost(int & deep) {
  if(DEBUG) DBG_MAQAO 
  std::vector<Loop*> innermostLoops;

  if (internalLoopsList.empty() && deep > 0) {
    innermostLoops.push_back(loop);
    deep-=1;
  } else {
    for(int i=0; i < internalLoopsList.size(); i++) {
      std::vector<Loop*> vectmp = internalLoopsList[i]->get_allInnermost(deep);
      innermostLoops.insert(innermostLoops.end(), vectmp.begin(), vectmp.end());
      if (deep > 0) {
        innermostLoops.push_back(loop);
        deep -= 1;
      }
    }
  }
  return innermostLoops;
}

Loop* ASTLoops::get_loop(){ return loop; }
Loop* ASTLoops::get_loop() const{ return loop; }
void ASTLoops::set_loop(Loop* l, size_t pid) {
  loop = l;
  loop->set_parent_id(pid);
  parent_id = pid;
  switch(l->get_loop_RosePtr()->variantT()) {
    case V_SgFortranDo:
      if ( DEBUG > 1) std::cout << "V_SgFortranDo\n";

      loop->set_loopType(Loop::DO);
      break;
    case V_SgWhileStmt : 
      if ( DEBUG > 1) std::cout << "V_SgWhileStmt\n";

      loop->set_loopType(Loop::WHILE);
      break;
    case V_SgDoWhileStmt : 
      if ( DEBUG > 1) std::cout << "V_SgDoWhileStmt\n";

      loop->set_loopType(Loop::DOWHILE);
      break;
    case V_SgForStatement:
      if ( DEBUG > 1) std::cout << "V_SgForStatement\n";

      loop->set_loopType(Loop::FOR);
      break;
    default: {
      if ( DEBUG ) {
        std::cout << "default case : " << l->get_loop_RosePtr()->class_name() << std::endl;
        std::cout << "The statement ("<< l->get_loop_RosePtr()->class_name() <<") is not a loop" << std::endl;
      }
      assert(false);
    }
  }
  SgStatementPtrList stmtList = l->get_loop_RosePtr()->generateStatementList();
  
  for (SgStatementPtrList::iterator p = stmtList.begin(); p != stmtList.end(); ++p) {
    if (SgScopeStatement * scope = isSgScopeStatement(*p)) {
      ASTLoops* newNode = new ASTLoops(scope, loop->get_id());
      internalLoopsList.push_back(newNode);     
    }

  }
}

void ASTLoops::print_loop_not_found() {
  if(!loop->is_matching_with_bin_loop()) {
    loop->printLines();
  }
  for(size_t i=0; i < internalLoopsList.size(); ++i) {
   internalLoopsList[i]->print_loop_not_found();
  }
}

void ASTLoops::printLines() {
  loop->printLines();
  for(size_t i=0; i < internalLoopsList.size(); ++i) {
   internalLoopsList[i]->printLines();
  }
}

void ASTLoops::print() {
  std::cout << " id "<< loop->get_id() << std::endl;//<< " parent id " <<  loop->get_parent_id() << " ";
  std::cout << "Number of children = " << internalLoopsList.size() << std::endl;
  loop->print();

  for(size_t i=0; i < internalLoopsList.size(); ++i) {
   internalLoopsList[i]->print();
  }
}

int ASTLoops::get_depth() {
  int max = 0;
  for (int i=0; i < internalLoopsList.size(); i++) {
    if (internalLoopsList[i]->get_depth() > max) max = internalLoopsList[i]->get_depth();
  }
  return 1 + max;
}

Loop* ASTLoops::get_innermost_loop() {
  if (loop) {
    if (internalLoopsList.empty()) return loop;
    else {
      return internalLoopsList[0]->get_innermost_loop();
    }
  }
  else 
    return NULL;
}

Loop* ASTLoops::get_loop_from_line(int start,int end/*=-1*/) {
  if(DEBUG) DBG_MAQAO
  if (((loop->get_line_start() == start /* && end   - loop->get_line_end()   < 100 && end   - loop->get_line_end()   > -100 */))
      || ((loop->get_line_end() == end  /* && start - loop->get_line_start() < 100 && start - loop->get_line_start() > -100 */)) 
    && loop->is_matching_with_bin_loop() == false ) return loop;

  Loop* loopFound = NULL;
  for(int i=0; i < internalLoopsList.size(); ++i) {
    loopFound = internalLoopsList[i]->get_loop_from_line(start,end);
    if (loopFound) return loopFound;
  }
  return NULL;
}

Loop* ASTLoops::get_loop_from_label(std::string label) {
  if(DEBUG) DBG_MAQAO
  std::vector<std::string> maqao_dir = s2s_api::get_MAQAO_directives(loop->get_loop_RosePtr());
  for(int i=0; i < maqao_dir.size(); i++) {
    // std::cout << "compare : " << maqao_dir[i].substr(maqao_dir[i].find("!DIR$ MAQAO ")+12) << " to " << label << std::endl;
    //if (maqao_dir[i].substr(maqao_dir[i].find("!DIR$ MAQAO ")+12) == label) {
    if (maqao_dir[i].substr(maqao_dir[i].find("MAQAO ")+6) == label) {
      return loop;
    }
  }

  Loop* loopFound = NULL;
  for(int i=0; i < internalLoopsList.size(); ++i) {
    loopFound = internalLoopsList[i]->get_loop_from_label(label);
    if (loopFound) return loopFound;
  }
  return NULL;
}

void ASTLoops::apply_directive() {
  if(DEBUG) DBG_MAQAO
  for(int i=0; i < internalLoopsList.size(); ++i) {
    internalLoopsList[i]->apply_directive();
  }
  loop->apply_directive();
}

void ASTLoops::apply_directive(std::vector<variable_spe*> varspe) {
  if(DEBUG) DBG_MAQAO
  //Update
  createAST(loop->get_loop_RosePtr(), parent_id);

  for(int i=0; i < internalLoopsList.size(); ++i) {
    internalLoopsList[i]->apply_directive(varspe);
  }

  loop->apply_directive(varspe);
}

//Use for neutral network convolution code
/**
 * Descendre au plus profond du nid de boucle pour trouver tous ce qui nous embete : if / ...
 * une fois à la boucle la plus profonde, créer une liste d'éléments à analyser vide.
 * puis on regarde dans le corps de la boucle  la recherche de if ou autre éléments qui pourrait nous embeter
 * Le tout en partant du dernier statement jusqu'au premier
 * Si on en trouve on regarde les variables dans la condition et on les ajoute tous dans la liste des élément à analyser
 * On continue de remonter et si on trouve une affectation avec un lhs qui est dans notre liste,
 *  on le retire de la liste et on ajoute toutes les variables du rhs

 * Une fois le corps analysé, on regarde les condition de départ et d'arrivé de la boucle
 * Si ce sont des vériables alors on les ajoute à la liste de variable à analyzer que l'on aura crée dans cette fonction
 * Si c'est en dur on a rien à spécialiser.
 * 
 */
void ASTLoops::whatSpecialize (std::vector<SgVariableSymbol*> & varList, int depth/*=1*/) {
  if(DEBUG) DBG_MAQAO
  
  //Check internal loops in the nest
  for (int i=0; i < internalLoopsList.size(); ++i) {
    // std::vector<SgVariableSymbol*> tmpVarList = internalLoopsList[i]->whatSpecialize();
    // varList.insert(varList.end(), tmpVarList.begin(), tmpVarList.end());
    internalLoopsList[i]->whatSpecialize(varList);
  }

  //Check the current loop
  loop->whatSpecialize(varList);

  int DeepOfInnerMostLoops = depth;
  // The list of innermost loops, just in case where there are not only one loop
  std::vector<Loop*> innermostLoops = get_allInnermost(DeepOfInnerMostLoops);
  for (int i=0; i < innermostLoops.size(); ++i) {
    if (innermostLoops[i]->get_loopType() == Loop::FOR) {
      SgForStatement* l = isSgForStatement(innermostLoops[i]->get_loop_RosePtr());
      // Check bounds
      SgExpression* cond = innermostLoops[i]->get_bound();
      s2s_api::whatSpecialize(cond, varList);

      // Check init
      SgStatementPtrList & forinitList = l->get_for_init_stmt()->get_init_stmt ();
      for (int i=0; i < forinitList.size(); ++i) {
        if (SgExprStatement* exprInit = isSgExprStatement(forinitList[i])) {
          if(SgBinaryOp* init = isSgBinaryOp(exprInit->get_expression())) {
            // Check right side
            s2s_api::whatSpecialize(init->get_rhs_operand_i(), varList);

            //Check left side if it's a var ref (not an array)
            if (SgVarRefExp* varRef = isSgVarRefExp(init->get_lhs_operand_i())) {
              SgVariableSymbol* varSymLHS = varRef->get_symbol();
              for (int i=0; i < varList.size(); ++i) {
                // If we found it we can reove it and exit, because there only one iteration of the variable in the list
                if (varSymLHS->get_name().getString() == varList[i]->get_name().getString()) {
                  // Remove the variable which are not usefull to follow
                  if(DEBUG > 1) std::cout << "ERASE : " << varList[i]->get_name().getString() << std::endl;
                  varList.erase(varList.begin()+i);
                  break;
                }
              }
            }
          }
        }
      }
    } else if (innermostLoops[i]->get_loopType() == Loop::DO) {
      SgFortranDo* l = isSgFortranDo(innermostLoops[i]->get_loop_RosePtr());
      // Check bounds
      SgExpression* cond = innermostLoops[i]->get_bound();
      s2s_api::whatSpecialize(cond, varList);

      // Check init
      if(SgBinaryOp* init = isSgBinaryOp(l->get_initialization ())) {
        // Check right side
        s2s_api::whatSpecialize(init->get_rhs_operand_i(), varList);

        //Check left side if it's a var ref (not an array)
        if (SgVarRefExp* varRef = isSgVarRefExp(init->get_lhs_operand_i())) {
          SgVariableSymbol* varSymLHS = varRef->get_symbol();
          for (int i=0; i < varList.size(); ++i) {
            // If we found it we can reove it and exit, because there only one iteration of the variable in the list
            if (varSymLHS->get_name().getString() == varList[i]->get_name().getString()) {
              // Remove the variable which are not usefull to follow
              if(DEBUG > 1) std::cout << "ERASE : " << varList[i]->get_name().getString() << std::endl;
              varList.erase(varList.begin()+i);
              break;
            }
          }
        }
      }
    } else {
        std::cout << "Loop format is not handle : " << innermostLoops[i]->loopType_toString() << std::endl;
    }
  }
}

//////////////////////////////////////////
//                                      //
//                Loop                  //
//                                      //
//////////////////////////////////////////

Loop::Loop(SgScopeStatement* sct, size_t pid):loop(sct), parent_id(pid) {
  switch(sct->variantT()) {
    case V_SgFortranDo:
      if ( VERBOSE > 1)
        std::cout << "V_SgFortranDo\n";

      set_loopType(DO);
      break;
    case V_SgWhileStmt : 
      if ( VERBOSE > 1)
        std::cout << "V_SgWhileStmt\n";

      set_loopType(WHILE);
      break;
    case V_SgDoWhileStmt : 
      if ( VERBOSE > 1)
        std::cout << "V_SgDoWhileStmt\n";

      set_loopType(DOWHILE);
      break;
    case V_SgForStatement:
      if ( VERBOSE > 1)
        std::cout << "V_SgForStatement\n";
 
      set_loopType(FOR);
      break;
    default: {
      if (VERBOSE)
        std::cout << "The statement ("<< sct->class_name() <<") is not a loop" << std::endl;
      assert(false);
    }
  }
  //Take a new ID for this new loop
  id = identifierLoops++;
  is_matching = false;
  hasBeenTransformed = false;
}

Loop::loopType_ Loop::get_loopType() { return typeOfLoop; }
Loop::loopType_ Loop::get_loopType() const { return typeOfLoop; }
void Loop::set_loopType(Loop::loopType_ lt){ typeOfLoop = lt; }
void Loop::set_loopType(std::string loopKeyword){
  if(loopKeyword == "DO") typeOfLoop = DO;
  else if (loopKeyword == "WHILE" )   typeOfLoop = WHILE;
  else if (loopKeyword == "DOWHILE" ) typeOfLoop = DOWHILE;
  else /*if (loopKeyword == "FOR" )*/ typeOfLoop = FOR;
}
SgScopeStatement* Loop::get_loop_RosePtr() { return loop; }
SgScopeStatement* Loop::get_loop_RosePtr() const { return loop; }
void Loop::set_loop(SgScopeStatement* l) {
  loop = isSgScopeStatement(l);

  switch(l->variantT()) {
    case V_SgFortranDo:
      if (VERBOSE > 1)
        std::cout << "V_SgFortranDo\n";
      set_loopType(DO);
      break;
    case V_SgWhileStmt : 
      if (VERBOSE > 1)
        std::cout << "V_SgWhileStmt\n";

      set_loopType(WHILE);
      break;
    case V_SgDoWhileStmt : 
      if (VERBOSE > 1)
        std::cout << "V_SgDoWhileStmt\n";

      set_loopType(DOWHILE);
      break;
    case V_SgForStatement:
      if (VERBOSE > 1)
        std::cout << "V_SgForStatement\n";

      set_loopType(FOR);
      break;
    default: {
      if (VERBOSE) {
        std::cout << "default case : " << l->class_name() << std::endl;
        std::cout << "The statement ("<< l->class_name() <<") is not a loop" << std::endl;
      }

      assert(false);
    }
  }
  is_matching = false;
}

size_t Loop::get_id() { return id; }
size_t Loop::get_id() const { return id; }
size_t Loop::get_parent_id() const { return parent_id; }
void Loop::set_parent_id(size_t pid) { parent_id = pid; }
void Loop::set_id(size_t ID) { id = ID; }
bool Loop::is_matching_with_bin_loop() { return is_matching; }
void Loop::set_matching_with_bin_loop(bool aff) { is_matching = aff; }
bool Loop::alreadyTransformed() { return hasBeenTransformed; }
int  Loop::get_line_start() { return loop->get_startOfConstruct ()->get_line(); }
std::string Loop::get_line_start_str() {
  std::ostringstream convert;   
  convert << loop->get_startOfConstruct ()->get_line();
  return convert.str();
}
int  Loop::get_line_end() { return loop->get_endOfConstruct ()->get_line(); }
std::string Loop::get_line_end_str() {
  std::ostringstream convert;   
  convert << loop->get_endOfConstruct ()->get_line();
  return convert.str();
}

SgBasicBlock* Loop::get_body() {
  switch(typeOfLoop) {
    case DO : {    
      SgStatement* bb = isSgFortranDo(loop)->get_body ();
      if (isSgBasicBlock(bb)) {  
        return isSgFortranDo(loop)->get_body ();
      }
      else {
        return SageBuilder::buildBasicBlock(bb);
      }
    }
    case WHILE : {   
      SgStatement* bb = isSgWhileStmt(loop)->get_body ();
      if (isSgBasicBlock(bb)) {
        return isSgBasicBlock(bb);
      }
      else {
        SgBasicBlock* body = SageBuilder::buildBasicBlock(bb);
        return body;
      }
    }
    case DOWHILE : { 
      SgStatement* bb = isSgDoWhileStmt(loop)->get_body ();
      if (isSgBasicBlock(bb)) {
        return isSgBasicBlock(bb);
      }
      else {
        return SageBuilder::buildBasicBlock(bb);
      }
    }
    case FOR : {     
      SgStatement* bb = isSgForStatement(loop)->get_loop_body ();
      if (isSgBasicBlock(bb)) {
        return isSgBasicBlock(bb);
      }
      else {
        return SageBuilder::buildBasicBlock(bb);
      }
    }
    default :      
      return NULL;
  }
}

SgExpression* Loop::get_bound() {
  if (DEBUG) DBG_MAQAO
  if (SgFortranDo * fortranloop = isSgFortranDo(loop)) {
    return fortranloop->get_bound();
  } else if (SgForStatement* forloop = isSgForStatement(loop)) {
    if (SgExprStatement* expr = isSgExprStatement(forloop->get_test()))
    return expr->get_expression();
  } 

  return NULL;
}

std::vector<std::string> Loop::get_maqao_directives() {
  std::vector<std::string> directives;
  //if is a fortran file
  if (currentFileType == "fortran") {
    if(AttachedPreprocessingInfoType* dir = loop->getAttachedPreprocessingInfo()) {
      AttachedPreprocessingInfoType::iterator i;
      for (i = dir->begin(); i != dir->end();) {
          int pos = std::string::npos;
        if (SageInterface::is_C_language () ||  SageInterface::is_Cxx_language  ()) {
          pos= (*i)->getString().find ("#pragma MAQAO");
        } else {
          pos= (*i)->getString().find ("!DIR$ MAQAO");
        }  
        if (pos != std::string::npos) {
          directives.push_back((*i)->getString());
        }
      }   
    }
  } else { // else if (currentFileType == "c" || currentFileType == "cpp") { //C/C++ file
    if (DEBUG) {
      std::cout << "loop line : "<< get_line_start() << std::endl;
      std::cout << "previous stmt : " << SageInterface::getPreviousStatement(loop)->class_name() << std::endl;
    }
    SgNode* previousNode = SageInterface::getPreviousStatement(loop);
    while (SgPragmaDeclaration* pragmaDecl = isSgPragmaDeclaration(previousNode)) {
      if (VERBOSE > 1 || DEBUG) std::cout << "pragma is : " << pragmaDecl->get_pragma()->get_pragma() << std::endl;
      directives.insert(directives.begin(),pragmaDecl->get_pragma()->get_pragma());
    }
  }
  return directives;
}

int Loop::get_vprof_ite () {
  if (DEBUG) DBG_MAQAO
  std::vector<std::string> directives = s2s_api::get_directives(loop);

  int ite_min =0; 
  int ite_mean=0;
  int ite_max =0;
  for (int i=0; i < directives.size(); i++) {
    std::cout << "directives["<<i<<"]="<<directives[i] << std::endl;
    if (directives[i].find("LOOP COUNT ") != std::string::npos) {        // FORTRAN
      ite_min = atoi(directives[i].substr(directives[i].find ("MIN=")+4,directives[i].find (",")-1).c_str());
      ite_max = atoi(directives[i].substr(directives[i].find ("MAX=")+4,directives[i].find (",")-1).c_str());
      ite_mean= atoi(directives[i].substr(directives[i].find ("AVG=")+4,directives[i].find (",")-1).c_str());   
    } else if (directives[i].find("loop_count ") != std::string::npos) { // C/C++
      ite_min = atoi(directives[i].substr(directives[i].find ("min=")+4,directives[i].find (",")-1).c_str());
      ite_max = atoi(directives[i].substr(directives[i].find ("max=")+4,directives[i].find (",")-1).c_str());
      ite_mean= atoi(directives[i].substr(directives[i].find ("avg=")+4,directives[i].find (",")-1).c_str());   
    } 
    if (ite_min == ite_max) {
      return ite_max;
    } 
  }
  return -1;
}

void Loop::printLines() {
  std::cout << "start " << loop->get_startOfConstruct ()->get_line() ;
  std::cout << " end " << loop->get_endOfConstruct ()->get_line() ;
  std::cout << std::endl;
}

std::string Loop::loopType_toString(){
  switch(typeOfLoop) {
    case DO :      return "DO";
    case WHILE :   return "WHILE";
    case DOWHILE : return "DOWHILE";
    case FOR :     return "FOR";
    default :      return "NOTHING";
  } 
}

void Loop::add_directive(std::string directive) {
  if(DEBUG) DBG_MAQAO
  s2s_api::add_directive(loop, directive);
}

void Loop::apply_directive() {
  if(DEBUG) DBG_MAQAO
  if (alreadyTransformed()) return;

  if (currentFileType == "fortran") {
    std::vector<std::string> maqaoDir = s2s_api::get_MAQAO_directives(loop);

    for (int i=0; i < maqaoDir.size(); i++) {

      int error = 0;
      std::string directive = maqaoDir[i];
      if (VERBOSE > 1 || DEBUG) { std::cout << "[fortran - nospe] directive found : " << directive << std::endl; }

      if (directive.find("IF_SPE_") != std::string::npos) {
            int pos_last_char = std::string::npos;

        pos_last_char = directive.find_last_of('=');
        if(pos_last_char == std::string::npos) 
          directive.find_last_of('_');
    
        std::string spe = directive.substr(directive.find ("IF_SPE_")+7,pos_last_char-(directive.find ("IF_SPE_")+7)).c_str();
        std::string func = s2s_api::get_function_RosePtr(loop)->get_declaration ()->get_name().getString() ;
    
        int index = func.find(spe);
        if(VERBOSE > 1 || DEBUG) 
          std::cout << "Check if we are in the right function, current function is : " 
                    << func 
                    << " , and the specialization is : "
                    << spe << std::endl;
    
        if (index != std::string::npos) {
          if(DEBUG) std::cout << "Yeah, we are in the right function" << std::endl;
          int idx =  directive.find("IF_SPE_");
          directive.erase(idx, 7+spe.length());
        } else {
          if(DEBUG) std::cout << "We are not in the right function" << std::endl;
          continue;
        }
    
      } else if (directive.find("MAQAO TAG LOOP") != std::string::npos){
        if(DEBUG) std::cout << "We found a MAQAO TAG LOOP" << std::endl;
        return;
      } else if (directive.find("MAQAO LABEL LOOP") != std::string::npos){
        if(DEBUG) std::cout << "We found a MAQAO LABEL LOOP" << std::endl;
        return;
      } else { // Si ce n'est pas une directive pour une fonction spécialisé 
        // alors ne pas appliquer cette directive dans les fonctions générées par ASSIST 
        // (car ce sont des fonctions spécialisées)
        
        // Pour les fonctions spé
        SgFunctionDefinition* func = SageInterface::getEnclosingFunctionDefinition(loop, false);
        if (func->get_file_info()->isTransformation()) {
          continue;
        }
      }
      s2s_api::remove_MAQAO_directives(loop, directive);
      apply_directive(directive);
      if (loop) s2s_api::remove_MAQAO_directives(loop);
      return;
    }
  } else { //C/C++, all have to be done in get_MAQAO_directives

    if (VERBOSE) {
      std::cout << "loop line : "<< get_line_start() << std::endl;
      std::cout << "previous stmt : " << SageInterface::getPreviousStatement(loop)->class_name() << std::endl;
      std::cout << "Comment above the loop : " << std::endl;
      std::vector<std::string> comment_list = s2s_api::get_comment_and_directives(loop);
      for (int i=0; i < comment_list.size(); i++) {
        std::cout << " " << comment_list[i] << std::endl;
      }
      std::cout << "getAttachedPreprocessingInfo : " << std::endl;
      AttachedPreprocessingInfoType *comments = loop->getAttachedPreprocessingInfo ();

        AttachedPreprocessingInfoType::iterator i;
        if (comments)
        std::cout << "size = "<< comments->size() << std::endl;
      else 
        std::cout << "coments empty " << std::endl;
        for (i = comments->begin (); i != comments->end (); i++)
        {
            std::cout << "directive : " << (*i)->getString ().c_str () << std::endl;
        }
    }

    SgPragmaDeclaration* dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(loop));    

    while (dir) {
      int pos = dir->get_pragma()->get_pragma().find ("MAQAO");
      if ( pos != std::string::npos 
        && dir->get_pragma()->get_pragma().find("MAQAO ANALYZE") == std::string::npos) 
      {
        std::string directive = dir->get_pragma()->get_pragma().substr(pos);

        if (VERBOSE > 1 || DEBUG) { std::cout << "[c - nospe] directive found : " << directive << std::endl; }

        if (directive.find("IF_SPE_") != std::string::npos) {
          // do nothing in this case, we are not in a specialized case
          dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
          continue;
        } else {
          SgPragmaDeclaration* dir2 = dir;
          dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
          SageInterface::removeStatement (dir2, true);
          apply_directive(directive);
        }
        return;
      } else {
        dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
        continue;
      }
    }
  }
}

void Loop::apply_directive(std::vector<variable_spe*> varspe) {
  if(DEBUG) DBG_MAQAO
  if (alreadyTransformed()) return;
  if (currentFileType == "fortran") {
    AttachedPreprocessingInfoType* dir = loop->getAttachedPreprocessingInfo();
    if(dir) {
      AttachedPreprocessingInfoType::iterator it;
      int error = 0;
      // for (int it=0; it < dir->size(); it++) {
      for (it = dir->begin(); it != dir->end();) {
        if (!(*it)) break;

        int pos = (*it)->getString().find ("!DIR$ MAQAO");
        if(pos != std::string::npos) {
          std::string directive = (*it)->getString().substr(pos);

          if (DEBUG) { std::cout << "[fortran - varspe] directive found : " << directive << std::endl; }

          if (directive.find("IF_SPE_") != std::string::npos) {
            int pos_last_char = directive.find_last_of('=');
            if(pos_last_char == std::string::npos) directive.find_last_of('_');

            std::string spe = directive.substr(directive.find ("IF_SPE_")+7,pos_last_char-(directive.find ("IF_SPE_")+7)).c_str();
            std::string func = s2s_api::get_function_RosePtr(loop)->get_declaration ()->get_name().getString() ;

            int index = func.find(spe);
            if (index != std::string::npos) {
              int idx =  directive.find("IF_SPE_");
              directive.erase(idx, 7+spe.length()+1);
            }
            else {
              // Découpage de la directive pour récupérer les données
              bool right_specialization = false;

              int postruc = s2s_api::find_first_number(spe.substr(0,spe.find_first_of('_')-1).c_str());

              std::string varname = spe.substr(0, postruc-1).c_str();
              std::string compareType = spe.substr(spe.find (varname)+varname.size()-1, 1).c_str();
              std::string varvalue = spe.substr(postruc).c_str();

              for (int i= 0; i < varspe.size(); i++) {
                if (varspe[i]->var_name == varname) {
                  if (varspe[i]->specialization == variable_spe::INTERVAL 
                      && compareType == "b") 
                  {
                    std::string inf_val = varvalue.substr(varvalue.find_last_of("_")+1) ;
                    std::string sup_val = varvalue.substr(0,varvalue.find_last_of("_"));

                    if (varspe[i]->sup_bound == atoi(sup_val.c_str()) && varspe[i]->inf_bound == atoi(varvalue.c_str())) {
                      right_specialization = true;

                    }
                  }
                  else if (varspe[i]->specialization == variable_spe::EQUALS 
                          && compareType == "e") 
                  {
                    if (varspe[i]->value == atoi(varvalue.c_str())) {
                      right_specialization = true;
                    }
                  }
                  else if (varspe[i]->specialization == variable_spe::INF 
                          && compareType == "i") 
                  {
                    if (varspe[i]->sup_bound == atoi(varvalue.c_str())) {
                      right_specialization = true;

                    }
                  }
                  else if (varspe[i]->specialization == variable_spe::SUP 
                          && compareType == "s") 
                  {
                    if (varspe[i]->inf_bound == atoi(varvalue.c_str())) {
                      right_specialization = true;

                    }
                  }
                }
              }

              if (right_specialization) {
                int idx = directive.find("IF_SPE_");
                directive.erase(idx, 7+varname.size()+1+varvalue.size()+1);
              } else if (*it) {
                it++;
                // it = dir->erase(it);
                continue;

              } else {
                continue;
              }
            } // end if index!= func.find(spe) ... else ...
          } else {
            // This function 
            if (*it) {
              it++;
              // it = dir->erase(it);
            }
            continue;
          }
          if (*it) {
            it++;
            // it = dir->erase(it); 
          }
          apply_directive(directive, &varspe);
          // return;
          
        } else {
          it++;
        }
      }
    }
  } else { //C/C++
    if (DEBUG) {
      std::cout << "loop line : "<< get_line_start() << std::endl;
      std::cout << "previous stmt : " << SageInterface::getPreviousStatement(loop)->class_name() << std::endl;
    }

    SgPragmaDeclaration* dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(loop));
    while (dir) {
      int pos = dir->get_pragma()->get_pragma().find ("MAQAO");
      if(pos != std::string::npos) {
        std::string directive = dir->get_pragma()->get_pragma().substr(pos);
        if (VERBOSE > 1 || DEBUG) { std::cout << "[c - varspe] directive found : " << directive << std::endl; }

        if (directive.find("IF_SPE_") != std::string::npos) {
          //TODO Check if information in varspe corresponding to the directive
          std::string spe = directive.substr(directive.find ("IF_SPE_")+7,directive.find('_')-(directive.find("IF_SPE_")+7)).c_str();
          std::string func = s2s_api::get_function_RosePtr(loop)->get_declaration ()->get_name().getString() ;

          int index = func.find(spe);
          if (index != std::string::npos) {
            int idx =  directive.find("IF_SPE_");
            directive.erase(idx, 7+spe.length());
          }
          else {
            // Découpage de la directive pour récupérer les données
            bool right_specialization = false;

            int postruc = s2s_api::find_first_number(spe.substr(0,spe.find_first_of('_')-1).c_str());

            if (postruc != -1) {
              std::string varname = spe.substr(0, postruc-1).c_str();
              std::string compareType = spe.substr(spe.find (varname)+varname.size()-1, 1).c_str();
              std::string varvalue = spe.substr(postruc).c_str();

              for (int i= 0; i < varspe.size(); i++) {
                  if (varspe[i]->var_name == varname) {
                    if (varspe[i]->specialization == variable_spe::INTERVAL 
                        && compareType == "b") 
                    {
                      std::string inf_val = varvalue.substr(varvalue.find_last_of("_")+1) ;
                      std::string sup_val = varvalue.substr(0,varvalue.find_last_of("_"));

                      if (varspe[i]->sup_bound == atoi(sup_val.c_str()) && varspe[i]->inf_bound == atoi(varvalue.c_str())) {
                        right_specialization = true;

                      }
                    }
                    else if (varspe[i]->specialization == variable_spe::EQUALS 
                            && compareType == "e") 
                    {
                      if (varspe[i]->value == atoi(varvalue.c_str())) {
                        right_specialization = true;
                      }
                    }
                    else if (varspe[i]->specialization == variable_spe::INF 
                            && compareType == "i") 
                    {
                      if (varspe[i]->sup_bound == atoi(varvalue.c_str())) {
                        right_specialization = true;

                      }
                    }
                    else if (varspe[i]->specialization == variable_spe::SUP 
                            && compareType == "s") 
                    {
                      if (varspe[i]->inf_bound == atoi(varvalue.c_str())) {
                        right_specialization = true;

                      }
                    }
                  }
              }
            } else if (directive.find(' ')  != std::string::npos) {

              std::string varnames = spe.substr(0, spe.find (' ')).c_str();
              std::vector<std::string> varname = s2s_api::split(varnames, " ");
              // Compare names in the varname list to the current var names to know if it's the right transformation to do or not
              for (int iVarName = 0; iVarName < varname.size(); iVarName++) {
                right_specialization = false;

                for (int iVarSpe= 0; iVarSpe < varspe.size(); iVarSpe++) {
                  if (varspe[iVarSpe]->var_name == varname[iVarName]) {
                    right_specialization = true;
                  }
                }
                if (!right_specialization) {
                  //On of the var name wasn't found so it's not the right specialization
                  break;
                }
              }
            }

            if (right_specialization) {
              int idx = directive.find("IF_SPE_");
              directive.erase(idx, 7+directive.find(' '));
            } else {
              SgPragmaDeclaration* dir2 = dir;
              dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
              // SageInterface::removeStatement (dir2, true);
              continue;
            }
          }
        } else {  
          SgPragmaDeclaration* dir2 = dir;
          dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
          SageInterface::removeStatement (dir2, true);
          continue;
        }
       
        SgPragmaDeclaration* dir2 = dir;
        dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
        SageInterface::removeStatement (dir2, true);

        apply_directive(directive, &varspe);

        return;
      } else {
        dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
        continue;
      }
    }
  }
}

bool Loop::apply_directive(std::string directive, std::vector<variable_spe*> *varspe) {
  if (DEBUG) DBG_MAQAO 
  if (alreadyTransformed()) return false;
  if (DEBUG) std::cout << "The directive to handle is : " << directive << std::endl;
  
  int error = 0;
  if (directive.find("FULLUNROLL")      != std::string::npos) {
    int unrollSize=0;
    SgStatement* parent = isSgStatement(loop->get_parent());

    if (directive.find ('=') != std::string::npos) {
      unrollSize = atoi(directive.substr(directive.find ('=')+1,directive.find (' ')-1).c_str());
    }

    // Erase all directive tagged as MAQAO directives
    s2s_api::remove_MAQAO_directives(loop);
    /*******************
    **   FULLUNROLL   **
    ********************/
    if (fullUnroll(unrollSize)) {
      // if (LOG) {s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" was full unrolled with success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
      if (LOG) {s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] full unroll - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
      return true;
    } else {
      // if (LOG) {s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" cannot be full unrolled.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
      if (LOG) {s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] full unroll - fail", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
      return false;
    }
  }
  else if (directive.find("UNROLL")     != std::string::npos) {
    std::string errorMSG = "Sorry we can't be able to determine the unroll size, thank you to provide information with the following directive : $DIR MAQAO UNROLL=<unrollSize>";
    SgStatement* parent = isSgStatement(loop->get_parent());
    
    if (directive.find ('=')  == std::string::npos) {
      if (VERBOSE) std::cout << errorMSG << std::endl;
      return false;
    }

    int unrollSize = atoi(directive.substr(directive.find ('=')+1,directive.find (' ')-1).c_str());
    bool reminderLoop = (directive.find("UNROLL") != std::string::npos) ? true : false;
    if (unroll(unrollSize,reminderLoop)) {
      // if (LOG) {s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" was unrolled with succes.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log");}
      if (LOG) {s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] unroll - success", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log");}
      return true;
    } else {
      // if (LOG) { s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" cannot be unrolled.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] unroll - fail", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;
    }
  }
  else if (directive.find("INTERCHANGE")!= std::string::npos) {
    size_t depth=2;
    size_t lexicoOrder=1;
    SgStatement* parent = isSgStatement(loop->get_parent());

    std::string errorMSG = "Sorry we can't be able to determine the depth, thank you to provide information with the following directive : $DIR MAQAO INTERCHANGE=<depth>(,<lexicoOrder>) ";
    
    if (directive.find ('=') != std::string::npos) {
      depth = atoi(directive.substr(directive.find ('=')+1,directive.find (',')-1).c_str());
      if (directive.find (',') != std::string::npos )
        lexicoOrder = atoi(directive.substr(directive.find (',')+1, directive.find (' ')-1).c_str());
    }

    // Erase all directive tagged as MAQAO directives
    s2s_api::remove_MAQAO_directives(loop);
    // Start the interchange transformation
    if (interchange(depth, lexicoOrder)) {
      // If successed
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] interchange - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }
      return true;
    } else {
      // If failed
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] interchange - fail.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;
    }
  }
  else if (directive.find("TILE_INNER") != std::string::npos) {
    size_t tileSize=0;
    SgStatement* parent = isSgStatement(loop->get_parent());
    std::string errorMSG = "Sorry we can't be able to determine the tile size, thank you to provide information with the following directive : $DIR MAQAO TILE_INNER=<tileSize> \nBy default the tile size is set to 8";
    if (directive.find ('=') != std::string::npos) {
      tileSize = atoi(directive.substr(directive.find ('=')+1,directive.find (' ')-1).c_str());
    } else {
      if (VERBOSE) std::cout << errorMSG << std::endl;
      tileSize = 8;
    }
    // Erase all directive tagged as MAQAO directives
    s2s_api::remove_MAQAO_directives(loop);
    if (tile_inner_v2(tileSize)) {
      // if (LOG) { s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe inner loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" was tilled with success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" strip mine - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return true;
    } else {
      //if (LOG) { s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe inner loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" cannot be tilled.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" strip mine - fail", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;
    }
  }
  else if (directive.find("TILE")       != std::string::npos) {
    size_t tileSize=0;
    SgStatement* parent = isSgStatement(loop->get_parent());
    std::string errorMSG = "Sorry we can't be able to determine the tile size, thank you to provide information with the following directive : $DIR MAQAO TILE=<tileSize> \nBy default the tile size is set to 8";
    if (directive.find ('=') != std::string::npos) {
      tileSize = atoi(directive.substr(directive.find ('=')+1,directive.find (' ')-1).c_str());
    } else {
      if (VERBOSE) std::cout << errorMSG << std::endl;
      // return;
      tileSize = 8;
    }

    // Erase all directive tagged as MAQAO directives
    s2s_api::remove_MAQAO_directives(loop);

    if (tile(tileSize)) {
      // if (LOG) { s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" was tilled with success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] tilling - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return true;
    } else {
      // if (LOG) { s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" cannot be tilled.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] tilling - fail.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;
    }
  }
  else if (directive.find("STRIP_MINIG")!= std::string::npos) {
    size_t tileSize=0;
    SgStatement* parent = isSgStatement(loop->get_parent());
    std::string errorMSG = "Sorry we can't be able to determine variables to tile the loop, thank you to provide information with the following directive : $DIR MAQAO TILE=<tileSize> ";
    if (directive.find ('=') != std::string::npos) {
      tileSize = atoi(directive.substr(directive.find ('=')+1,directive.find (' ')-1).c_str());
    } else {
      if (VERBOSE) std::cout << errorMSG << std::endl;
      return false;
    }
    // Erase all directive tagged as MAQAO directives
    s2s_api::remove_MAQAO_directives(loop);
    if (strip_mining(tileSize)) {
      // if (LOG) { s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" was strip mined with success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] strip mine - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return true;
    } else {
      // if (LOG) { s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" cannot be strip mined.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] strip mine - fail", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;
    }
  }
  else if (directive.find("SHORTVEC")   != std::string::npos) {
    std::string errorMSG = "";
    //Erase the directive before to transform the loop !
    bool transfo = false;
    SgStatement* parent = isSgStatement(loop->get_parent());
    // Erase all directive tagged as MAQAO directives
    s2s_api::remove_MAQAO_directives(loop);

    //TODO lire la directive et récuperer '%'<valeur> 
    //puis apeler la fonction shortVectoGen(<valeur>)
    if (int pos = directive.find('%') != std::string::npos) {
      std::string valStr = ""; 
      std::cout << "pos = " << pos << std::endl;
      // hum hum, sorry I don't remember why ... :/
      //if (pos < 11)
      //  pos=19;

      while (pos < directive.size()) {
        if(directive[pos] <= '9' && directive[pos] >= '0') {
          valStr += directive[pos]; 
        }
        pos++;
      }
      std::cout << "valstr found is : " << valStr << std::endl;
      transfo = shortVectoGen(atoi(valStr.c_str()));
    }
    else if ( directive.find("AVX2") != std::string::npos ) {
      transfo = shortVecto(true);
    } else {
      transfo = shortVecto();
    }

    if (transfo) {
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] Block vecto - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
       return true;
   } else {
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] Block vecto - fail"    , "log_"+s2s_api::getEnclosingsourceFile_RosePtr(parent)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;    
    }
  } 
  else if (size_t pos = directive.find("SPECIALIZE") != std::string::npos) {
    size_t posStart = directive.find("(", pos);
    size_t posEnd  = directive.find(")");
    std::vector<variable_spe*> variables_list;

    if ( posStart  != std::string::npos && posEnd != std::string::npos) {
      std::string substring = directive.substr(posStart+1, posEnd); 
      int i = 0; 
      
      if (varspe) {
        for(int i=0; i < varspe->size(); i++) {
          variables_list.push_back((*varspe)[i]);
        }
      }

      // Check all information about the specialization
      do {
        variable_spe* varspe_local = new variable_spe();

        //Avoid space at the begining
        std::string value = "";
        std::string variable = "";
        while (substring[i] == ' ' || substring[i] == ',') ++i;
        //search the name of the variable
        for( ; i < substring.length() 
          && substring[i] != ' ' 
          && substring[i] != '=' 
          && substring[i] != '<' 
          && substring[i] != '>' 
          && substring[i] != '{'; ++i) {
          variable += substring[i];
        }
        varspe_local->var_name = strdup(variable.c_str());

        //Avoid space and equals
        while (substring[i] == ' ') ++i;

        //Determine while style of the specialization
        if (substring[i] == '=') {
          //Avoid space and equals
          while (substring[i] == ' ') ++i;
          if (substring[i+1] == '{')  {
            i++;
            varspe_local->specialization = variable_spe::INTERVAL;
          } else {
            varspe_local->specialization = variable_spe::EQUALS;
          }
        }
        else if (substring[i] == '<') {
          varspe_local->specialization = variable_spe::INF;
        }
        else if (substring[i] == '>') {
          varspe_local->specialization = variable_spe::SUP;
        }
        else if  (substring[i] == '{')  {
          varspe_local->specialization = variable_spe::INTERVAL;
        }
        else {
          if (VERBOSE) 
            std::cout << " The directive \"" << directive 
                    << "\" isn't well wrote\nThe transformation will not be applied on the next statement. " 
                    << std::endl;
          error = 1;
          break;
        }

        i++;
        while (substring[i] == ' ') ++i;

        for(i ; 
          i < substring.length() 
          && substring[i] != ' ' 
          && substring[i] != ',' 
          && substring[i] != ')' 
          && substring[i] != '}'; 
          ++i) 
        {
          value += substring[i];
        }

        if (varspe_local->specialization == variable_spe::EQUALS) {
          varspe_local->value = atoi(value.c_str());
        } else if (varspe_local->specialization == variable_spe::INF) {
          varspe_local->sup_bound = atoi(value.c_str());
        } else if (varspe_local->specialization == variable_spe::SUP) {
          varspe_local->inf_bound = atoi(value.c_str());
        } else if (varspe_local->specialization == variable_spe::INTERVAL) {
          varspe_local->inf_bound = atoi(value.c_str())-1;
          value = "";

          while (substring[i] == ' ' || substring[i] == ',' || substring[i] == '}') ++i;

          for( ; i < substring.length() && substring[i] != ' ' && substring[i] != '}'; ++i) {
            value += substring[i];
          }
          varspe_local->sup_bound = atoi(value.c_str())+1;
        }

        while (substring[i] == ' '|| substring[i] == '}') ++i;
        //varspe_local->pretty_print();
        variables_list.push_back(varspe_local);
      } while (substring[i++] ==  ',');
            
      // if no error was detected -> specialize
      if (!error) {

        // It will specialize as time as necessary recursively. We need to do that because the loop move 
        // For move we need to copy and remove; so the current pointer does not exist after specialization
        // TODO : be clever and do better
        if (specialize(variables_list))  {
          if (LOG) {
            // s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" was specialized", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
            s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] specialization - success", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
          }
          return true;
        } else {
          if (LOG) {
            // s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+"\nThe loop line "+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+" cannot be specialized", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
            s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] specialization - fail", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
          }
          return false;
        }
      }
    } else {
      if (VERBOSE) std::cout << "The directive \"" << directive << "\" isn't well wrote\nThe transformation will not be applied on the next statement. " << std::endl;
    }
  }
  else if (size_t pos = directive.find("REPLACEBY")  != std::string::npos) {
    std::string textForReplacing = "" ;
    std::string errorMSG = "Sorry we had a problem during the \"REPLACEBY\" transformation, thank you to verify your directive, it have to look like : $DIR MAQAO REPLACEBY={<TEXT>}} ";
    
    if (directive.find ('{') != std::string::npos) {
      textForReplacing = directive.substr(directive.find ('{')+1, (directive.find_last_of ('}'))- (directive.find ('{')+1) );

      // Erase all directive tagged as MAQAO directives
      s2s_api::remove_MAQAO_directives(loop);
      replaceByText(textForReplacing);

      if (LOG) {
        s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+" l:"+s2s_api::itoa(get_line_start())+"] replacement by "+textForReplacing+" - success", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      }
      return true;
    } else {
      if (VERBOSE) std::cout << errorMSG << std::endl;
      if (LOG) {
        s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+" l:"+s2s_api::itoa(get_line_start())+"] replacement by "+textForReplacing+ " -fail"   , "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      }
      return false;
    }
  }
  else {
    if (VERBOSE) std::cout << "Directive:" << directive << " is unknown !" << std::endl;
    return false;
  }
}

bool Loop::hasFixBounds() {
  if (SgFortranDo * fortranloop = isSgFortranDo(loop)) {
    if ( isSgIntVal(isSgBinaryOp(fortranloop->get_initialization())->get_rhs_operand())
      && isSgIntVal((fortranloop->get_bound()))) {
        return true;
    }
  } else if (SgForStatement* forloop = isSgForStatement(loop)) {

    std::cout << "C \"for\" loops are not handle yet" << std::endl;
  }
  return false;
}

void Loop::print() {
  if(DEBUG) DBG_MAQAO
  std::cout << " id : " << id << " | "
            << " type  : " << loopType_toString()  << " | "
            << " start " << loop->get_startOfConstruct ()->get_line()  << " | "
            << " end " << loop->get_endOfConstruct ()->get_line() 
            << std::endl;

  if (DEBUG) {
    AttachedPreprocessingInfoType* dir = loop->getAttachedPreprocessingInfo();
    AttachedPreprocessingInfoType::reverse_iterator i;
    if(dir) {
      for (i = dir->rbegin(); i != dir->rend(); i++) {
        if((*i)->getString().find ('!DIR$') != std::string::npos) {
          std::cout << "type of directive : " << (*i)->getTypeOfDirective() << " | ";
          std::cout << (*i)->getString().c_str() << "\n ";
        }
      }
    }
    std::cout << loop->unparseToString();

  }
}

bool Loop::unroll(size_t unrollSize, bool reminder/*=false*/) {
  if (DEBUG) DBG_MAQAO
  if (LOG){ s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+"\nUnroll Transformation", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }

  if (isSgForStatement(loop)) {
    forLoopUnroll(isSgForStatement(loop), 0, unrollSize);
  }
  else if (SgFortranDo * fdo = isSgFortranDo(loop)) {
    SgNode * parent = fdo->get_parent();
    SgFortranDo * loopCopy     = isSgFortranDo(SageInterface::copyStatement(fdo));
    SgFortranDo * reminderLoop = isSgFortranDo(SageInterface::copyStatement(fdo));

    s2s_api::remove_MAQAO_directives(reminderLoop);
    loopCopy->set_parent(parent);
    
    loopCopy = loopUnroll(loopCopy, unrollSize);
    if(!loopCopy)  {
      s2s_api::log("In "+ s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+ "\nLoop l." + get_line_start_str() + " was not unrolled ", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
    } else if (reminder) {
      //Modify the initialization
      isSgStatement(parent)->replace_statement(fdo,loopCopy);
      if (SgBinaryOp * rl_init = isSgBinaryOp(reminderLoop->get_initialization ())) {
        SgExpression* new_rl_init = SageInterface::copyExpression(loopCopy->get_bound ());
        rl_init->set_rhs_operand(new_rl_init);
        new_rl_init->set_parent(rl_init);

        //Build if condition
        //Build the modulo
        SgModOp* modOp = SageBuilder::buildModOp(
          fdo->get_bound (),
          SageBuilder::buildIntVal(unrollSize));
        modOp->set_need_paren(true);
        //Modulo % unrollSize == 0
        SgBinaryOp* ifcond = SageBuilder::buildNotEqualOp(modOp, SageBuilder::buildIntVal(0));
        //Build the If stmt
        SgIfStmt* ifstmt = SageBuilder::buildIfStmt(SageBuilder::buildExprStatement (ifcond),reminderLoop, NULL);

        //Set parent
        modOp->set_parent(ifcond);
        ifcond->set_parent(ifstmt);
        ifstmt->set_parent(isSgStatement(parent));
        reminderLoop->set_parent(ifstmt);
        //Insert the if statment
        SageInterface::insertStatementAfter(loopCopy, ifstmt, false);
      }        
      loop = fdo;
    } else {
      isSgStatement(parent)->replace_statement(fdo,loopCopy);

    }
  }
  hasBeenTransformed = true;
  return true;
}

bool Loop::fullUnroll(size_t unrollSize) {
  if (DEBUG) DBG_MAQAO
  if (LOG) s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+"\nFull unroll Transformation ", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
  std::string errorMSG = "Sorry we can't be able to determine the loop bound value, thank you to provide information on the following directive : MAQAO FULLUNROLL=<val> ";

  if (typeOfLoop == DO) {
    if (!isSgFortranDo(loop)->get_has_end_statement ()) {
      if (LOG) s2s_api::log("The loop format is not supported yet.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      std::cout << "this format of loop is not suported yet. " << loop->unparseToString() << std::endl;
      return false;
    }
  }

  if (SgFortranDo * fortranloop = isSgFortranDo(loop)) {
    /********************
    **   UNROLL SIZE   **
    *********************/
    if (unrollSize == 0) {
      if (SgIntVal* bound_val = isSgIntVal(fortranloop->get_bound())) {
        unrollSize = bound_val->get_value();
      } else if (SgVarRefExp* vre = isSgVarRefExp(fortranloop->get_bound())) {
        if (SgIntVal* bound_val = s2s_api::trace_back_to_last_affectation(vre)) {
          unrollSize = bound_val->get_value();
        } else {
          if (VERBOSE) std::cout << errorMSG << std::endl;
          return false;
        }
      } else {
        if (VERBOSE) std::cout << errorMSG << std::endl;
        return false;
      }
    }

    if (!isSgBinaryOp(fortranloop->get_initialization())) return false;
    /***********************
    **   INITIALIZATION   **
    ************************/
    SgBasicBlock * original_body = isSgBasicBlock(SageInterface::copyStatement(fortranloop->get_body()));
    SgBasicBlock * newBody = SageBuilder::buildBasicBlock();
    SgVariableSymbol * var = isSgVarRefExp(isSgBinaryOp(fortranloop->get_initialization())->get_lhs_operand())->get_symbol();
    int start = 0;

    newBody->set_parent(fortranloop->get_parent());
    /*******************
    **   BOUNDS VAL   **
    ********************/
    if (SgIntVal* intvalue = isSgIntVal(isSgBinaryOp(fortranloop->get_initialization())->get_rhs_operand())) {
      start = intvalue->get_value();
        /*******************
        **   FULLUNROLL   **
        ********************/
      for(int i=start; i < start+unrollSize; ++i) {
        SgBasicBlock * body_copy = isSgBasicBlock(SageInterface::copyStatement(original_body));
        s2s_api::constantPropagation (body_copy, var, i);
        newBody->append_statement(body_copy);
      }

    } else { // The init is not just an integer
      for(int i=0; i < unrollSize; ++i) {
        SgBasicBlock * body_copy = isSgBasicBlock(SageInterface::copyStatement(original_body));
        s2s_api::replace_expr (body_copy, var, SageBuilder::buildAddOp(isSgBinaryOp(isSgFortranDo(loop)->get_initialization())->get_rhs_operand(),SageBuilder::buildIntVal(i)));
        newBody->append_statement(body_copy);
      }
    }
    
    /****************
    **   REPLACE   **
    *****************/
    // SageInterface::replaceStatement(fortranloop, SageInterface::copyStatement(fortranloop->get_body()));
    SageInterface::replaceStatement(fortranloop, SageInterface::copyStatement(newBody));
    
    hasBeenTransformed = true;    
    return true;
  } else if (SgForStatement* forloop = isSgForStatement(loop)) {
    /***********************
    **   INITIALIZATION   **
    ************************/
    SgBasicBlock * original_body = isSgBasicBlock(SageInterface::copyStatement(forloop->get_loop_body ()));
    SgBasicBlock * newBody = SageBuilder::buildBasicBlock();
    SgVariableSymbol * var;
    SgBinaryOp* init_binop;

    /***********************************
    **   ITERATOR VAR & wrong cases   **
    ************************************/
    if (SgUnaryOp* uop = isSgUnaryOp(forloop->get_increment ())) {
      if (!isSgPlusPlusOp(uop)) {
        if (LOG) s2s_api::log("Cannot transform this loop due to its incrementation (it must be i++ or ++i) .", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
      }
    }
    if (forloop->get_for_init_stmt()->get_init_stmt ().size() > 1) {
      if (LOG) s2s_api::log("Cannot transform this loop due to its a non cannonical initialization.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
    } else {
      if ((init_binop = isSgBinaryOp(forloop->get_for_init_stmt()->get_init_stmt()[0]))) { }
      else if (isSgExprStatement(forloop->get_for_init_stmt()->get_init_stmt()[0])) {
        if (init_binop = isSgBinaryOp(isSgExprStatement(forloop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())) { }
        else {
          if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical initialization.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
          return false;
        }
      }
      var = isSgVarRefExp(init_binop->get_lhs_operand())->get_symbol();
    }

    int start = 0;
    newBody->set_parent(forloop->get_parent());

    /*******************
    **   BOUNDS VAL   **
    ********************/
    /*** UNROLL SIZE ***/
    if (unrollSize == 0) { // if the unroll size have not been precise by the user
      if (SgExprStatement* loopCond = isSgExprStatement(forloop->get_test())) { // if its not something strange
        if (SgBinaryOp* binop = isSgBinaryOp(loopCond->get_expression())) { // If the condition is well a binary op
          if (s2s_api::isinExpr(binop->get_lhs_operand(), var) && !isSgBinaryOp(binop->get_lhs_operand())) { // if we have not more complexe op that : i op <val> 
            if (isSgLessThanOp(binop)) { // if its i < <val>
              if (SgIntVal* bound_val = isSgIntVal(binop->get_rhs_operand())) { // if <val> is a constant
                unrollSize = bound_val->get_value();
              } else if (SgVarRefExp* vre = isSgVarRefExp(binop->get_rhs_operand())) { // if <val> is a determinist variable
                if (SgIntVal* bound_val = s2s_api::trace_back_to_last_affectation(vre)) { // try to determine the val of the var
                  unrollSize = bound_val->get_value();
                } else {
                  if (VERBOSE) std::cout << errorMSG << std::endl;
                  return false;
                }
              } else {
                if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical condition.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
                if (VERBOSE) std::cout << errorMSG << std::endl;
                return false;
              }
            } else if (isSgLessOrEqualOp(binop)) {
              if (SgIntVal* bound_val = isSgIntVal(binop->get_rhs_operand())) { // if <val> is a constant
                unrollSize = bound_val->get_value() +1;
              } else if (SgVarRefExp* vre = isSgVarRefExp(binop->get_rhs_operand())) { // if <val> is a determinist variable
                if (SgIntVal* bound_val = s2s_api::trace_back_to_last_affectation(vre)) { // try to determine the val of the var
                  unrollSize = bound_val->get_value() +1;
                } else {
                  if (VERBOSE) std::cout << errorMSG << std::endl;
                  return false;
                }
              } else {
                if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical condition.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
                if (VERBOSE) std::cout << errorMSG << std::endl;
                return false;
              }
            } else {
              if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical condition.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
              if (VERBOSE) std::cout << errorMSG << std::endl;
              return false;
            }
          } else if (s2s_api::isinExpr(binop->get_rhs_operand(), var) && !isSgBinaryOp(binop->get_rhs_operand())) {
            if (isSgGreaterThanOp(binop)) { // if its i < <val>
              if (SgIntVal* bound_val = isSgIntVal(binop->get_lhs_operand())) { // if <val> is a constant
                unrollSize = bound_val->get_value();
              } else if (SgVarRefExp* vre = isSgVarRefExp(binop->get_lhs_operand())) { // if <val> is a determinist variable
                if (SgIntVal* bound_val = s2s_api::trace_back_to_last_affectation(vre)) { // try to determine the val of the var
                  unrollSize = bound_val->get_value();
                } else {
                  if (VERBOSE) std::cout << errorMSG << std::endl;
                  return false;
                }
              } else {
                if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical condition.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
                if (VERBOSE) std::cout << errorMSG << std::endl;
                return false;
              }
            } else if (isSgGreaterOrEqualOp(binop)) {
              if (SgIntVal* bound_val = isSgIntVal(binop->get_lhs_operand())) { // if <val> is a constant
                unrollSize = bound_val->get_value() +1;
              } else if (SgVarRefExp* vre = isSgVarRefExp(binop->get_lhs_operand())) { // if <val> is a determinist variable
                if (SgIntVal* bound_val = s2s_api::trace_back_to_last_affectation(vre)) { // try to determine the val of the var
                  unrollSize = bound_val->get_value() +1;
                } else {
                  if (VERBOSE) std::cout << errorMSG << std::endl;
                  return false;
                }
              } else {
                if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical condition.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
                if (VERBOSE) std::cout << errorMSG << std::endl;
                return false;
              }
            } else {
              if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical condition.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
              if (VERBOSE) std::cout << errorMSG << std::endl;
              return false;
            }
          } else {
            if (LOG) s2s_api::log("Cannot transform this loop due to its condition (iterator not found in the loop condition).", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
            if (VERBOSE) std::cout << errorMSG << std::endl;
            return false;
          }
        } else {
          if (VERBOSE) std::cout << errorMSG << std::endl;
          return false;
        }
      } else {
        if (VERBOSE) std::cout << errorMSG << std::endl;
        return false;
      }
    }

    /****** START ******/
    if (SgIntVal* intvalue = isSgIntVal(init_binop->get_rhs_operand())) {
      start = intvalue->get_value();
    } else {
      if (LOG) s2s_api::log("Cannot transform this loop due to its non cannonical initialization.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
    }

    /*******************
    **   FULLUNROLL   **
    ********************/
    if (VERBOSE) std::cout << "Unroll from " << start << "to " << unrollSize << std::endl;
    for(int i=start; i < start+unrollSize; ++i) {
      SgBasicBlock * body_copy = isSgBasicBlock(SageInterface::copyStatement(original_body));
      s2s_api::constantPropagation (body_copy, var, i);
      newBody->append_statement(body_copy);
    }

    if (handleStopKeyword(isSgBasicBlock(newBody))) {
      // SageInterface::replaceStatement(forloop, SageInterface::copyStatement(forloop->get_body()));
      SageInterface::replaceStatement(forloop, SageInterface::copyStatement(newBody));
      hasBeenTransformed = true;
      return true;
    } else {
      if (LOG) s2s_api::log("The loop was not unrolled due to a case with a \"continue\" keywords not handled yet.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
    }
  }
  if (LOG) s2s_api::log("The loop was not a \"for\"(C/C++ style) or a \"do\"(fortran style).", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
  return false;
}

//! Interchange/Permutate a n-level perfectly-nested loop rooted at 'loop' using a lexicographical order number within [0,depth!)
bool Loop::interchange(size_t depth, size_t lexicoOrder) {
  if(DEBUG) DBG_MAQAO
  // if (LOG) s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+"\nInterchange Transformation ", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");

  if (typeOfLoop == FOR) {
    if (SageInterface::loopInterchange(isSgForStatement(loop), depth, lexicoOrder)) {
      hasBeenTransformed = true;
      return true;
    } else {
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] interchange - failed", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;
    }
  } else if (typeOfLoop == DO) {
    if (lexicoOrder == 0) // allow 0 to mean no interchange at all
      return true;

    // parameter verification
    if (loop == NULL) {
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] interchange - failed - Loop does not exist.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }
      return false;

    }
    //must have at least two levels
    if (depth <= 1) {
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] interchange - failed - Depth of the loop to interchange is inferior to 2.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }    
      return false;
    }
    
    int result = 1;
    for (size_t i=2; i<=depth; i++) result*=i;

    if (lexicoOrder>=result) {return false;}

    // TODO need to verify the input loop has n perfectly-nested children loops inside
    // save the loop nest's headers: init, test, and increment
    std::vector<SgFortranDo* > loopNest = SageInterface::querySubTree<SgFortranDo>(loop,V_SgFortranDo);
    //ROSE_ASSERT(loopNest.size()>=depth);
    if (loopNest.size() < depth) {
      if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] interchange - failed - Depth of the loop to interchange is superior the the size of the nest.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }    
      return false;
    }

    //Check if they're any statement which prevent of currentFileType the interchange
    for (int i=0; i < loopNest.size() && i < depth-1; i++) {
      SgStatementPtrList bodyStmt = loopNest[i]->get_body()->get_statements();
      for(int j=0; j < bodyStmt.size(); j++) {
        if (SgExprStatement* exprstmt = isSgExprStatement(bodyStmt[j])) {
          SgVarRefExp * incrementVar = isSgVarRefExp(isSgBinaryOp(isSgFortranDo(loop)->get_initialization())->get_lhs_operand());
          if (s2s_api::isinExpr(exprstmt->get_expression(),incrementVar->get_symbol())) {
            //  if (LOG) s2s_api::log("In "+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+"\nThey are statement between loops which prevent of the interchange ", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
            if (LOG) s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] interchange - failed - They are statement between loops which prevent the interchange.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
            return false;
          }
        } else if (isSgScopeStatement(bodyStmt[j])) {
          break; // If they're a loop go up to the next loop to check.
        }  
      }
    }

    std::vector<std::vector<SgNode*> > loopHeads;
    for (std::vector<SgFortranDo* > ::iterator i = loopNest.begin(); i!= loopNest.end(); i++)
    {
      SgFortranDo* cur_loop = *i;
      std::vector<SgNode*> head;
      head.push_back(cur_loop->get_initialization());
      head.push_back(cur_loop->get_bound());
      head.push_back(cur_loop->get_increment());
      loopHeads.push_back(head);
    }

    // convert the lexicographical number to a permutation order array permutation[depth]
    std::vector<size_t> changedOrder = getPermutationOrder_s2s (depth, lexicoOrder);
    // rewrite the loop nest to reflect the permutation
    // set the header to the new header based on the permutation array
    for (size_t i=0; i<depth; i++)
    {
      // only rewrite if necessary
      if (i != changedOrder[i])
      {
        SgFortranDo* cur_loop = loopNest[i];
        std::vector<SgNode*> newhead = loopHeads[changedOrder[i]];

        SgExpression* init = isSgExpression(newhead[0]);
        ROSE_ASSERT(init != cur_loop->get_initialization ());
        cur_loop->set_initialization(init);
        if (init)
        {
          init->set_parent(cur_loop);
          SageInterface::setSourcePositionForTransformation(init);
        }

        SgExpression* test = isSgExpression(newhead[1]);
        cur_loop->set_bound(test);
        if (test)
        {
          test->set_parent(cur_loop);
          SageInterface::setSourcePositionForTransformation(test);
        }

        SgExpression* incr = isSgExpression(newhead[2]);
        cur_loop->set_increment(incr);
        if (incr)
        {
          incr->set_parent(cur_loop);
          SageInterface::setSourcePositionForTransformation(incr);
        }
      }
    }

    hasBeenTransformed = true;
    return true;
  } else {
    return false;
  }
}

bool Loop::strip_mining(size_t tileSize) {
  if(DEBUG) DBG_MAQAO

  if (typeOfLoop == FOR) {
    if (SageInterface::loopTiling(isSgForStatement(loop), 1, tileSize)) {
      hasBeenTransformed = true;
      return true;
    } else {
      return false;
    }
  } else if (typeOfLoop == DO) {
    if (tile_fortran_do(isSgFortranDo(loop), tileSize)) {
      hasBeenTransformed = true;
      return true;
    } else {
      return false;
    }
  }
  return false;
}

bool Loop::tile(size_t tileSize=8) {
  if(DEBUG) DBG_MAQAO

  if (typeOfLoop == FOR) {
    // Locate the target loop at level n
    std::vector<SgForStatement* > loops= SageInterface::querySubTree<SgForStatement>(loop,V_SgForStatement);
    for (int i=0; i < loops.size(); i++ ) {
      if (!SageInterface::loopTiling(isSgForStatement(loop), 1, tileSize)) {
        if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] tilling - fail.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }

        return false;
      }
    }
    return true;
  } else if (typeOfLoop == DO) {
    // Locate the target loop at level n
    std::vector<SgFortranDo* > loops= SageInterface::querySubTree<SgFortranDo>(loop,V_SgFortranDo);
    for (int i=0; i < loops.size(); i++ ) {
      if (!tile_fortran_do(isSgFortranDo(loops[i]), tileSize)) {
        if (LOG) { s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(get_line_start())+"-"+s2s_api::itoa(get_line_end())+"] tilling - fail.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log"); }

        return false;
      }
    }
    hasBeenTransformed = true;
    return true;
  }
  return false;
}

bool Loop::tile_inner_v2(size_t tileSize) {
  if(DEBUG) DBG_MAQAO
  // skip tiling if tiling size is 0 (no tiling), we allow 0 to get a reference value for the original code being tuned
  // 1 (no need to tile)
  if (tileSize<=1)
    return true;

  if (typeOfLoop == FOR) {
    std::vector<SgForStatement* > loops= SageInterface::querySubTree<SgForStatement>(loop,V_SgForStatement);
    if (SageInterface::loopTiling(isSgForStatement(loop), loops.size(), tileSize)) {
      hasBeenTransformed = true;
      return true;
    } else {
      return false;
    }
  } else if (typeOfLoop == DO) {
    // Locate the target loop at level n
    std::vector<SgFortranDo* > loops= SageInterface::querySubTree<SgFortranDo>(loop,V_SgFortranDo);
    //get the inner loop
    SgFortranDo* target_loop = isSgFortranDo(loops[loops.size()-1]); // adjust to numbering starting from 0

    // grab the target loop's essential header information
    SgInitializedName* ivar = NULL;
    SgExpression* lb = NULL;
    SgExpression* ub = NULL;
    SgExpression* step = NULL;

      //Check the init of ivar and lb
      SgExpression* init = target_loop->get_initialization();
      if(isSgAssignOp(init)) {
        if (isSgVarRefExp(isSgBinaryOp(init)->get_lhs_operand()) 
          && isSgIntVal(isSgBinaryOp(init)->get_rhs_operand()))
        {
          ivar = isSgVarRefExp(isSgBinaryOp(init)->get_lhs_operand())->get_symbol()->get_declaration();
          lb = isSgIntVal(isSgBinaryOp(init)->get_rhs_operand());
        } else { if(LOG) {std::cout << "WARNING: cannot tile the loop l."<<loop->get_startOfConstruct()->get_line()<< " because the loop wasn't canonical.\n";} return false; }
      } else { if(LOG){ std::cout << "WARNING: cannot tile the loop l."<<loop->get_startOfConstruct()->get_line()<< " because the loop wasn't canonical.\n";} return false; }
      //Check the up
      ub = target_loop->get_bound();
      //Check the stride
      step = target_loop->get_increment();

    if(!(ivar&& lb && ub && step)) {
      if(LOG){
        std::cout << "WARNING: cannot tile the loop l."
        << loop->get_startOfConstruct()->get_line()
        << " because the loop wasn't canonical.\n";
      } 
      return false;
    }
    ROSE_ASSERT(ivar != NULL);
    ROSE_ASSERT(lb   != NULL);
    ROSE_ASSERT(ub   != NULL);
    ROSE_ASSERT(step != NULL);
    
    // Add a controlling loop around the top loop nest
    // Ensure the parent can hold more than one children
    SgLocatedNode* parent = NULL; //SageInterface::ensureBasicBlockAsParent(loopNest)
    if (SageInterface::isBodyStatement(loop)) {// if it is a single body statement (Already a for statement, not a basic block)
      parent = SageInterface::makeSingleStatementBodyToBlock(loop);
    } else {
      parent = isSgLocatedNode(loop->get_parent());
    }
    ROSE_ASSERT(parent!= NULL);

    // Now we can prepend a controlling loop index variable: _lt_var_originalIndex
    std::string ivar2_name = "lt_var_"+ivar->get_name().getString();
    SgScopeStatement* scope = loop->get_scope();
    ROSE_ASSERT(scope != NULL);
    SgVariableDeclaration* loop_index_decl = SageBuilder::buildVariableDeclaration (ivar2_name, SageBuilder::buildIntType(),NULL, scope);
    // insert the variable with other declaration at the top of the function
    s2s_api::insert_declaration_into_function(target_loop, loop_index_decl);
     
    // init statement of the loop header, copy the lower bound
    SgExpression* init_stmt = SageBuilder::buildAssignOp(SageBuilder::buildVarRefExp(ivar2_name,scope), SageInterface::copyExpression(lb));
    ROSE_ASSERT(init_stmt != NULL);

    // build loop incremental  I
    // expression var+=up*tilesize or var-=upper * tilesize
    SgExpression* incr_exp = NULL;
    SgExpression* orig_incr_exp = target_loop->get_increment();
    ROSE_ASSERT(orig_incr_exp != NULL);

    if (isSgNullExpression(step))
      incr_exp = SageBuilder::buildIntVal(tileSize);
    else if (isSgIntVal(step)) {
      if (isSgIntVal(step)->get_value() == 1) {
      incr_exp = SageBuilder::buildIntVal(tileSize);
      }
    } else {
      incr_exp = SageBuilder::buildMultiplyOp(SageInterface::copyExpression(step), SageBuilder::buildIntVal(tileSize));
    }
    ROSE_ASSERT(incr_exp != NULL);

    // END CHECKING
    // ----------------

    // Control loop Creation
    SgFortranDo* control_loop = new SgFortranDo(init_stmt, ub, incr_exp, SageBuilder::buildBasicBlock());
    SgBinaryOp* cond = SageBuilder::buildAddOp(SageBuilder::buildVarRefExp(ivar2_name,scope), SageBuilder::buildSubtractOp(SageInterface::copyExpression(incr_exp),SageBuilder::buildIntVal(1)));
    isSgFortranDo(target_loop)->set_bound(cond);
    cond->set_parent(loop);
    // ----------------
    // Control bound and loop
      
    std::string ivar3_name = "lt_bound_"+isSgVarRefExp(ub)->get_symbol ()->get_name().getString();
    SgVariableDeclaration* loop_bound_decl = SageBuilder::buildVariableDeclaration (ivar3_name, SageBuilder::buildIntType(),NULL, scope);
    SgVariableDeclaration* control_loop_new_bound = SageBuilder::buildVariableDeclaration (ivar3_name, SageBuilder::buildIntType(),NULL, scope);
    control_loop->set_bound(SageBuilder::buildVarRefExp(ivar3_name, control_loop));
    s2s_api::insert_declaration_into_function(target_loop, loop_bound_decl);

    SgExpression * divide_control_bound = SageBuilder::buildDivideOp(ub,incr_exp);
    divide_control_bound->set_need_paren (true);
    SgExprStatement* bound_control_loop =  SageBuilder::buildAssignStatement( SageBuilder::buildVarRefExp(ivar3_name, scope), SageBuilder::buildMultiplyOp(divide_control_bound, control_loop->get_increment()) );
    SageInterface::insertStatementBefore(loop, bound_control_loop);

    // ----------------
    // ending if

    SgIfStmt * control_if = NULL;
    SgFortranDo* ending_loop = new SgFortranDo(init_stmt, ub, incr_exp, SageBuilder::buildBasicBlock());
    ending_loop->set_parent(loop->get_parent());

    SgFortranDo* copy_loop = isSgFortranDo(SageInterface::copyStatement(loop));

    control_if = SageBuilder::buildIfStmt(
      SageBuilder::buildLessThanOp(
        SageBuilder::buildVarRefExp(ivar3_name, control_if),
        SageBuilder::buildVarRefExp(isSgVarRefExp(ub)->get_symbol ()->get_name().getString(), control_if)),
      SageBuilder::buildBasicBlock(copy_loop),
      NULL);
    control_if->set_parent(loop->get_parent());
    SageInterface::insertStatementAfter(loop, control_if);


    std::vector<SgFortranDo* > copy_loops= SageInterface::querySubTree<SgFortranDo>(copy_loop,V_SgFortranDo);
    SgFortranDo* copy_target_loop = isSgFortranDo(copy_loops[copy_loops.size()-1]);
    copy_target_loop->set_initialization(SageBuilder::buildAssignOp(SageBuilder::buildVarRefExp(ivar->get_name().getString(), copy_target_loop) , SageBuilder::buildAddOp(control_loop->get_bound(),SageBuilder::buildIntVal(1))));
    copy_target_loop->set_bound(ub);
    copy_target_loop->set_has_end_statement (true);

    // ----------------

    control_loop->set_parent(loop->get_parent());

    SageInterface::insertStatementBefore(loop, control_loop);
    // move loopNest into the control loop
    SageInterface::removeStatement(loop);
    SageInterface::appendStatement(loop,isSgBasicBlock(control_loop->get_body()));
    loop->set_parent(control_loop);

    // rewrite the lower (i=lb), upper bounds (i<=/>= ub) of the target loop
    SgAssignOp* assign_op  = isSgAssignOp(init);

    ROSE_ASSERT(assign_op);
    assign_op->set_rhs_operand(SageBuilder::buildVarRefExp(ivar2_name,scope));
      
    SageInterface::setOneSourcePositionForTransformation (control_loop);
    control_loop->set_has_end_statement(true);

    ROSE_ASSERT(control_loop->get_file_info() != NULL);
    ROSE_ASSERT(loop->get_file_info() != NULL);
    ROSE_ASSERT(control_loop->get_bound() != NULL);
    ROSE_ASSERT(control_loop->get_increment() != NULL);

    hasBeenTransformed = true;
    return true;
  }
  return false;
}

bool Loop::shortVecto (bool useAVX) {
  if(DEBUG) DBG_MAQAO

  int nbOfIteration = 0, MAXBOUNDFORPOORVEC = 8, start=0 ;
  if (typeOfLoop == FOR) {
    SgForStatement * floop = isSgForStatement(loop);

    // If the bound is known and if it's  of type : x <(=) 8
    if (SgExprStatement* exprstmt = isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])) {
      if(SgBinaryOp* init_binop = isSgBinaryOp(exprstmt->get_expression()) ) {
        if ((!isSgIntVal(init_binop->get_rhs_operand()) && !isSgVarRefExp(init_binop->get_lhs_operand()))
            || floop->get_for_init_stmt()->get_init_stmt().size() > 1 
            || !(isSgLessThanOp(isSgExprStatement(floop->get_test())->get_expression()) 
               || isSgLessOrEqualOp(isSgExprStatement(floop->get_test())->get_expression()))) 
        {
          if (LOG) s2s_api::log("The loop is not cannonical.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
          return false;
        }
      }
    }

    nbOfIteration = isSgIntVal(isSgBinaryOp(isSgExprStatement(floop->get_test())->get_expression())->get_rhs_operand())->get_value();
    // if the condition is <= add one to the number of iterations
    if (isSgLessOrEqualOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())) {
      nbOfIteration++;
    }

    if(isSgIntVal(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_rhs_operand())) {
      start = isSgIntVal(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_rhs_operand())->get_value();
      nbOfIteration -= start;
    } else {
      if (LOG) s2s_api::log("The loop is not cannonical (initialization).", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
    }

    // If the bound respect some conditions
    if( ! (nbOfIteration < MAXBOUNDFORPOORVEC && nbOfIteration > 2 ) ) {
      if (LOG) {std::string msg = "Something wrong with the number of iteration, as reminder, the max number of iteration authorized for the bloc vectorization transformation is 7 and the minimum is 3."; s2s_api::log(msg, "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
      return false;
    }

    SgNode * parent = floop->get_parent();
    std::string directive ="simd";
    std::string directive_next ="vector unaligned";

    // if nbOfIteration == 6/7
    if ( nbOfIteration >= 6 ) {
      SgVarRefExp * incrementVar;
      incrementVar = isSgVarRefExp(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_lhs_operand());
      ROSE_ASSERT(incrementVar);
      // split de la boucle en 2 boucles :
      
      // l'une de 4 iterations
      SgStatement* newTest = SageBuilder::buildExprStatement(SageBuilder::buildLessOrEqualOp(incrementVar, SageBuilder::buildIntVal(start+4)));
      floop->set_test (newTest);
      newTest->set_parent(floop);
      // Ajout des directives sur les boucles créées
      s2s_api::add_directive( floop, directive );
      s2s_api::add_directive( floop, directive_next );

      // l'autre de 2
      SgForStatement * secondLoop = isSgForStatement(SageInterface::copyStatement(floop));
      //s2s_api::add_directive(secondLoop, directive);
      SgExprStatement* secondLoop_test = SageBuilder::buildExprStatement(SageBuilder::buildLessOrEqualOp(incrementVar, SageBuilder::buildIntVal(start+6)));
      secondLoop->set_test(secondLoop_test);
      secondLoop_test->set_parent(secondLoop);
      SgStatementPtrList listOfInit;
      SgExprStatement* newlistOfInit = SageBuilder::buildAssignStatement(incrementVar, SageBuilder::buildIntVal(start+5));
      newlistOfInit->set_parent(secondLoop);
      listOfInit.push_back(newlistOfInit);
      SgForInitStatement* secondLoop_init = SageBuilder::buildForInitStatement(listOfInit);
      secondLoop->set_for_init_stmt(secondLoop_init);
      secondLoop_init->set_parent(secondLoop);
      secondLoop->set_parent(floop->get_parent());
      SageInterface::insertStatementAfter(floop, secondLoop);

      //if nbOfIteration == 7
      if( (nbOfIteration%2) != 0 ) {
        SgBasicBlock * bodyCopy = isSgBasicBlock(SageInterface::copyStatement(floop->get_loop_body()));
        bodyCopy->set_parent(secondLoop->get_parent());
        SageInterface::insertStatementListAfter(secondLoop, bodyCopy->get_statements());

        s2s_api::constantPropagation(bodyCopy , incrementVar->get_symbol(), nbOfIteration); 
      }

    // Transformation of the loop into vectorize loops of 4/2 iterations
    } else {
      s2s_api::add_directive( floop, directive );
      s2s_api::add_directive( floop, directive_next );
      //if nbOfIteration == 5 / 3
      if((nbOfIteration % 2) != 0) {
        SgVarRefExp * incrementVar;

        floop->set_test(SageBuilder::buildExprStatement(SageBuilder::buildLessOrEqualOp(incrementVar ,SageBuilder::buildIntVal(nbOfIteration-1+start))));
        
        incrementVar = isSgVarRefExp(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_lhs_operand());
        SgExprStatement * newExpr = SageBuilder::buildAssignStatement(incrementVar, SageBuilder::buildIntVal(nbOfIteration+start));
        newExpr->get_file_info()->set_col(floop->get_file_info()->get_col());

        // ajout de la dernière iteration si necessaire
        SageInterface::insertStatementListAfter(floop, isSgBasicBlock(floop->get_loop_body())->get_statements());
        // ajout de l'assignation de la variable d'iteration de la boucle
        // pour ne pas à avoir à gérer tout de suite la propagation de constante
        SageInterface::insertStatementAfter(floop, newExpr);
        newExpr->set_parent(floop->get_parent());
      }
    }
  } else if (typeOfLoop == DO) {
    SgFortranDo * floop = isSgFortranDo(loop);
    // If the bound is known
    if (isSgNullExpression(floop->get_bound ())) {
      nbOfIteration = 1;
    }
    else if (!isSgIntVal (floop->get_bound ()))  {
      if (LOG) s2s_api::log("Error : The bound of the loop wasn't Integer type it's : "+floop->get_bound()->class_name()+" the loop will not be transform.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(floop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
    } else {
      nbOfIteration = isSgIntVal (floop->get_bound())->get_value();
    }

    // If the bound respect some conditions
    if( ! (nbOfIteration < MAXBOUNDFORPOORVEC && nbOfIteration > 2 ) ) {
        return false;
    }
    //SgBasicBlock * bodyCopy = isSgBasicBlock(SageInterface::copyStatement (floop->get_body()));
    SgNode * parent = floop->get_parent ();
    std::string directive ="SIMD";
    std::string directive_next ="VECTOR UNALIGNED";

    // if nbOfIteration == 6/7
    if ( nbOfIteration >= 6 ) {
      SgVarRefExp * incrementVar;
      incrementVar = isSgVarRefExp(isSgBinaryOp(floop->get_initialization())->get_lhs_operand());

      // split de la boucle en 2 boucles :
      // l'une de 4 iterations
      floop->set_bound (SageBuilder::buildIntVal(4));
      // Ajout des directives sur les boucles créées
      s2s_api::add_directive( floop, directive );
      s2s_api::add_directive( floop, directive_next );

      // l'autre de 2
      SgFortranDo * secondLoop = isSgFortranDo(SageInterface::copyStatement(floop));
      //s2s_api::add_directive(secondLoop, directive);
      secondLoop->set_bound(SageBuilder::buildIntVal(6));
      SgExprStatement* secondLoop_init = SageBuilder::buildAssignStatement(incrementVar, SageBuilder::buildIntVal(5));
      secondLoop->set_initialization(secondLoop_init->get_expression());
      secondLoop_init->set_parent(secondLoop);
      secondLoop->set_parent(floop->get_parent());
      SageInterface::insertStatementAfter(floop, secondLoop);

      //if nbOfIteration == 7
      if( (nbOfIteration%2) != 0 ) {
        SgBasicBlock * bodyCopy = isSgBasicBlock(SageInterface::copyStatement(floop->get_body()));
        bodyCopy->set_parent(secondLoop->get_parent());
        SageInterface::insertStatementListAfter(secondLoop, bodyCopy->get_statements());

        s2s_api::constantPropagation(bodyCopy , incrementVar->get_symbol(), nbOfIteration);      
      }

    // Transformation of the loop into vectorize loops of 4/2 iterations
    } else {
      s2s_api::add_directive( floop, directive );
      s2s_api::add_directive( floop, directive_next );
      //if nbOfIteration == 5 / 3
      if((nbOfIteration % 2) != 0) {
        SgVarRefExp * incrementVar;

        floop->set_bound(SageBuilder::buildIntVal(nbOfIteration-1));
        
        incrementVar = isSgVarRefExp(isSgBinaryOp(floop->get_initialization())->get_lhs_operand());
        SgExprStatement * newExpr = SageBuilder::buildAssignStatement(incrementVar, SageBuilder::buildIntVal(nbOfIteration));
        newExpr->get_file_info()->set_col(floop->get_file_info()->get_col());

        // ajout de la dernière iteration si necessaire
        SageInterface::insertStatementListAfter(floop, isSgBasicBlock(floop->get_body())->get_statements());
        // ajout de l'assignation de la variable d'iteration de la boucle
        // pour ne pas à avoir à gérer tout de suite la propagation de constante
        SageInterface::insertStatementAfter(floop, newExpr);
        newExpr->set_parent(floop->get_parent());
      }
    }
  }
  hasBeenTransformed = true;
  return true;
}

bool Loop::shortVectoGen (int moduloVal) {
  if(DEBUG) DBG_MAQAO
  
  int nbOfIteration = 0, MAXBOUNDFORPOORVEC = 4, start=0 ;
  
  if (typeOfLoop == FOR) {
    SgForStatement * floop = isSgForStatement(loop);

    // If the bound is known and if it's  of type : x <(=) 8
    if (SgExprStatement* exprstmt = isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])) {
      if(SgBinaryOp* init_binop = isSgBinaryOp(exprstmt->get_expression()) ) {
        if ((!isSgIntVal(init_binop->get_rhs_operand()) && !isSgVarRefExp(init_binop->get_lhs_operand()))
            || floop->get_for_init_stmt()->get_init_stmt().size() > 1 
            || !(isSgLessThanOp(isSgExprStatement(floop->get_test())->get_expression()) 
               || isSgLessOrEqualOp(isSgExprStatement(floop->get_test())->get_expression()))) 
        {
          if (LOG) {s2s_api::log("The loop is not cannonical.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
          return false;
        }
      }
    }

    nbOfIteration = isSgIntVal(isSgBinaryOp(isSgExprStatement(floop->get_test())->get_expression())->get_rhs_operand())->get_value();
    // if the condition is <= add one to the number of iterations
    if (isSgLessOrEqualOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())) {
      nbOfIteration++;
    }

    if(isSgIntVal(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_rhs_operand())) {
      start = isSgIntVal(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_rhs_operand())->get_value();
      nbOfIteration -= start;
    } else {
      if (LOG) {s2s_api::log("The loop is not cannonical (initialization).", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
      return false;
    }

    // If the bound respect some conditions
    if( ! (nbOfIteration <= MAXBOUNDFORPOORVEC && nbOfIteration >= 2 ) ) {
      if (LOG) {DBG_MAQAO std::string msg = "Something wring with the number of iteration, as reminder, the max number of iteration authoized is 7 ans the minimum is 3."; s2s_api::log(msg, "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");}
      return false;
    }

    SgNode * parent = floop->get_parent();
    std::string directive ="simd";
    std::string directive_next ="vector unaligned";

    // if nbOfIteration == 6/7
    if ( nbOfIteration >= 6 ) {
      SgVarRefExp * incrementVar;
      incrementVar = isSgVarRefExp(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_lhs_operand());
      ROSE_ASSERT(incrementVar);
      // split the loop into 2 loops :
      
      // one of four iterations
      SgStatement* newTest = SageBuilder::buildExprStatement(SageBuilder::buildLessOrEqualOp(incrementVar, SageBuilder::buildIntVal(start+4)));
      floop->set_test (newTest);
      newTest->set_parent(floop);
      // Add directives abouves new loops
      s2s_api::add_directive( floop, directive );
      s2s_api::add_directive( floop, directive_next );

      //The other of two iterations
      SgForStatement * secondLoop = isSgForStatement(SageInterface::copyStatement(floop));

      SgExprStatement* secondLoop_test = SageBuilder::buildExprStatement(SageBuilder::buildLessOrEqualOp(incrementVar, SageBuilder::buildIntVal(start+6)));
      secondLoop->set_test(secondLoop_test);
      secondLoop_test->set_parent(secondLoop);
      SgStatementPtrList listOfInit;
      SgExprStatement* newlistOfInit = SageBuilder::buildAssignStatement(incrementVar, SageBuilder::buildIntVal(start+5));
      newlistOfInit->set_parent(secondLoop);
      listOfInit.push_back(newlistOfInit);
      SgForInitStatement* secondLoop_init = SageBuilder::buildForInitStatement(listOfInit);
      secondLoop->set_for_init_stmt(secondLoop_init);
      secondLoop_init->set_parent(secondLoop);
      secondLoop->set_parent(floop->get_parent());
      SageInterface::insertStatementAfter(floop, secondLoop);

      //if nbOfIteration == 7
      if( (nbOfIteration%2) != 0 ) {
        SgBasicBlock * bodyCopy = isSgBasicBlock(SageInterface::copyStatement(floop->get_loop_body()));
        bodyCopy->set_parent(secondLoop->get_parent());
        SageInterface::insertStatementListAfter(secondLoop, bodyCopy->get_statements());

        s2s_api::constantPropagation(bodyCopy , incrementVar->get_symbol(), nbOfIteration); 
      }

    // Transformation of the loop into vectorize loops of 4/2 iterations
    } else {
      s2s_api::add_directive( floop, directive );
      s2s_api::add_directive( floop, directive_next );
      //if nbOfIteration == 5 / 3
      if((nbOfIteration % 2) != 0) {
        SgVarRefExp * incrementVar;

        floop->set_test(SageBuilder::buildExprStatement(SageBuilder::buildLessOrEqualOp(incrementVar ,SageBuilder::buildIntVal(nbOfIteration-1+start))));
        
        incrementVar = isSgVarRefExp(isSgBinaryOp(isSgExprStatement(floop->get_for_init_stmt()->get_init_stmt()[0])->get_expression())->get_lhs_operand());
        SgExprStatement * newExpr = SageBuilder::buildAssignStatement(incrementVar, SageBuilder::buildIntVal(nbOfIteration+start));
        newExpr->get_file_info()->set_col(floop->get_file_info()->get_col());

        // ajout de la dernière iteration si necessaire
        SageInterface::insertStatementListAfter(floop, isSgBasicBlock(floop->get_loop_body())->get_statements());
        // ajout de l'assignation de la variable d'iteration de la boucle
        // pour ne pas à avoir à gérer tout de suite la propagation de constante
        SageInterface::insertStatementAfter(floop, newExpr);
        newExpr->set_parent(floop->get_parent());
      }
    }
  } else if (typeOfLoop == DO) {
    SgFortranDo * floop = isSgFortranDo(loop);
    // If the bound is known
    if (isSgNullExpression(floop->get_bound ())) {
      if (LOG) s2s_api::log("Error : The bound of the loop is NULL, the loop will not be transform.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(floop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
    }

    // If the bound respect some conditions
    if( moduloVal != MAXBOUNDFORPOORVEC ) {
      if (LOG) s2s_api::log("Error : The value of the modulo is not correct, the loop will not be transform.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(floop)->get_sourceFileNameWithoutPath ()+".log");
      return false;
    }

    SgNode * parent = floop->get_parent ();
    std::string directive ="SIMD";
    std::string directive_next ="VECTOR UNALIGNED";
    
    SgExpression* loopInitRhs = isSgBinaryOp(floop->get_initialization())->get_rhs_operand();
    if (!loopInitRhs) { return false; }
    SgExpression* loopBound= floop->get_bound();
    if (!loopBound) { return false; }
    SgStatement* toReplace = floop;

    for (int i = 0; i < moduloVal; i++) {
      if (i == 0) {
        SgSubtractOp* subtract_tmp = SageBuilder::buildSubtractOp(loopInitRhs,SageBuilder::buildIntVal(1));
        SgModOp* modOp = SageBuilder::buildModOp(
          SageBuilder::buildSubtractOp(loopBound,subtract_tmp),
          SageBuilder::buildIntVal(moduloVal));
        modOp->set_need_paren(true);

        SgIntVal* intVal = SageBuilder::buildIntVal(0);
        // Build the condition for the if stmt
        SgExpression* cond = SageBuilder::buildEqualityOp(modOp, intVal);
        modOp->set_parent(cond);
        intVal->set_parent(cond);
        SgFortranDo * copyfloop = isSgFortranDo(SageInterface::copyStatement(floop));
        SgBasicBlock* false_body = SageBuilder::buildBasicBlock(SageInterface::copyStatement(toReplace));
        //Build if stmt
        SgIfStmt* ifstmt = SageBuilder::buildIfStmt(cond, copyfloop, false_body);
        
        //Set parents
        ifstmt->set_parent(parent);
        cond->set_parent(ifstmt);
        copyfloop->set_parent(ifstmt);

        SageInterface::insertStatement(toReplace, ifstmt);

        SageInterface::removeStatement(toReplace, false);

        //Add directives
        s2s_api::add_directive( copyfloop, directive );
        s2s_api::add_directive( copyfloop, directive_next );

        //For the next iteration
        toReplace =  ifstmt;
      } else if (i == 1) {
        SgSubtractOp* subtract_tmp = SageBuilder::buildSubtractOp(loopInitRhs,SageBuilder::buildIntVal(1));
        SgModOp* modOp = SageBuilder::buildModOp(
          SageBuilder::buildSubtractOp(loopBound,subtract_tmp),
          SageBuilder::buildIntVal(moduloVal));

        modOp->set_need_paren(true);

        SgIntVal* intVal = SageBuilder::buildIntVal(1);
        // Build the condition for the if stmt
        SgExpression* cond = SageBuilder::buildEqualityOp(modOp, intVal);
        modOp->set_parent(cond);
        intVal->set_parent(cond);
        SgFortranDo * copyfloop = isSgFortranDo(SageInterface::copyStatement(floop));
        SgExpression* newBound = SageBuilder::buildSubtractOp(loopBound, SageBuilder::buildIntVal(1));
        copyfloop->set_bound(newBound);

        SgBasicBlock * false_body = SageBuilder::buildBasicBlock(SageInterface::copyStatement(toReplace));
        //Build if stmt
        SgIfStmt* ifstmt = SageBuilder::buildIfStmt(cond, copyfloop, false_body);
        //Build the last iteration
        SgBasicBlock* lastIteration = isSgBasicBlock(SageInterface::copyStatement(floop->get_body()));
        //Replace var step by the bound expression in the last iteration
        s2s_api::replace_expr(lastIteration, isSgVarRefExp(isSgBinaryOp(floop->get_initialization())->get_lhs_operand())->get_symbol (), loopBound);

        //Set parents
        ifstmt->set_parent(parent);
        false_body->set_parent(ifstmt);

        SageInterface::insertStatement(toReplace, ifstmt);
      
        SageInterface::removeStatement(toReplace, false);

        //Insert the last iteration after the copy of the loop.
        SageInterface::insertStatementAfter(copyfloop, lastIteration);
        lastIteration->set_parent(ifstmt);

        //Add directives
        s2s_api::add_directive( copyfloop, directive );
        s2s_api::add_directive( copyfloop, directive_next );

        //For the next iteration
        toReplace =  ifstmt;
      } else if (i == 2) {
        SgSubtractOp* subtract_tmp = SageBuilder::buildSubtractOp(loopInitRhs,SageBuilder::buildIntVal(1));
        SgModOp* modOp = SageBuilder::buildModOp(
          SageBuilder::buildSubtractOp(loopBound,subtract_tmp),
          SageBuilder::buildIntVal(moduloVal));
        modOp->set_need_paren(true);
        SgIntVal* intVal = SageBuilder::buildIntVal(2);
        // Build the condition for the if stmt
        SgExpression* cond = SageBuilder::buildEqualityOp(modOp, intVal);
        modOp->set_parent(cond);
        intVal->set_parent(cond);

        //Copy the first loop
        SgFortranDo * copyfloop = isSgFortranDo(SageInterface::copyStatement(floop));
        //Copy of the second loop
        SgFortranDo* secondLoop = isSgFortranDo(SageInterface::copyStatement(floop));

        SgExpression* newBoundloop1 = SageBuilder::buildAddOp(loopInitRhs, SageBuilder::buildIntVal(1));
        SgExpression* newInitloop2  = SageBuilder::buildAddOp(loopInitRhs, SageBuilder::buildIntVal(2));

        //Set the new bound for the loop 1 and the new init for the loop 2
        copyfloop->set_bound(newBoundloop1);
        newBoundloop1->set_parent(copyfloop);
        secondLoop->set_initialization(newInitloop2);
        newInitloop2->set_parent(secondLoop);


        //Build if stmt
        SgBasicBlock* false_body= SageBuilder::buildBasicBlock(SageInterface::copyStatement(toReplace));
        SgBasicBlock* true_body = SageBuilder::buildBasicBlock(copyfloop, secondLoop);
        SgIfStmt* ifstmt = SageBuilder::buildIfStmt(cond, true_body, false_body);

        //Set parents
        ifstmt->set_parent(parent);
        false_body->set_parent(ifstmt);

        SageInterface::insertStatement(toReplace, ifstmt);
        SageInterface::removeStatement(toReplace, false);

        //Add directives
        s2s_api::add_directive( copyfloop, directive );
        s2s_api::add_directive( copyfloop, directive_next );
        s2s_api::add_directive( secondLoop, directive );
        s2s_api::add_directive( secondLoop, directive_next );

        //For the next iteration
        toReplace =  ifstmt;
      } else if (i == 3) {
        SgSubtractOp* subtract_tmp = SageBuilder::buildSubtractOp(loopInitRhs,SageBuilder::buildIntVal(1));
        SgModOp* modOp = SageBuilder::buildModOp(
          SageBuilder::buildSubtractOp(loopBound,subtract_tmp),
          SageBuilder::buildIntVal(moduloVal));
        modOp->set_need_paren(true);
        SgIntVal* intVal = SageBuilder::buildIntVal(3);
        // Build the condition for the if stmt
        SgExpression* cond = SageBuilder::buildEqualityOp(modOp, intVal);
        modOp->set_parent(cond);
        intVal->set_parent(cond);

        //Copy the first loop
        SgFortranDo * copyfloop = isSgFortranDo(SageInterface::copyStatement(floop));
        //Copy of the second loop
        SgFortranDo* secondLoop = isSgFortranDo(SageInterface::copyStatement(floop));

        SgExpression* newBoundloop1 = SageBuilder::buildSubtractOp(loopBound, SageBuilder::buildIntVal(3));
        SgExpression* newInitloop2  = SageBuilder::buildSubtractOp(loopBound, SageBuilder::buildIntVal(2));
        SgExpression* newBoundloop2 = SageBuilder::buildSubtractOp(loopInitRhs, SageBuilder::buildIntVal(1));

        //Set the new bound for the loop 1 and the new init for the loop 2
        copyfloop->set_bound(newBoundloop1);
        newBoundloop1->set_parent(copyfloop);
        secondLoop->set_initialization(newInitloop2);
        newInitloop2->set_parent(secondLoop);
        secondLoop->set_bound(newBoundloop2);
        newBoundloop2->set_parent(secondLoop);

        //Build if stmt
        SgBasicBlock* true_body = SageBuilder::buildBasicBlock(copyfloop, secondLoop);
        SgBasicBlock* false_body= SageBuilder::buildBasicBlock(SageInterface::copyStatement(toReplace));

        SgIfStmt* ifstmt = SageBuilder::buildIfStmt(cond, true_body, false_body);

        //Build the last iteration
        SgBasicBlock* lastIteration = isSgBasicBlock(SageInterface::copyStatement(floop->get_body()));
        //Replace var step by the bound expression in the last iteration
        s2s_api::replace_expr(lastIteration, isSgVarRefExp(isSgBinaryOp(floop->get_initialization())->get_lhs_operand())->get_symbol (), loopBound);

        //Set parents
        ifstmt->set_parent(parent);
        true_body->set_parent(ifstmt);
        false_body->set_parent(ifstmt);
        
        SageInterface::insertStatement(toReplace, ifstmt);
        
        SageInterface::removeStatement(toReplace, false);

        //Insert the last iteration after the copy of the loop.
        // //Insert the last iteration after the copy of the loop.
        SageInterface::insertStatementAfter(secondLoop, lastIteration);
        lastIteration->set_parent(true_body);

        //Add directives
        s2s_api::add_directive( copyfloop, directive );
        s2s_api::add_directive( copyfloop, directive_next );
        s2s_api::add_directive( secondLoop, directive );
        s2s_api::add_directive( secondLoop, directive_next );

        //For the next iteration
        toReplace =  ifstmt;
      }
    }
  }
  hasBeenTransformed = true;
  return true;
}

bool Loop::split () {
  // TODO
  DBG_MAQAO
  std::cout << "The split transformation was not implemented yet." << std::endl;
}

bool Loop::isSplitable () {
  if(DEBUG) DBG_MAQAO
  SgStatementPtrList body = loop->getStatementList();
  std::vector<SgVarRefExp*> listOfArray;
  for (int i = 0; i < body.size(); i++) {
    if (SgExprStatement* exprstmt = isSgExprStatement(body[i])) {
      if (isSgAssignOp(exprstmt->get_expression()) || isSgCompoundAssignOp(exprstmt->get_expression())) {
        SgBinaryOp* assOp = isSgBinaryOp(exprstmt->get_expression());
        if (SgPntrArrRefExp* arrRef = isSgPntrArrRefExp(assOp->get_lhs_operand())) {
          if (SgVarRefExp* varRef =isSgVarRefExp(arrRef->get_lhs_operand())) {
            bool isIndependant = true;

            for (int j = 0; j < listOfArray.size(); j++) {
              if (varRef->get_symbol()->get_name().getString() == listOfArray[j]->get_symbol()->get_name().getString()) {
                isIndependant = false;
              }
              if (s2s_api::isinExpr(assOp->get_rhs_operand(), listOfArray[j]->get_symbol())) {
                isIndependant = false;
              }
              //TODO ADD a way to search if a tmp variable is used and it's a hiden array behind
            } // end for

            if (isIndependant) {
              listOfArray.push_back(varRef);
            }
          } // end if is Var RefExp 
        } // end if is ptnrarrRefExp
      } // end if is Assign
      // else if () {} // TODO HANDLE CALLS
    } // end if is ExprStmt
  } // end for

  if (listOfArray.size() > 1 ) {
    std::cout << s2s_api::getEnclosingsourceFile_RosePtr(loop)->getFileName() 
              << " contains a loop with more than 2 differents arrays stores" << std::endl;
    std::cout << "The loop line " << get_line_start() << " contains independant arrays :" << std::endl;

    for (int j=0; j < listOfArray.size(); j++) {
      std::cout << listOfArray[j]->get_symbol()->get_name().getString() << " ";
    }
    std::cout << std::endl;
    std::cout << "===========================================" << std::endl;
  }
  return true;
}

bool Loop::isJamable() {
  if(DEBUG) DBG_MAQAO
  SgBasicBlock * bodyBB;
  if(typeOfLoop == DO) bodyBB = isSgBasicBlock(isSgFortranDo(loop)->get_body());
  else if (typeOfLoop == FOR) bodyBB = isSgBasicBlock(isSgForStatement(loop)->get_loop_body());
  else if (typeOfLoop == WHILE) bodyBB = isSgBasicBlock(isSgWhileStmt(loop)->get_body());
  else if (typeOfLoop == DOWHILE) bodyBB = isSgBasicBlock(isSgDoWhileStmt(loop)->get_body());

  if(bodyBB) {
    SgStatementPtrList & stmtsList = bodyBB->get_statements();
    SgStatementPtrList::iterator p = stmtsList.begin();
    if( SageInterface::is_Fortran_language() ) {
      if (isSgFortranDo(*p)) {
        p++;
        for (; p != stmtsList.end(); ++p) {
          if (!isSgFortranDo(*p) || isSgVariableDeclaration (*p))
            return false;
        }
        return true;
      }
    }
    else {
      if (isSgForStatement(*p)) {
        p++;
        for (; p != stmtsList.end(); ++p) {
          if (!isSgForStatement(*p) || isSgVariableDeclaration (*p)) 
            return false;
        }
        return true;
      }
    }
  }
  return false;
}

bool Loop::specialize(std::vector<variable_spe*> var) {
  if(DEBUG) DBG_MAQAO
  std::vector<SgSymbol*> v_symbol;
  std::vector<int>       v_val;
  SgSymbolTable *        st = s2s_api::get_function_RosePtr(loop)->get_body()->get_symbol_table();

  //Search all variable symbol into symbols tables
  for(int i=0; i < var.size(); i++) {
    if (std::string(var[i]->var_name) == "ifthen" || std::string(var[i]->var_name) == "ifelse") {
      continue;
    }

    SgSymbol* vsym = st->find_variable(std::string(var[i]->var_name));

    if (!vsym) {
      SgSymbolTable * stfunc = s2s_api::get_function_RosePtr(loop)->get_symbol_table();
      vsym = stfunc->find_variable(strdup(var[i]->var_name));
    }
    if (!vsym) {
      SgSymbolTable * stloop = loop->get_symbol_table();
      vsym = stloop->find_variable(strdup(var[i]->var_name));
    }    
    if (!vsym) {
      if(LOG) {
        std::cout << "The loop line  "<< loop->get_file_info()->get_line()
        << " will not be specialize, because the variable \"" << std::string(var[i]->var_name)
        << "\" was not found in this scope."
        << std::endl;
      }
      return false;
    }

    if (!isSgTypeInt(vsym->get_type ())) {
      if(LOG) {
        std::cout << "The loop line  "<< loop->get_file_info()->get_line()
          << " will not be specialize, because " << std::string(var[i]->var_name)
          << " is not an INTEGER"
          << std::endl;
      }
      //We will not operate if isn't a INTEGER
      return false;
    } else {
      v_symbol.push_back(vsym);
      if (var[i]->specialization == variable_spe::EQUALS) { v_val.push_back(var[i]->value);}
      else if (var[i]->specialization == variable_spe::INF) { v_val.push_back(var[i]->sup_bound); }
      else if (var[i]->specialization == variable_spe::SUP) { v_val.push_back(var[i]->inf_bound); }
      else if (var[i]->specialization == variable_spe::INTERVAL) {
      }
    }
  }

  SgStatement* loop_copy = SageInterface::copyStatement(loop);
  SgStatement* loop_orig = SageInterface::copyStatement(loop);

  /****************
  **   IF COND   **
  *****************/
  int icond = 0, ite = 0;
  bool firstcond = true;
  SgExprStatement * ifcond;
  for (icond = 0; (icond < var.size()) && (ite < v_symbol.size()); icond++) {
    if (std::string(var[icond]->var_name) == "ifthen" || std::string(var[icond]->var_name) == "ifelse") { 
      continue; 
    }
    if (firstcond) {
      ifcond = build_if_cond(var[icond], v_symbol[ite++], loop);
      firstcond = false;
    } else {
      ifcond = build_if_cond_from_ifcond(var[icond], v_symbol[ite++], loop, ifcond);
    }
  }


  SgBasicBlock * true_body   = SageBuilder::buildBasicBlock();
  SgBasicBlock * false_body  = SageBuilder::buildBasicBlock();

  SgIfStmt * ifstmt = SageBuilder::buildIfStmt (ifcond, true_body, false_body);

  /********************
  ** PARENT/CHILDREN **
  *********************/
  ifstmt->set_parent(loop->get_parent());
  ifcond->set_parent(ifstmt);
  true_body->set_parent(ifstmt);
  false_body->set_parent(ifstmt);

  /***************
  ** TRUE BODY **
  ****************/
  SageInterface::insertStatementAfter (loop, ifstmt);
  true_body->append_statement (loop_copy);

  isSgBasicBlock(true_body)->set_symbol_table(isSgScopeStatement(loop_copy)->get_symbol_table());
  isSgBasicBlock(true_body)->get_symbol_table()->set_parent(true_body);

  /***************
  ** FALSE BODY **
  ****************/
  false_body->insertStatementInScope (loop_orig, true);
  SgStatement *parentScope = isSgScopeStatement(loop_orig->get_parent());
  loop_orig->set_parent(false_body);
  // s2s_api::copy_directives(loop,loop_orig);

  isSgBasicBlock(false_body)->set_symbol_table(isSgScopeStatement(loop_orig)->get_symbol_table());
  isSgBasicBlock(false_body)->get_symbol_table()->set_parent(false_body);

  SageInterface::removeStatement(loop, true);
  s2s_api::copy_directives(ifstmt, loop_orig);
  s2s_api::copy_directives(ifstmt, loop_copy);

  s2s_api::remove_MAQAO_directives(ifstmt);

  /************************
  ** Propagate the const **
  *************************/
  // Propagate the constant throught the function copied 
  //and replace all call the de variable "v_symbol" by the value val
  for(int i = 0, ite = 0; i < var.size(); ++i) {
    if (var[i]->specialization == variable_spe::EQUALS 
        && std::string(var[i]->var_name) != "ifthen" 
        && std::string(var[i]->var_name) != "ifelse") {

      s2s_api::constantPropagation(loop_copy, isSgVariableSymbol(v_symbol[ite++]), var[i]->value);
    }
  }

  s2s_api::dead_code_elimination(loop_copy, var);

  /**************************
  ** APPLY DIR ON SPE LOOP **
  ***************************/
  SageInterface::fixStatement(loop_copy, true_body);
  ASTLoops* newloop = new ASTLoops (isSgScopeStatement(loop_copy), parent_id);
  newloop->apply_directive(var);

  /***************************
  ** APPLY DIR ON ORIG LOOP **
  ****************************/
  SageInterface::fixStatement(loop_orig, false_body);
  ASTLoops* newlooporig = new ASTLoops (isSgScopeStatement(loop_orig), parent_id);

  newlooporig->apply_directive();

  return true;
}

SgBasicBlock* Loop::jam_body(SgBasicBlock* body) {
  if(DEBUG) DBG_MAQAO
  SgBasicBlock * newBody = isSgBasicBlock( SageInterface::copyStatement(body));
  SgStatementPtrList & stmtsList = isSgBasicBlock(newBody)->get_statements();
  SgStatementPtrList::iterator p = stmtsList.begin();
  
  if( SageInterface::is_Fortran_language() ) {
    SgFortranDo * otherLoop = NULL;

    while(!isSgFortranDo(*p)) p++;
    SgFortranDo * firstLoop = isSgFortranDo(*p);

    p++;
    // prendre le statement de la loop et l'inserer avec append_statement (SgStatement *stmt) du scope de la firstLoop (vérifier aussi le class_name)
    for (; p != stmtsList.end(); ++p) {
      if(otherLoop != NULL) {
        p--;
        SgStatement * parent = isSgStatement(otherLoop->get_parent());
        parent->remove_statement(otherLoop);
      }
      otherLoop = isSgFortranDo(*p);
      if (otherLoop) {
        SgStatement * otherLoopBody = SageInterface::copyStatement(otherLoop->get_body());
        if(isSgScopeStatement(firstLoop->get_body())) {
          SageInterface::appendStatement (otherLoopBody, isSgScopeStatement(firstLoop->get_body()));
        }
        otherLoopBody->set_parent(firstLoop);
      }
    }
    if(otherLoop != NULL) {
        p--;
        SgStatement * parent = isSgStatement(otherLoop->get_parent());
        parent->remove_statement(otherLoop);
    }
    body = newBody;
  }
  else {
    SgForStatement * otherLoop = NULL;

    while(!isSgForStatement(*p)) p++;
    SgForStatement * firstLoop = isSgForStatement(*p);

    p++;
    // prendre le statement de la loop et l'inserer avec append_statement (SgStatement *stmt) du scope de la firstLoop (vérifier aussi le class_name)
    for (; p != stmtsList.end(); ++p) {
      if(otherLoop != NULL) {
        p--;
        SgStatement * parent = isSgStatement(otherLoop->get_parent());
        parent->remove_statement(otherLoop);
      }
      otherLoop = isSgForStatement(*p);
      if (otherLoop) {
        SgStatement * otherLoopBody = SageInterface::copyStatement(otherLoop->get_loop_body());
        if(isSgScopeStatement(firstLoop->get_loop_body())) {
          SageInterface::appendStatement (otherLoopBody, isSgScopeStatement(firstLoop->get_loop_body()));
        }
        otherLoopBody->set_parent(firstLoop);
      }
    }
    if(otherLoop != NULL) {
        p--;
        SgStatement * parent = isSgStatement(otherLoop->get_parent());
        parent->remove_statement(otherLoop);
    }
    body = newBody;
  }
  return newBody;
}

bool Loop::clean(int nb_ite) {
  if (DEBUG) DBG_MAQAO
  if (typeOfLoop == DO) {
    SgFortranDo * floop = isSgFortranDo(loop);
    SgStatement * parent = isSgStatement(floop->get_parent());

    // Test if :
    // (init + (step*nb_iter))-step == bound
    SgExpression* loopInitRhs = isSgBinaryOp(floop->get_initialization())->get_rhs_operand();
    SgExpression* loopBound   = floop->get_bound();
    SgExpression* loopStep    = floop->get_increment();
    if (!loopBound || !loopInitRhs) { return false; }
    if (!loopStep || isSgNullExpression(loopStep)) loopStep = SageBuilder::buildIntVal(1);

    // Build the condition for the if stmt
    SgMultiplyOp * mulOp = SageBuilder::buildMultiplyOp(SageBuilder::buildIntVal(nb_ite-1), loopStep);
    mulOp->set_need_paren(true);
    SgAddOp * addOp = SageBuilder::buildAddOp(loopInitRhs, mulOp); 
    addOp->set_need_paren(true);
    SgEqualityOp * eqOp = SageBuilder::buildEqualityOp(addOp, loopBound);
    eqOp->set_need_paren(true);

    mulOp->set_parent(addOp);
    addOp->set_parent(eqOp);
    SgFortranDo * copyfloop = isSgFortranDo(SageInterface::copyStatement(floop));
    
    SgBasicBlock* false_body = SageBuilder::buildBasicBlock(SageInterface::copyStatement(loop));
    //Build if stmt
    SgIfStmt* ifstmt = SageBuilder::buildIfStmt(eqOp, copyfloop, false_body);
          
    //Set parents
    ifstmt->set_parent(parent);
    eqOp->set_parent(ifstmt);
    copyfloop->set_parent(ifstmt);

    SageInterface::insertStatement(loop, ifstmt,false,false);
    SageInterface::removeStatement(loop, false);
    
    //Full unroll the loop with small number of iterations
    Loop * cfl = new Loop(copyfloop, -1);
    cfl->fullUnroll(nb_ite);
  }
}

bool Loop::replaceByText(std::string text) {
  if(DEBUG) DBG_MAQAO
  /********************************************************
   ** In text, \n is detected as two separate characters **
   ** and not the char '\n'                              **
   ** So, we replace it in text                          **
   ********************************************************/
  const std::string s = std::string("\\")+std::string("n");
  const std::string t = std::string("\n");

  std::string::size_type n = 0;  
  if (DEBUG) std::cout << "text : \n" << text << std::endl;
  text = text + "\n";
  while ( ( n = text.find( s, n ) ) != std::string::npos )
  {
      text.replace( n, s.size(), t );
      n += t.size();
  }


  //Attached the text after the previous statement
  SgStatement* prevStmt = SageInterface::getPreviousStatement(loop);
  AstUnparseAttribute* code = new AstUnparseAttribute(text,AstUnparseAttribute::e_after);
  ROSE_ASSERT(code != NULL);

  prevStmt->addNewAttribute(AstUnparseAttribute::markerName,code);

  // SageInterface::addTextForUnparser(SageInterface::getNextStatement(loop), text, AstUnparseAttribute::e_before);
  SageInterface::removeStatement (loop, /*bool autoRelocatePreprocessingInfo=*/true);
}

void Loop::whatSpecialize (std::vector<SgVariableSymbol*> & varList) {
  if(DEBUG) DBG_MAQAO

  // LOOK AT THE BODY
  SgStatement * bodyStmt = get_body();
  if (SgBasicBlock* body = isSgBasicBlock(bodyStmt)) {
      SgStatementPtrList & stmtsList = body->get_statements (); 
      for (int i=stmtsList.size()-1; i >= 0; --i) {
        s2s_api::whatSpecialize(stmtsList[i], varList); 
      }
  } else {
    // If thinks there is not other case to handle
    std::cout << "There is a problem with the loop body" << std::endl;
  }

  if (typeOfLoop == FOR) {
    // CHECK THE LOOP
    SgForStatement* l = isSgForStatement(loop);
    // CHECK INIT AND REMOVE WHAT WE NEED TO REMOVE
    SgStatementPtrList & forinitList = l->get_for_init_stmt()->get_init_stmt ();
    if (forinitList.size() > 1) {
      std::cout << "WARNING: the loop have more than one initialization and the analyze could not work correctly." << std::endl;
    }
    for (int i=0; i < forinitList.size(); ++i) {
      if (SgExprStatement* exprInit = isSgExprStatement(forinitList[i])) {
        if(SgBinaryOp* init = isSgBinaryOp(exprInit->get_expression())) {
          if (SgVarRefExp* varRef = isSgVarRefExp(init->get_lhs_operand_i())) {
            SgVariableSymbol* varSymLHS = varRef->get_symbol();
            for (int i=0; i < varList.size(); ++i) {
              // If we found it we can reove it and exit, because there only one iteration of the variable in the list
              if (varSymLHS->get_name().getString() == varList[i]->get_name().getString()) {
                // CHECK COND AND ADD WHAT WE NEED TO ADD 
                //Look into condition to know what involved this variable
                SgExpression* cond = isSgExprStatement(l->get_test())->get_expression();
                s2s_api::whatSpecialize(cond,varList);
                // And into the rhs of the init
                s2s_api::whatSpecialize(init->get_rhs_operand_i(), varList);
                // Remove the variable which are not usefull to follow
                // std::cout << "ERASE : " << varList[i]->get_name().getString() << std::endl;
                varList.erase(varList.begin()+i);
                break;
              }
            }
          }
        }
      } 
    }
  } else if (typeOfLoop == DO) {
    // CHECK THE LOOP
    SgFortranDo* l = isSgFortranDo(loop);
    // CHECK INIT AND REMOVE WHAT WE NEED TO REMOVE
    if(SgBinaryOp* init = isSgBinaryOp(l->get_initialization ())) {
      if (SgVarRefExp* varRef = isSgVarRefExp(init->get_lhs_operand_i())) {
        SgVariableSymbol* varSymLHS = varRef->get_symbol();
        for (int i=0; i < varList.size(); ++i) {
          // If we found it we can reove it and exit, because there only one iteration of the variable in the list
          if (varSymLHS->get_name().getString() == varList[i]->get_name().getString()) {
            // CHECK COND AND ADD WHAT WE NEED TO ADD 
            //Look into condition to know what involved this variable
            SgExpression* cond = l->get_bound();
            s2s_api::whatSpecialize(cond,varList);
            // And into the rhs of the init
            s2s_api::whatSpecialize(init->get_rhs_operand_i(), varList);
            // Remove the variable which are not usefull to follow
            if(DEBUG) std::cout << "ERASE : " << varList[i]->get_name().getString() << std::endl;
            varList.erase(varList.begin()+i);
            break;
          }
        }
      } 
    }
  } else {
    std::cout << "Format("<<loopType_toString()<<") of the loop is not supported to looking for what to analyze" << std::endl;
  }
}

//Currently it's only working with the keyword "continue"
bool handleStopKeyword (SgBasicBlock* body) {
  if(DEBUG) DBG_MAQAO
  if (!body) return false;

  SgBasicBlock* underbody = isSgBasicBlock(body);
  for (int k = 0; k < body->get_statements ().size(); ++k) {
    //Check if one statement of the BB is a sub basick block to check inside recursively
    if (isSgBasicBlock(body->get_statements ()[k])) { 
      handleStopKeyword(isSgBasicBlock(body->get_statements ()[k])); 
    } else {
      //In the new body, we have to check if they're any "continue" keyword. 
      SgStatementPtrList & listStmt = isSgBasicBlock(underbody)->get_statements ();

      for(int i=0; i < listStmt.size(); ++i) {
        if (SgIfStmt* ifstmt = isSgIfStmt(listStmt[i])) {
          // Search the position of the stop keyword if exist (else -1 will be returned) 
          int thereIsAStopStmt = s2s_api::thereIsAStopStmt(ifstmt->get_true_body());
          // Check if we detect a stop keyword in the else body, return false if we found it because we dont handle it yet
          if (ifstmt->get_false_body()) {
            if (s2s_api::thereIsAStopStmt(ifstmt->get_false_body())){
              if (LOG) s2s_api::log("Cannot transform this loop due to a stop keyword in a \"else\" body of an \"if\" statement.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(body)->get_sourceFileNameWithoutPath ()+".log");
              return false;
            }
          }

          //If the first statement of the body is a continue we can handle it
          if (thereIsAStopStmt == 0) {
            // If exist a "else" body, we have to invert the conditionnal and move the "else" body at the then body and remove the original 
            if (ifstmt->get_false_body()) {
              ifstmt->set_conditional(
                SageBuilder::buildExprStatement(
                  SageBuilder::buildNotOp(
                    isSgExprStatement(ifstmt->get_conditional())->get_expression())));

              ifstmt->set_true_body(SageInterface::copyStatement(ifstmt->get_false_body()));
              SageInterface::removeStatement(ifstmt->get_false_body(), false);
              ifstmt->set_false_body(NULL);
              ifstmt->get_true_body()->set_parent(ifstmt);
              
              handleStopKeyword(isSgBasicBlock(ifstmt->get_true_body()));
            } else { // If they're not else body, then we create it because statements have to be execute only if we don't take the continue
              ifstmt->set_conditional(
                SageBuilder::buildExprStatement(
                  SageBuilder::buildNotOp(
                    isSgExprStatement(ifstmt->get_conditional())->get_expression())));
              SgBasicBlock * new_if_true_body = SageBuilder::buildBasicBlock();
              for(int j=i+1; j < listStmt.size(); j++) {
                new_if_true_body->append_statement(SageInterface::copyStatement(listStmt[j]));
                if (VERBOSE > 1 || DEBUG) std::cout << "[remove] " << listStmt[j]->unparseToString() << std::endl;
                SageInterface::removeStatement(listStmt[j]);
              }
              ifstmt->set_true_body(new_if_true_body);
              new_if_true_body->set_parent(ifstmt);
              handleStopKeyword(new_if_true_body);

              break;
            }
          // Try to handle if the continue statement is not the first statement of the body
          } else if (thereIsAStopStmt > 0) {
            SgStatementPtrList & trueBodyList = isSgBasicBlock(ifstmt->get_true_body())->get_statements();
            //We remove all statement after it, because it will never be executed
            for (int t=thereIsAStopStmt; t < trueBodyList.size(); ++t) {
              if (VERBOSE > 1 || DEBUG) std::cout << "[remove] " << trueBodyList[t]->unparseToString() << std::endl;
              SageInterface::removeStatement(trueBodyList[t]);
            }

            //Moving statement after the if inside the new if 
            SgBasicBlock* nextstmt = SageBuilder::buildBasicBlock();
            for (int j=i+1; j < listStmt.size(); ++j) {
              nextstmt->append_statement(SageInterface::copyStatement(listStmt[j]));
              SageInterface::removeStatement(listStmt[j]);
            }

            SgIfStmt* secondIf = SageBuilder::buildIfStmt(
              SageBuilder::buildNotOp(
              SageInterface::copyExpression(isSgExprStatement(ifstmt->get_conditional())->get_expression())),
              nextstmt, NULL);

            secondIf->set_parent(ifstmt->get_parent());
            SageInterface::insertStatementAfter(ifstmt, secondIf);
            // this transformation risk to seg fault the loop due to moving statement somewhere else
            // so it must return now
            return true;
          } else { // thereIsAStopStmt < 0 no stop so no need to do anything
            continue;
          }
        } // if it's not a if statement, they're not reason to have a stop keyword
      }
    }
  }
  return true;
}

SgBasicBlock * unroll_body(SgBasicBlock * loopBody, SgVarRefExp * incrementVar, SgExpression * stride, size_t unrollSize) {
  if(DEBUG) DBG_MAQAO
  if (SgBasicBlock * unrolledBody = isSgBasicBlock(loopBody)) {
    SgScopeStatement * loopParent = isSgFortranDo(loopBody->get_parent());
    
    if (!loopParent) {loopParent = isSgForStatement(loopBody->get_parent());}

    if (loopParent) {
      SgStatementPtrList & bodyList = loopBody->get_statements();
      SgStatementPtrList::iterator endList = bodyList.end();
      // Duplicate the body
      SgBasicBlock * cpy_loop_body = isSgBasicBlock(SageInterface::copyStatement(loopBody));

      //Duplicate as time as the unroll factor and replace the variable by the varibale + iteration
      for(int i = 1; i < unrollSize; ++i) {
        SgBasicBlock * copyBody = isSgBasicBlock(SageInterface::copyStatement(cpy_loop_body));
        copyBody->set_parent(loopParent);
        loopParent->append_statement(copyBody);
        s2s_api::replace_expr (copyBody, incrementVar->get_symbol(), s2s_api::myBuildBinOp(incrementVar, SageBuilder::buildIntVal(i), V_SgAddOp));
      }

    } else {
      // The body to unroll is not from a loop
      if (VERBOSE > 1 || DEBUG) std::cout << "Parent is not a loop; parent is a " <<loopBody->get_parent()->class_name() << std::endl;
    }
    // return an unrolled BB attached to nobody
    return unrolledBody;
  }
  else {
    // Big trouble, the loopBody is not a BasicBlock
    std::cout << "problem ! \n";
    return NULL;
  }
}

bool forLoopUnroll(SgForStatement  * loop, size_t nest, size_t unrollSize) {
  if(DEBUG) DBG_MAQAO
  SgBasicBlock   * body ;
  SgStatement    * loopBody;
  SgForStatement * stmtLoop;
  bool goDeeper = false;
  
  //DON'T CHECK DEEPER, TAKE THE FIRST LOOP AFTER THE PRAGMA
  nest=0;

  loopBody = loop->get_loop_body();
  while(nest > 0) {
    body = isSgBasicBlock(loopBody);
    if(body) {
      SgStatementPtrList & stmtsList = body->get_statements();
      for (SgStatementPtrList::iterator p = stmtsList.begin(); 
            p != stmtsList.end() && !goDeeper; ++p) {
        
        stmtLoop = isSgForStatement(*p);
        if(stmtLoop) {
          loop = stmtLoop;
          loopBody = loop->get_loop_body();
          goDeeper = true;
        }
      }
      //If we not found any other loop inside the loop
      if(!goDeeper) {
        nest = 0;
        break;
      } else {
        --nest;
        goDeeper = false;
      }
    }
  }

  SgStatement * loopCopy = SageInterface::copyStatement(loop);
  SgForStatement * loopUnrolled = loopUnroll(loop, unrollSize);
  if(loopUnrolled) {
    SgIfStmt * newIfStmt;
    SgForStatement * residue;
    SgNode * parent = loop->get_parent();

    residue = isSgForStatement(SageInterface::copyStatement(loopCopy));
    newIfStmt = SageBuilder::buildIfStmt(SageBuilder::buildBoolValExp(true), loopUnrolled, loopCopy);
    newIfStmt->set_parent(parent);
    loopUnrolled->set_parent(newIfStmt);
    loopCopy->set_parent(newIfStmt);
    
    SageInterface::insertStatementAfter(loopUnrolled, residue);
    residue->set_parent(newIfStmt);

    SgStatementPtrList& initPtrList = loopUnrolled->get_init_stmt();
    
      //Add in the symbols table (in case of declaration type "int i")
      for (SgStatementPtrList::iterator p = initPtrList.begin(); p != initPtrList.end(); ++p) {
      SgStatement * copyVarDecl = SageInterface::copyStatement(*p);
        copyVarDecl->set_parent(newIfStmt);
      SageInterface::insertStatementBefore(loopUnrolled, copyVarDecl);
      }
      // /!\ BECAREFULL ! PROVISOIRE
      //TODO : n'effacer que les déclarations insérées avant la boucle
      //ERASE LAST DECLARATION FROM THE "FOR" UNROLLED STMT
      loopUnrolled->get_init_stmt().pop_back();
      //loopUnrolled->set_for_init_stmt(NULL);
      residue->get_init_stmt().pop_back();  
      isSgStatement(parent)->replace_statement(loop,newIfStmt);
      return true;
  } else {
    if (VERBOSE || DEBUG) {
      std::cout << "The Loop\n";
      std::cout << loop->unparseToString() << std::endl;
      std::cout << "Has not been unrolled \n";
    }
      return false;
  }
}

SgForStatement * loopUnroll(SgForStatement  * loop, size_t unrollSize) {
  if(DEBUG) DBG_MAQAO
  //Body
  SgStatement     * loopBody,  * loopBodyCopy;
  //INCREMENT / Stride
  SgExpression    * increment, * newIncrement;
  SgExprStatement * stride_stmt;  //Stride' statement
  //CONDITION
  SgStatement * test, * newTest;
  SgExpression * newCondition;
  bool testIsNull;
  //INITIALISATION
  SgForInitStatement * newInit;
  //For the New Statement wich will replace the actual
  SgForStatement * newForLoop;
  SgBasicBlock * loopBodyNew;

  //****************************************
  //*                STRIDE                *
  //****************************************
  increment = loop->get_increment();
  newIncrement = get_unroll_stride(increment, unrollSize);
  if(!newIncrement) {
    return NULL;
  }

  //****************************************
  //*               CONDITION              *
  //****************************************
  newCondition = get_unroll_cond(loop, unrollSize-1);
  if(newCondition) {
   newTest = SageBuilder::buildExprStatement(newCondition);
  }
  else { 
    return NULL;
  }

  //****************************************
  //*            INITIALIZATION            *
  //****************************************
  newInit = SageBuilder::buildForInitStatement(
                                    (loop->get_for_init_stmt ())->get_init_stmt ());

  SgExpression * stride; 
  SgVarRefExp * incrementVar = NULL;
  if(isSgUnaryOp(increment)) {
    incrementVar = isSgVarRefExp(isSgUnaryOp(increment)->get_operand ());
    if(isSgPlusPlusOp(increment))
      stride = SageBuilder::buildIntVal(1);
    else 
      stride = SageBuilder::buildIntVal(-1);
  }
  else{ 
    incrementVar = isSgVarRefExp(isSgBinaryOp(increment)->get_lhs_operand());
    stride = get_stride_from_rhs_expression(incrementVar,isSgBinaryOp(increment)->get_rhs_operand());
  }
  
  //****************************************
  //*                 BODY                 *
  //****************************************
  loopBodyNew = unroll_body(isSgBasicBlock(loop->get_loop_body()), incrementVar, stride, unrollSize);

  if(!loopBodyNew) {
    return NULL;
  }
  loopBodyNew->set_parent(newForLoop);

  newForLoop = myBuildForStatement(
    newInit,
    newTest, newIncrement,
    loopBodyNew);
    
  SgStatementPtrList& initPtrList = newForLoop->get_init_stmt();
  //Add in the symbols table (in case of declaration type "int i")
  for (SgStatementPtrList::iterator p = initPtrList.begin(); p != initPtrList.end(); ++p) {
    if (isSgVariableDeclaration( (*p) ) ) {
      SageInterface::fixVariableDeclaration(isSgVariableDeclaration((*p)),newForLoop);
      // fix varRefExp to the index variable used in increment, conditional expressions
      SageInterface::fixVariableReferences(newForLoop);
    }
  }
  return newForLoop;
}

SgFortranDo * loopUnroll(SgFortranDo * loop, size_t unrollSize) {
  if(DEBUG) DBG_MAQAO

  if (!loop->get_has_end_statement ()) {
    std::cout << "this format of loop is not suported yet. " << loop->unparseToString() << std::endl;
    return NULL;
  }

  SgBasicBlock * bodyCopy = isSgBasicBlock(SageInterface::copyStatement(loop->get_body()));
  
  //****************************************
  //*                STRIDE                *
  //****************************************
  SgExpression * stride = loop->get_increment();
  SgVarRefExp * incrementVar;
  if (isSgNullExpression(stride))  {
    loop->set_increment(SageBuilder::buildIntVal(1));
    stride = loop->get_increment();
  }

  if(isSgIntVal(stride))
    loop->set_increment(SageBuilder::buildIntVal(isSgIntVal(stride)->get_value()*unrollSize));
  else if (isSgVarRefExp(stride))
    loop->set_increment(SageBuilder::buildMultiplyOp(stride,SageBuilder::buildIntVal(unrollSize))); 
  else {
    std::cout << "The stride format wasn't recognized\n";
    NOT_IMPLEMENTED_YET
    return NULL;
  }

  //****************************************
  //*            INITIALIZATION            *
  //****************************************
  if(isSgBinaryOp(loop->get_initialization()))
    incrementVar = isSgVarRefExp(isSgBinaryOp(loop->get_initialization())->get_lhs_operand());
  else if(isSgVariableDeclaration(loop->get_initialization())) {
     SgInitializedNamePtrList& declList = isSgVariableDeclaration(loop->get_initialization())->get_variables ();

     if(declList.size() != 1) {
      std::cout << "Only one declaration per loop is available\n";
      NOT_IMPLEMENTED_YET
      return NULL;
    }

    incrementVar = isSgVarRefExp(*declList.begin());
  } else {
    std::cout << "Initialisation format wasn't recognized\n";
    NOT_IMPLEMENTED_YET
    return NULL;
  }
  //****************************************
  //*                BOUND                 *
  //****************************************
  SgExpression * boundExpr = loop->get_bound();
  loop->set_bound(SageBuilder::buildSubtractOp (boundExpr,SageBuilder::buildIntVal(unrollSize-1)));

  //****************************************
  //*              BODY UNROLL             *
  //****************************************
  bodyCopy = unroll_body(loop->get_body(), incrementVar, stride, unrollSize);
  
  return loop;
}

bool tile_fortran_do(SgFortranDo* loop,size_t tileSize) {
  if(DEBUG) DBG_MAQAO
  if (!loop) { return false; }
  // skip tiling if tiling size is 0 (no tiling), we allow 0 to get a reference value for the original code being tuned
  // 1 (no need to tile)
  if (tileSize<=1)
    return true;

  SgFortranDo* target_loop = isSgFortranDo(loop); // adjust to numbering starting from 0

  // grab the target loop's essential header information
  SgInitializedName* ivar = NULL;
  SgExpression* lb = NULL;
  SgExpression* ub = NULL;
  SgExpression* step = NULL;

    //Check the init of ivar and lb
    SgExpression* init = target_loop->get_initialization();
    if(isSgAssignOp(init)) {
      if (isSgVarRefExp(isSgBinaryOp(init)->get_lhs_operand()) 
        && isSgIntVal(isSgBinaryOp(init)->get_rhs_operand()))
      {
        ivar = isSgVarRefExp(isSgBinaryOp(init)->get_lhs_operand())->get_symbol()->get_declaration();
        lb = isSgIntVal(isSgBinaryOp(init)->get_rhs_operand());
      } else { if(LOG) {std::cout << "WARNING: cannot tile the loop l."<<loop->get_startOfConstruct()->get_line()<< " because the loop wasn't canonical.\n";} return false; }
    } else { if(LOG){ std::cout << "WARNING: cannot tile the loop l."<<loop->get_startOfConstruct()->get_line()<< " because the loop wasn't canonical.\n";} return false; }
    //Check the up
    ub = target_loop->get_bound();
    //Check the stride
    step = target_loop->get_increment();

  if(!(ivar&& lb && ub && step)) {
    if(LOG){
      std::cout << "WARNING: cannot tile the loop l."
      << loop->get_startOfConstruct()->get_line()
      << " because the loop wasn't canonical.\n";
    } 
    return false;
  }
  ROSE_ASSERT(ivar != NULL);
  ROSE_ASSERT(lb   != NULL);
  ROSE_ASSERT(ub   != NULL);
  ROSE_ASSERT(step != NULL);

  // Add a controlling loop around the top loop nest
  // Ensure the parent can hold more than one children
  SgLocatedNode* parent = NULL; //SageInterface::ensureBasicBlockAsParent(loopNest)
  if (SageInterface::isBodyStatement(loop)) // if it is a single body statement (Already a for statement, not a basic block)
    parent = SageInterface::makeSingleStatementBodyToBlock(loop);
  else
    parent = isSgLocatedNode(loop->get_parent());

  ROSE_ASSERT(parent!= NULL);

  // Now we can prepend a controlling loop index variable: _lt_var_originalIndex
  std::string ivar2_name = "lt_var_"+ivar->get_name().getString();
  SgScopeStatement* scope = loop->get_scope();
  ROSE_ASSERT(scope != NULL);
  if (!isSgScopeStatement(s2s_api::get_function_RosePtr(target_loop)->get_body())->symbol_exists (SgName(ivar2_name))) {
    SgVariableDeclaration* loop_index_decl = SageBuilder::buildVariableDeclaration (ivar2_name, SageBuilder::buildIntType(),NULL, scope);
  
    // insert the variable with other declaration at the top of the function
    SageInterface::insertStatementAfter (
        SageInterface::findLastDeclarationStatement(isSgScopeStatement(s2s_api::get_function_RosePtr(target_loop)->get_body())),
        loop_index_decl, 
        true);
  }

  // init statement of the loop header, copy the lower bound
  SgExpression* init_stmt = SageBuilder::buildAssignOp(SageBuilder::buildVarRefExp(ivar2_name,scope), SageInterface::copyExpression(lb));
  ROSE_ASSERT(init_stmt != NULL);

  SgExpression* cond_stmt = SageBuilder::buildNullExpression();
  ROSE_ASSERT(cond_stmt != NULL);

  // build loop incremental  I
  // expression var+=up*tilesize or var-=upper * tilesize
  SgExpression* incr_exp = NULL;
  SgExpression* orig_incr_exp = target_loop->get_increment();
  ROSE_ASSERT(orig_incr_exp != NULL);

  if (isSgNullExpression(step)) {
    incr_exp = SageBuilder::buildIntVal(tileSize);    
  } else if (isSgIntVal(step)) {
    if (isSgIntVal(step)->get_value() == 1) {
      incr_exp = SageBuilder::buildIntVal(tileSize);
    } else {
       if(LOG){ std::cout << "WARNING: cannot tile the loop l."<<loop->get_startOfConstruct()->get_line() << " because the loop stride is not equals to 1.\n";}
       std::cout << "The loop is : " << loop->unparseToString() << std::endl;
      return false;
    }
  } else {
    incr_exp = SageBuilder::buildMultiplyOp(SageInterface::copyExpression(step), SageBuilder::buildIntVal(tileSize));
  }
  ROSE_ASSERT(incr_exp != NULL);

  SgFortranDo* control_loop = new SgFortranDo(init_stmt, ub, incr_exp, SageBuilder::buildBasicBlock());
  isSgFortranDo(loop)->set_bound(cond_stmt);


  SgBinaryOp* cond = SageBuilder::buildSubtractOp(SageBuilder::buildAddOp(SageBuilder::buildVarRefExp(ivar2_name,scope),SageInterface::copyExpression(incr_exp)), SageBuilder::buildIntVal(1));
  cond->set_parent(loop);
  SageInterface::addTextForUnparser (cond_stmt, "min("+ub->unparseToString()+","+cond->unparseToString()+")", AstUnparseAttribute::e_before );

  control_loop->set_parent(loop->get_parent());
  control_loop->set_has_end_statement(true);
    
  SageInterface::insertStatementBefore(loop, control_loop);
  // move loopNest into the control loop
  SageInterface::removeStatement(loop);
  SageInterface::appendStatement(loop,isSgBasicBlock(control_loop->get_body()));
  loop->set_parent(control_loop);

  // rewrite the lower (i=lb), upper bounds (i<=/>= ub) of the target loop
  SgAssignOp* assign_op  = isSgAssignOp(init);
  ROSE_ASSERT(assign_op);
  assign_op->set_rhs_operand(SageBuilder::buildVarRefExp(ivar2_name,scope));
    
  SageInterface::setOneSourcePositionForTransformation (control_loop);

  ROSE_ASSERT(control_loop->get_file_info() != NULL);
  ROSE_ASSERT(loop->get_file_info() != NULL);
  ROSE_ASSERT(control_loop->get_bound() != NULL);
  ROSE_ASSERT(control_loop->get_increment() != NULL);

  //if (LOG) s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithPath ()+"] Tiled - success", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop)->get_sourceFileNameWithoutPath ()+".log");
  
  return true;
}

SgExpression * get_stride_from_rhs_expression(SgExpression * lhs, SgExpression * rhs) {
  if(DEBUG) DBG_MAQAO
  SgExpression * rightLhs, * rightRhs;
  SgVariableSymbol * varSymbLhs;
  SgBinaryOp * binOp;
  SgName lhsName;
  
  //if left hand side isn't an VariableExpression return NULL
  if(!isSgVarRefExp(lhs)) return NULL;
  
  varSymbLhs =  (isSgVarRefExp(lhs))->get_symbol();
  binOp = isSgBinaryOp(rhs);

  //If it's an Expression like a function call or just a variable
  if(!binOp) {
    if(isSgExpression(rhs)) {
      //The function Call is not implemented yet
      if(isSgCallExpression(rhs)) { 
        NOT_IMPLEMENTED_YET
        return NULL;
      } else {
        return rhs;
      }
    }
  }

  lhsName = varSymbLhs->get_name();
  rightLhs = binOp->get_lhs_operand();
  rightRhs = binOp->get_rhs_operand();

  SgVarRefExp * rightLhsRef = isSgVarRefExp(rightLhs);
  SgVarRefExp * rightRhsRef = isSgVarRefExp(rightRhs);

  if(rightLhsRef) {
    if((rightLhsRef->get_symbol())->get_name() == lhsName)
      return rightRhs;
    else if (rightRhsRef)
      if((rightRhsRef->get_symbol())->get_name() == lhsName)
        return rightLhs;
  }
  if(rightRhsRef) {
      if((rightRhsRef->get_symbol())->get_name() == lhsName)
        return rightLhs;
  } else {
    NOT_IMPLEMENTED_YET
    return rhs;
  }
  return NULL;
}

SgExpression * get_stride_from_rhs_expression(SgExpression * binOp) {
  if(DEBUG) DBG_MAQAO
  if(isSgBinaryOp(binOp)) 
    return get_stride_from_rhs_expression(isSgBinaryOp(binOp)->get_lhs_operand(), isSgBinaryOp(binOp)->get_rhs_operand());
  return NULL;
}

SgExpression * get_unroll_stride(SgExpression * increment, size_t unrollSize) {
  if(DEBUG) DBG_MAQAO
  SgExpression * lhs, * rhs, *newRhs, * newIncrement = NULL, * stride;
  SgBinaryOp * binOp;
  SgUnaryOp * unaOp;
  //for ( ; ; func()) case not implemented
  if(isSgCallExpression(increment) || !increment) { 
    std::cout << "The case with function as increment is not implemented yet\n";  
    return NULL;
  }

  if(isSgBinaryOp(increment)) {
    binOp = isSgBinaryOp(increment);
    lhs = binOp->get_lhs_operand();
    rhs = binOp->get_rhs_operand();
    //for ( ; ; i = func()) case not implemented
    if(isSgCallExpression(rhs)) {
      std::cout << "The case with a function affected to the increment is not implemented yet\n"; 
      return NULL;
    }
  }
  else if(isSgUnaryOp(increment)) {
    unaOp = isSgUnaryOp(increment);
    lhs = unaOp->get_operand();
  }

  switch(increment->variantT()) {
    //Binary expressions
    // i = Expr | i = 1 + i
    case V_SgAssignOp : {
      //stride = getNewRhsExpression(lhs,rhs);
      stride = get_stride_from_rhs_expression(lhs,rhs);
      SgIntVal * iv = isSgIntVal(stride);
      if (iv) {
        newRhs = SageBuilder::buildIntVal(iv->get_value()*unrollSize);
      }else {
        SgIntVal * unrollSizeValue = SageBuilder::buildIntVal(unrollSize);
        newRhs = SageBuilder::buildAddOp(SageBuilder::buildMultiplyOp(stride,unrollSizeValue),lhs);
      }
      switch(rhs->variantT()){
        case V_SgAddOp:
          newIncrement = SageBuilder::buildPlusAssignOp(lhs,newRhs);
          break;
        case V_SgSubtractOp:
          newIncrement = SageBuilder::buildMinusAssignOp(lhs,newRhs);
          break;
        case V_SgMultiplyOp:
          newIncrement = SageBuilder::buildMultAssignOp(lhs,newRhs);
          break;
        case V_SgDivideOp:
          NOT_IMPLEMENTED_YET
          break;
        default :
          NOT_IMPLEMENTED_YET
          break;
      }
      break;
    }
    //+=
    case V_SgPlusAssignOp : {
      stride = binOp->get_rhs_operand();
      SgIntVal * iv = isSgIntVal(stride);
      if (iv) {
        newRhs = SageBuilder::buildIntVal(iv->get_value()*unrollSize);
      }else {
        SgIntVal * unrollSizeValue = SageBuilder::buildIntVal(unrollSize);
        newRhs = SageBuilder::buildMultiplyOp(stride,unrollSizeValue);
      }
      newIncrement = SageBuilder::buildPlusAssignOp(lhs,newRhs);
      break;
    }
    //-=
    case V_SgMinusAssignOp: {
      stride = binOp->get_rhs_operand();
      SgIntVal * iv = isSgIntVal(stride);
      if (iv) {
        newRhs = SageBuilder::buildIntVal(-(iv->get_value()*unrollSize));
      } else {
       SgIntVal * unrollSizeValue = SageBuilder::buildIntVal(-unrollSize);
       newRhs = SageBuilder::buildMultiplyOp(stride,unrollSizeValue);
      }
      newIncrement = SageBuilder::buildMinusAssignOp(lhs,newRhs);
      break;
    }
    //*=
    case V_SgMultAssignOp : {
      stride = binOp->get_rhs_operand();
      SgIntVal * iv = isSgIntVal(stride);
      if (iv) {
        newRhs = SageBuilder::buildIntVal(iv->get_value()*unrollSize);
      }else {
        newRhs = SageBuilder::buildMultiplyOp(stride,lhs);
      }
      newIncrement = SageBuilder::buildMultAssignOp(lhs,newRhs);
      break;
    }
    ///=
    case V_SgDivAssignOp : {
      stride = binOp->get_rhs_operand();
      SgIntVal * iv = isSgIntVal(stride);
      if (iv) {
        newRhs = SageBuilder::buildIntVal(iv->get_value()*unrollSize);
      }else {
        newRhs = SageBuilder::buildDivideOp(stride,lhs);
      }
      newIncrement = SageBuilder::buildDivAssignOp(lhs,newRhs);
      break;
    }
    //<<=
    case V_SgLshiftAssignOp : {
      stride = binOp->get_rhs_operand();
      SgIntVal * iv = isSgIntVal(stride);
      if (iv) {
        newRhs = SageBuilder::buildIntVal(iv->get_value()*unrollSize);
      }else {
        newRhs = SageBuilder::buildLshiftOp(stride,lhs);
      }
      newIncrement = SageBuilder::buildLshiftAssignOp(lhs,newRhs);
      break;
    }
    //>>=
    case V_SgRshiftAssignOp : {
      stride = binOp->get_rhs_operand();
      SgIntVal * iv = isSgIntVal(stride);
      if (iv) {
        newRhs = SageBuilder::buildIntVal(iv->get_value()*unrollSize);
      }else {
        newRhs = SageBuilder::buildRshiftOp(stride,lhs);
      }        
      newIncrement = SageBuilder::buildRshiftAssignOp(lhs,newRhs);
      break;
    }
   //Unary expressions
    // i++
    case V_SgPlusPlusOp : {
      newRhs = SageBuilder::buildIntVal(unrollSize);
      newIncrement = SageBuilder::buildPlusAssignOp(lhs,newRhs);
      break;
    }
    // i--
    case V_SgMinusMinusOp : {

      newRhs = SageBuilder::buildIntVal(unrollSize);
    
      newIncrement = SageBuilder::buildMinusAssignOp(lhs,newRhs);
      break;
    }
    default: 
      NOT_IMPLEMENTED_YET
  }
  //std::cout << "Avant : " << increment->unparseToString() << " | Après : " << newIncrement->unparseToString() << std::endl;
  //Return the new increment which is the stride
  return newIncrement;
}

SgExpression * get_unroll_cond(SgForStatement * loop, size_t unrollSize) {
  if(DEBUG) DBG_MAQAO
  SgExpression * testExpr;
  SgExpression * newTest = NULL;
  testExpr = loop->get_test_expr();

  SgBinaryOp * bin_test = isSgBinaryOp(testExpr);
  
  //if is an unary expression like !i
  if(!bin_test) {
    NOT_IMPLEMENTED_YET
    return NULL;
  }

  SgExpression * lhs, * rhs;
  lhs = bin_test->get_lhs_operand();
  rhs = bin_test->get_rhs_operand();
  
  switch(testExpr->variantT()) {
    //Binaries cases
    case V_SgGreaterThanOp : {
      newTest = SageBuilder::buildGreaterThanOp(lhs,SageBuilder::buildAddOp (rhs,SageBuilder::buildIntVal(unrollSize)));
      break;
    }
    case V_SgGreaterOrEqualOp : {
      newTest = SageBuilder::buildGreaterOrEqualOp(lhs,SageBuilder::buildAddOp (rhs,SageBuilder::buildIntVal(unrollSize)));
      break;
    }
    case V_SgLessOrEqualOp : {
      newTest = SageBuilder::buildLessOrEqualOp(lhs,SageBuilder::buildSubtractOp (rhs,SageBuilder::buildIntVal(unrollSize)));
      break;
    }
    case V_SgLessThanOp : {
      newTest = SageBuilder::buildLessThanOp(lhs,SageBuilder::buildSubtractOp (rhs,SageBuilder::buildIntVal(unrollSize)));
      break;
    }
    case V_SgAndOp : 
    case V_SgBitAndOp : 
    case V_SgBitOrOp :
    case V_SgBitXorOp : 
    case V_SgEqualityOp : 
    case V_SgNotEqualOp :
    case V_SgOrOp :
    default : {
      NOT_IMPLEMENTED_YET 
      return NULL;
    }
  }
  return newTest;
}

SgForStatement * myBuildForStatement(SgStatement* initialize_stmt, SgStatement * test, SgExpression * increment, SgStatement * loop_body) {
  if(DEBUG) DBG_MAQAO
  SgForStatement * result = new SgForStatement(test, increment, loop_body);
  ROSE_ASSERT(result);

  SageInterface::setOneSourcePositionForTransformation(result);
  if (test)      test->set_parent(result);
  if (loop_body) loop_body->set_parent(result);
  if (increment) increment->set_parent(result);
  // if (else_body) else_body->set_parent(result);

  result->set_else_body(NULL);
  if (SgForInitStatement* for_init_stmt = isSgForInitStatement(initialize_stmt)) {
    if (result->get_for_init_stmt() != NULL) {
      delete result->get_for_init_stmt();
      result->set_for_init_stmt(NULL);
    }

    result->set_for_init_stmt(for_init_stmt);
    for_init_stmt->set_parent(result);
    return result;
  }

  SgForInitStatement* init_stmt = new SgForInitStatement();
  ROSE_ASSERT(init_stmt);
  SageInterface::setOneSourcePositionForTransformation(init_stmt);

  if (result->get_for_init_stmt() != NULL) {
    delete result->get_for_init_stmt();
    result->set_for_init_stmt(NULL);
  }

  result->set_for_init_stmt(init_stmt);
  init_stmt->set_parent(result);

  if (initialize_stmt) {
    init_stmt->append_init_stmt(initialize_stmt);
    // Support for "for (int i=0; )"
    // The symbols are inserted into the symbol table attached to SgForStatement
    if (isSgVariableDeclaration(initialize_stmt)) {
      SageInterface::fixVariableDeclaration(isSgVariableDeclaration(initialize_stmt),result);
      // fix varRefExp to the index variable used in increment, conditional expressions
      SageInterface::fixVariableReferences(result);
    }
  }
  return result;
}

//////////////////////////////////////////
//                                      //
//                API                   //
//                                      //
//////////////////////////////////////////

Loop* get_loop_from_line(ASTRoot* ast, int lstart, int lend/* = -1*/, int delta/*=0*/) {
  if(DEBUG) DBG_MAQAO
  Loop * loop = ast->get_loop_from_line(lstart,lend);
  if(loop) {
    return loop;
  } else {
    // - dlt
    int dlt = 1;
    // if(!loop)
    while(dlt <= delta) {
      loop = ast->get_loop_from_line(lstart-dlt, lend+dlt);
      if(loop) {
        //If both are clsed or at least one of two is correct
        if(((loop->get_line_start() >= lstart-dlt || loop->get_line_start() <= lstart+dlt) 
          && (loop->get_line_end() >= lend-dlt || loop->get_line_end() <= lend+dlt))
          || ((loop->get_line_start() == lstart) || loop->get_line_end() == lend))
        {
          return loop;
        }
      }
      dlt++;
    }
    // + dlt
    dlt = 1;
    if(!loop)
      while(dlt <= delta) {
        loop = ast->get_loop_from_line(lstart+dlt,lend-dlt);
        if(loop) {
          //If both are clsed or at least one of two is correct
          if(((loop->get_line_start() > lstart-dlt || loop->get_line_start() < lstart+dlt) 
              && (loop->get_line_end() > lend-dlt || loop->get_line_end() < lend+dlt))
            || ((loop->get_line_start() == lstart) || loop->get_line_end() == lend))
          {
            return loop;
          }
        }
        dlt++;
      }
  }
  return NULL;
}

Loop* get_loop_from_label(ASTRoot* ast, std::string label) {
  if(DEBUG) DBG_MAQAO
  if(Loop * loop = ast->get_loop_from_label(label)) {
    return loop;
  } else {
    return NULL;
  }
}
