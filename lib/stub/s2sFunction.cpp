#include "s2sFunction.hpp"
#include "fortran_support.h"
#include <sstream>
#include <algorithm>

//////////////////////////////////////////
//                                      //
//             ASTFunction              //
//                                      //
//////////////////////////////////////////
//Constructor
ASTFunction::ASTFunction(SgSourceFile* file) {
  if (DEBUG) DBG_MAQAO
  identifierFunctions = 0;
  // id = identifierFunctions++;
  root = file->get_globalScope ();
  SgDeclarationStatementPtrList& declList = root->get_declarations ();

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
        Function * new_func  = new Function(defn);
        internalFunctionsList.push_back(new_func);
      }
    } else {
      SgFunctionDeclaration *func = isSgFunctionDeclaration(*it);
      if (func == NULL)
        continue;
      SgFunctionDefinition *defn = func->get_definition();
      if (defn == NULL)
        continue;
      Function * new_func  = new Function(defn);
      internalFunctionsList.push_back(new_func);
    }
  }
}

ASTFunction::ASTFunction(SgBasicBlock* scope) {
  if (DEBUG) DBG_MAQAO
  if(TransformationSupport::getSourceFile(scope)) 
    ASTFunction(TransformationSupport::getSourceFile(scope));
  else {
    std::cout << "Source file was not found we can't create the AST." << std::endl;
  }
}

ASTFunction::ASTFunction(SgGlobal* root):root(root) {
  if (DEBUG) DBG_MAQAO
  // std::cout << root->unparseToString() << std::endl;
  SgDeclarationStatementPtrList& declList = root->get_declarations ();

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
        Function * new_func  = new Function(defn);
        internalFunctionsList.push_back(new_func);
      }
    } else {
      SgFunctionDeclaration *func = isSgFunctionDeclaration(*it);
      if (func == NULL)
        continue;
      SgFunctionDefinition *defn = func->get_definition();
      if (defn == NULL)
        continue;
      Function * new_func  = new Function(defn);
      internalFunctionsList.push_back(new_func);
    }
  }
}

//Destructor
ASTFunction::~ASTFunction() {
  if (DEBUG) DBG_MAQAO
  root = NULL;
  for(int i=0; i < internalFunctionsList.size(); ++i) {
    if(internalFunctionsList[i]) {
      internalFunctionsList[i] = NULL;
      delete(internalFunctionsList[i]);
    }
  }
}

//Accessors
std::vector<Function*> ASTFunction::get_internalFunctionsList() {
  if (DEBUG) DBG_MAQAO 
  return internalFunctionsList; 
}

void ASTFunction::set_internalFunctionsList(std::vector<Function*> vectorList) {
  if (DEBUG) DBG_MAQAO 
  int i;
  for(i=0; i < vectorList.size(); i++) {
    internalFunctionsList[i] = vectorList[i];
  }
  for (int j = internalFunctionsList.size(); j > i; --j) {
    internalFunctionsList.pop_back();
  }
}
    
//Miscellaneous
void ASTFunction::printASTLines() {
  if (DEBUG) DBG_MAQAO
  for(int i=0; i < internalFunctionsList.size(); ++i) {
    internalFunctionsList[i]->printLines();
  }
  std::cout << std::endl;
}

void ASTFunction::print_function_not_found() {
  if (DEBUG) DBG_MAQAO
  for(int i=0; i < internalFunctionsList.size(); ++i) {
    internalFunctionsList[i]->print_function_not_found();
  }
}

void ASTFunction::print() {
  if (DEBUG) DBG_MAQAO
  std::cout << "file = " << root->get_file_info()->get_filenameString() << std::endl;

  for(int i=0; i < internalFunctionsList.size(); ++i) {
    internalFunctionsList[i]->print();
  }
}

void ASTFunction::print_names() {
  if (DEBUG) DBG_MAQAO
  for(int i=0; i < internalFunctionsList.size(); ++i) {
    std::cout << internalFunctionsList[i]->get_name() << std::endl;
  }
}

Function* ASTFunction::get_function_from_line(int start,int end/*=-1*/) {
  if (DEBUG) DBG_MAQAO

  for(int i=0; i < internalFunctionsList.size(); ++i) {
    if ( internalFunctionsList[i]->get_line_start() == start 
      || internalFunctionsList[i]->get_line_end() == end ) 
    {
      return internalFunctionsList[i];
    }
  }

  return NULL;
}

Function* ASTFunction::get_func_from_name(std::string name) {
  if (DEBUG) DBG_MAQAO

  for(int i=0; i < internalFunctionsList.size(); ++i) {
    if ( internalFunctionsList[i]->get_name() == name) 
    {
      return internalFunctionsList[i];
    }
  }

  return NULL;
}

void ASTFunction::apply_directive() {
  if (DEBUG) DBG_MAQAO
  for(int i=0; i < internalFunctionsList.size(); ++i) {
    internalFunctionsList[i]->apply_directive();
  }
}

bool ASTFunction::empty() {
  if (DEBUG) DBG_MAQAO
  return internalFunctionsList.empty();
}

//////////////////////////////////////////
//                                      //
//              Function                //
//                                      //
//////////////////////////////////////////
//Function::Constructors
Function::Function(SgFunctionDefinition* f): function(f) {
  if (DEBUG) DBG_MAQAO

  if (SgStatement* body = f->get_body()) {
    body_without_trans = isSgBasicBlock(SageInterface::deepCopy(body));
    body_without_trans->set_parent(function);
  } else {
    body_without_trans = NULL;
  }

  if( SgProcedureHeaderStatement* procedureHeader = isSgProcedureHeaderStatement(f)) {
    if (procedureHeader->isFunction() == true) {
      set_functionType(FUNCTION);
    } else if (procedureHeader->isSubroutine() == true) {
      set_functionType(SUBROUTINE);
    }else {
      set_functionType(UNKNOWN);
    }
  }else {
    if (DEBUG) DBG_MAQAO
  }
  is_matching = false;
}
    
Function::~Function() {
  if (DEBUG) DBG_MAQAO
  // SageInterface::removeStatement(body_without_trans);
}

//Function::ACCESSORS
SgFunctionDefinition* Function::get_function() {
  if (DEBUG) DBG_MAQAO 
  return function;
}

SgFunctionDefinition* Function::get_function() const {
  if (DEBUG) DBG_MAQAO 
  return function; 
}

void Function::set_function(SgFunctionDefinition* f) {
  if (DEBUG) DBG_MAQAO
  function = f;

  SgProcedureHeaderStatement* procedureHeader = isSgProcedureHeaderStatement(f);
  if (procedureHeader->isFunction() == true) {
    set_functionType(FUNCTION);
  } else if (procedureHeader->isSubroutine() == true) {
    set_functionType(SUBROUTINE);
  }else {
    set_functionType(UNKNOWN);
  }
  is_matching = false;
}

size_t Function::get_id() {return id;}
size_t Function::get_id() const {return id;}
void   Function::set_id(size_t ID) { id = ID; }

std::string Function::get_name() const { return function->get_declaration ()->get_name().getString(); }
std::string Function::get_file_name() const {return function->get_file_info()->get_filenameString();}

Function::functionType_ Function::get_functionType() { return typeOfFunction; }
Function::functionType_ Function::get_functionType() const { return typeOfFunction; }
void Function::set_functionType(Function::functionType_ ft){ typeOfFunction = ft; }
void Function::set_functionType(std::string functionKeyword){
  if(functionKeyword == "SUBROUTINE") typeOfFunction = SUBROUTINE;
  else if (functionKeyword == "FUNCTION" )   typeOfFunction = FUNCTION;
  else typeOfFunction = UNKNOWN;
}
    
bool Function::is_matching_with_bin_loop() { return is_matching;}
void Function::set_matching_with_bin_loop(bool aff) { is_matching = aff; }
    
int  Function::get_line_start() { return function->get_startOfConstruct()->get_line(); }
int  Function::get_line_end() { return function->get_endOfConstruct()->get_line(); }

//Function::Prints
/**
 * Print the type of the function (Fortran do; C/C++ "for" ; while ; etc)
 */
std::string Function::print_functionType(){
  if(DEBUG) DBG_MAQAO
  switch(typeOfFunction) {
    case SUBROUTINE : return "SUBROUTINE";
    case FUNCTION :   return "FUNCTION";
    case UNKNOWN :    return "UNKNOWN";
    default :         return "DEFAULT";
  } 
}

void print_stmt_class(SgBasicBlock* body, int indentation) {
  if(DEBUG) DBG_MAQAO
  SgStatementPtrList stmt_list = body->get_statements();
  for (SgStatementPtrList::iterator it = stmt_list.begin(); it != stmt_list.end(); ++it) {
    for(int i=0; i < indentation; i++) std::cout << " ";
    std::cout << (*it)->class_name() << std::endl;
    if(SgForStatement  * loop = isSgForStatement(*it)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body()))
        print_stmt_class(subbody,indentation+2);
    } else if (SgFortranDo * loop = isSgFortranDo(*it)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
        print_stmt_class(subbody,indentation+2);
    } else if (SgDoWhileStmt * loop = isSgDoWhileStmt(*it)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
        print_stmt_class(subbody,indentation+2);
    } else if (SgWhileStmt  * loop = isSgWhileStmt(*it)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
        print_stmt_class(subbody,indentation+2);
    } else if (SgIfStmt * ifstmt = isSgIfStmt(*it)) {
      if(SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ()))
        print_stmt_class(bodytrue,indentation+2);
      if(SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ()))
        print_stmt_class(bodyfalse,indentation+2);
    } else if (SgSwitchStatement* switchStmt = isSgSwitchStatement(*it)) {
        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body()))
        print_stmt_class(subbody,indentation+2);
    } else if (SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(*it)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body()))
        print_stmt_class(subbody,indentation+2);
    } else if (SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(*it)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body()))
        print_stmt_class(subbody,indentation+2);
    } else if (SgBasicBlock* subbody = isSgBasicBlock(*it)) {
        print_stmt_class(subbody,indentation+2);
    }
  }
}

void Function::print_statement_class() {
  if(DEBUG) DBG_MAQAO
  std::cout << "print all statements (Rose) classes for the function " << get_name() << std::endl;
  SgBasicBlock * body = function->get_body ();
  print_stmt_class(body, 0);
}

/**
 * Print the function begining and ending lines.
 */
void Function::printLines() {
  if (DEBUG) DBG_MAQAO
  std::cout << " start " << get_line_start() 
            << " end "   << get_line_end() 
            << std::endl;
}

/**
 * Print information about the function
 */
void Function::print() {
  if (DEBUG) DBG_MAQAO
  std::cout << " id : " << id 
            << " name  : " << get_name() 
            << " start " << get_line_start() 
            << " end " << get_line_end() 
            << std::endl;
}

void Function::print_function_not_found() {
  if (DEBUG) DBG_MAQAO
  if(!is_matching) {
    printLines();
  }
}

//Function::Miscellaneous
void Function::add_directive(std::string directive) {
  if (DEBUG) DBG_MAQAO 
  if (function) {
    std::string pragma ="DIR$ "+directive;
    PreprocessingInfo::DirectiveType dtype=PreprocessingInfo::F90StyleComment;

    SageInterface::attachComment(function, pragma, PreprocessingInfo::before, dtype);
  }
}

bool Function::apply_directive() {
  if (DEBUG) DBG_MAQAO 
  if (s2s_api::isFortranFile(s2s_api::getEnclosingsourceFile_RosePtr(function)->getFileName ())) {
    
    std::vector<std::string> maqaoDir = s2s_api::get_MAQAO_directives(function->get_declaration());

    if (maqaoDir.size() != 0) {

      for (int i=0; i < maqaoDir.size(); i++) {
        int dirPos = maqaoDir[i].find ("!DIR$ MAQAO");

        if( dirPos != std::string::npos) {
          std::string directive = maqaoDir[i].substr(dirPos);

          if (DEBUG) { std::cout << "directive found above " << get_name() << std::endl; }

          bool directive_applied = apply_directive(directive);
          if (!directive_applied) { return false; } 
        }
      }
      s2s_api::remove_MAQAO_directives(function->get_declaration());
    } else { //if dir == 0 or NULL
      if (DEBUG) std::cout << "no directives founded above the function " << get_name() << std::endl;
    }
  } else { //C/C++

    if (VERBOSE > 1 ||  DEBUG) {
      std::cout << "Function name : "<< get_name() << " "; printLines();      
      SgStatement* previousstmt = SageInterface::getPreviousStatement(function);
      if (DEBUG > 1) std::cout << "previous stmt : " << previousstmt->class_name() << std::endl;
      previousstmt = SageInterface::getPreviousStatement(previousstmt);
      if (DEBUG > 1)std::cout << "previous previous stmt : " << previousstmt->class_name() << std::endl;
    }

    int error = 0;
    SgPragmaDeclaration* dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(function));
    if (isSgFunctionDeclaration(SageInterface::getPreviousStatement(function))) {
      dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(SageInterface::getPreviousStatement(function)));
    }
    while (dir) {
      int pos = dir->get_pragma()->get_pragma().find ("MAQAO");
      if(pos != std::string::npos) {
        std::string directive = dir->get_pragma()->get_pragma().substr(pos);
        if (VERBOSE > 1 || DEBUG) { std::cout << "directive found : " << directive << std::endl; }

        bool directive_applied = apply_directive(directive);
        if (directive_applied) {
          SgPragmaDeclaration* dir2 = dir;
          dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
          SageInterface::removeStatement (dir2, true);
        }      
      } else {
        dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
        continue;
      }
    }
  }
}

bool Function::apply_directive(std::string directive) {
  if (DEBUG) DBG_MAQAO 
  if (VERBOSE > 1 || DEBUG) std::cout<< "Directive : " << directive << std::endl;

  int error = 0;
  // SPECIALIZATION TRANSFORMATION
  if(size_t pos = directive.find("SPECIALIZE") != std::string::npos ) {

    size_t posStart = directive.find("(", pos);
    size_t posEnd  = directive.find(")");

    std::vector<variable_spe*> variables_list;

    if ( posStart  != std::string::npos && posEnd != std::string::npos) {
      int i = 0; 
      std::string substring = directive.substr(posStart+1, posEnd); 
              
      // Check all information about the specialization
      do {
        variable_spe* varspe = new variable_spe();
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
        varspe->var_name = strdup(variable.c_str());

        //Avoid space and equals
        while (substring[i] == ' ') ++i;

        //Determine while style of the specialization
        if (substring[i] == '=') {
          //Avoid space and equals
          while (substring[i] == ' ') ++i;
          if (substring[i+1] == '{')  {
            i++;
            varspe->specialization = variable_spe::INTERVAL;
          } else {
            varspe->specialization = variable_spe::EQUALS;
          }
        }
        else if (substring[i] == '<') {
          varspe->specialization = variable_spe::INF;
        }
        else if (substring[i] == '>') {
          varspe->specialization = variable_spe::SUP;
        }
        else if  (substring[i] == '{')  {
          varspe->specialization = variable_spe::INTERVAL;
        }
        else {
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

        if (varspe->specialization == variable_spe::EQUALS) {
          varspe->value = atoi(value.c_str());
        } else if (varspe->specialization == variable_spe::INF) {
          varspe->sup_bound = atoi(value.c_str());
        } else if (varspe->specialization == variable_spe::SUP) {
          varspe->inf_bound = atoi(value.c_str());
        } else if (varspe->specialization == variable_spe::INTERVAL) {
          varspe->inf_bound = atoi(value.c_str())-1;
          value = "";

          while (substring[i] == ' ' || substring[i] == ',' || substring[i] == '}') ++i;

          for( ; i < substring.length() && substring[i] != ' ' && substring[i] != '}'; ++i) {
            value += substring[i];
          }
          varspe->sup_bound = atoi(value.c_str())+1;
        }

        while (substring[i] == ' '|| substring[i] == '}') ++i;
        //varspe->pretty_print();
        variables_list.push_back(varspe);
      } while (substring[i++] ==  ',');
              
      // if no error was detected -> specialize
      if (!error) {
        if (directive.find("SPECIALIZE_WITHOUT_CALL") != std::string::npos) {
          specialize_v2(variables_list, false);
        }
        else {
          specialize_v2(variables_list, true);
        }
      }
    } else {
      std::cout << "The directive \"" << directive << "\" isn't well wrote\nThe transformation will not be applied on the next statement. " << std::endl;
    }
  }
  //PARTIAL DEAD CODE ELIMINATION TRANSFORMATION 
  else if (directive.find("DEADCODE") != std::string::npos ) {
    s2s_api::dead_code_elimination(function->get_body());
  }
  else if (directive.find("PREFETCH") != std::string::npos ) {
    std::string config = "";
    if (directive.find ('=') != std::string::npos) {
      config = directive.substr(directive.find ('=')+1).c_str();
      modify_prefetch(config);
      // break;
    }
    else {
      std::cout << "Warning: Directive is not well written" << std::endl;
    }
  }
  else if (directive.find("ADDLIBCALLMAIN") != std::string::npos ) {
    if(DEBUG) DBG_MAQAO

    vprofLibCallformain = function;
    // Do it at the end
    s2s_api::add_init_call (function, 1, 0);

  }
  else if (directive.find("ADDLIBCALL") != std::string::npos ) {
    std::string varNames = "";
    if (directive.find ('=') != std::string::npos) {
      varNames = directive.substr(directive.find ('=')+1).c_str();
      s2s_api::add_lib_call (function, varNames);
      // break;
    }
    else {
      std::cout << "Warning: Directive is not well written, you wrote "<< directive<< " instead of ADDLIBCALL" << std::endl;
    }
  }
  else {
    std::cout << "Directive unknown, do nothing." << std::endl;
    return false;
  }
  return true;
}

SgExprStatement* build_if_cond(variable_spe* var, SgSymbol* v_symbol, SgScopeStatement* function) {
  if (DEBUG) DBG_MAQAO 
  std::ostringstream convert;
  SgExprStatement * ifcond;

  if(var->specialization == variable_spe::EQUALS) {
    convert << var->value;    
    ifcond = SageBuilder::buildExprStatement(
              s2s_api::myBuildBinOp(
                SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                V_SgEqualityOp, 
                false));
  } else if (var->specialization == variable_spe::INF) {
    convert << var->sup_bound;    
    ifcond = SageBuilder::buildExprStatement(
              s2s_api::myBuildBinOp(
                SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                V_SgLessThanOp, 
                false));
  } else if (var->specialization == variable_spe::SUP) {
    convert << var->inf_bound;  
    ifcond = SageBuilder::buildExprStatement(
              s2s_api::myBuildBinOp(
                SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                V_SgGreaterThanOp, 
                false));
  } else if (var->specialization == variable_spe::INTERVAL) {
      std::ostringstream convert2;

      convert << var->sup_bound;  
      convert2 << var->inf_bound;  
      SgExprStatement * part1 = SageBuilder::buildExprStatement(
                    s2s_api::myBuildBinOp(
                      SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                      SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                      V_SgLessThanOp, 
                      true));
  
      SgExprStatement * part2 = SageBuilder::buildExprStatement(
                    s2s_api::myBuildBinOp(
                       SageBuilder::buildVarRefExp(SgName(convert2.str()),function) , 
                      SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                      V_SgLessThanOp, 
                       true));

      ifcond = SageBuilder::buildExprStatement(
                    s2s_api::myBuildBinOp(part2->get_expression(),part1->get_expression(),V_SgAndOp, false));

  }
  ifcond->set_parent(function);
  // std::cout << "unparse ifcond init : " << ifcond->unparseToString() ;
  convert.clear();
  convert.str("");

  return ifcond;
}

SgExprStatement* build_if_cond_from_ifcond(variable_spe* var, SgSymbol* v_symbol,  SgScopeStatement* function, SgExprStatement* ifcond) {
  if (DEBUG) DBG_MAQAO 
  std::ostringstream convert;
  SgExprStatement * ifcond2;

  if(var->specialization == variable_spe::EQUALS) {
          convert << var->value;  
          ifcond2 = SageBuilder::buildExprStatement(
                    s2s_api::myBuildBinOp(ifcond->get_expression(),
                      s2s_api::myBuildBinOp(
                        SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                        SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                        V_SgEqualityOp, 
                        false),
                      V_SgAndOp, false));  
  } else if (var->specialization == variable_spe::INF) {
          convert << var->sup_bound;    
          ifcond2 = SageBuilder::buildExprStatement(
                      s2s_api::myBuildBinOp(ifcond->get_expression(),
                      s2s_api::myBuildBinOp(
                        SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                        SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                        V_SgLessThanOp, 
                        false),
                      V_SgAndOp, false));  
  } else if (var->specialization == variable_spe::SUP) {
          convert << var->inf_bound;    
          ifcond2 = SageBuilder::buildExprStatement(
                      s2s_api::myBuildBinOp(ifcond->get_expression(),
                      s2s_api::myBuildBinOp(
                        SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                        SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                        V_SgGreaterThanOp, 
                        false),
                      V_SgAndOp, false));  
  } else if (var->specialization == variable_spe::INTERVAL) {
        std::ostringstream convert2;
        convert << var->sup_bound;  
        convert2 << var->inf_bound;  

        //sup_bound :  VAR < sup_bound
        SgExprStatement * part1 = SageBuilder::buildExprStatement(
                      s2s_api::myBuildBinOp(
                        SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                        SageBuilder::buildVarRefExp(SgName(convert.str()),function) , 
                        V_SgLessThanOp, 
                        true));
        //inf_bound :  inf_bound < VAR
        SgExprStatement * part2 = SageBuilder::buildExprStatement(
                      s2s_api::myBuildBinOp(
                        SageBuilder::buildVarRefExp(SgName(convert2.str()),function) , 
                        SageBuilder::buildVarRefExp(v_symbol->get_name(),function) , 
                        V_SgLessThanOp, 
                        true));

        ifcond2 = SageBuilder::buildExprStatement(
                    s2s_api::myBuildBinOp(
                      ifcond->get_expression(),
                      s2s_api::myBuildBinOp(part2->get_expression(),part1->get_expression(),V_SgAndOp, true),
                    V_SgAndOp, false));
  }

  convert.clear();
  convert.str("");

  return ifcond2;
}

//V2.0 if()... return elseif ()... return elseif ()... return endif orig_body
void Function::specialize_v2(std::vector<variable_spe*> var, bool create_call) {
  if (DEBUG) DBG_MAQAO 
  ROSE_ASSERT(var.size() != 0);

  if (!function) { return; }
  SgStatement * original_body = SageInterface::copyStatement(function->get_body());
  SgGlobal * globalScope = SageInterface::getGlobalScope(function);
  SgSymbolTable * st = function->get_symbol_table();
  SgSymbolTable * gst = globalScope->get_symbol_table();
  std::vector<SgSymbol*> v_symbol;
  std::vector<int>       v_val;

  //looking for variable to specialize into symbol tables
  for(int i=0; i < var.size(); i++) {
      SgSymbol* vsym = st->find_variable(std::string(var[i]->var_name));
      if (!vsym) {
        SgBasicBlock * bb = isSgBasicBlock(original_body);
        SgSymbolTable * stbb = bb->get_symbol_table();
        vsym = stbb->find_variable(std::string(var[i]->var_name));
        
        if (!vsym) {
          if(LOG || VERBOSE) {
            std::cout << "The function "<< function->get_declaration()->get_name()
                      << " will not be specialize, because " << std::string(var[i]->var_name)
                      << " is not a parameter of the function or declare in the body of the function"
                      << std::endl;
          }
          return;
        }
      }
      if (!isSgTypeInt(vsym->get_type ())) {
        if(LOG || VERBOSE) {
          std::cout << "The function "<< function->get_declaration()->get_name()
                    << " will not be specialize, because " << std::string(var[i]->var_name)
                    << " is not an INTEGER"
                    << std::endl;
        }
        //We will not operate if isn't a INTEGER
        return;
      } else {
        v_symbol.push_back(vsym);
        if (var[i]->specialization == variable_spe::EQUALS) { v_val.push_back(var[i]->value);}
        else if (var[i]->specialization == variable_spe::INF) { v_val.push_back(var[i]->sup_bound); }
        else if (var[i]->specialization == variable_spe::SUP) { v_val.push_back(var[i]->inf_bound); }
        else if (var[i]->specialization == variable_spe::INTERVAL) {
          
        }
      }
  }
  if (!body_without_trans) {
    if (LOG || VERBOSE) std::cout << "The function "<< function->get_declaration()->get_name()
                    << " will not be specialize, because of the body, impossible to copy it."<< std::endl;
    return;
  }
  //Build the new function
  SgFunctionDeclaration* copy_f_def = s2s_api::build_functionDecl(function->get_declaration(), var, body_without_trans);
  if (!copy_f_def) { return; }
  copy_f_def->set_endOfConstruct(TRANSFORMATION_FILE_INFO);                        
  copy_f_def->get_endOfConstruct()->set_parent(copy_f_def);

  // Specialize
  if (create_call) {
    // Create the If condtion 
    SgExprStatement * ifcond = build_if_cond(var[0], v_symbol[0], function);

    // With all variables to specialize
    for(int i = 1; i < var.size(); ++i) {
      ifcond = build_if_cond_from_ifcond(var[i], v_symbol[i], function, ifcond);
    }

    //***************
    //** TRUE BODY **
    //***************
    //Creation of the CALL stmt
    std::ostringstream convert;
    std::string ending_func_name = "_ASSIST";
    //Reconstruct the name of the function      
    for(int i=0; i < var.size(); ++i) {
        std::string opstring = "";
        if (var[i]->specialization == variable_spe::EQUALS) {
          convert << var[i]->value;
          opstring="e";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
        }
        else if (var[i]->specialization == variable_spe::INF) {
          convert << var[i]->sup_bound;
          opstring="i";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
        }
        else if (var[i]->specialization == variable_spe::SUP) {
          convert << var[i]->inf_bound;
          opstring="s";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
        }
        else if (var[i]->specialization == variable_spe::INTERVAL) {
          convert << (var[i]->inf_bound+1);
          opstring="b";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
          convert.clear();
          convert.str("");
          convert << (var[i]->sup_bound-1);
          ending_func_name += "_"+convert.str();
        }          

        convert.clear();
        convert.str("");
    }

    //The character '-' isn't valide for a function name, (i.e "x=-1")
    std::replace(ending_func_name.begin(), ending_func_name.end(), '-', 'm');

    SgFunctionSymbol  * f_symbol  = gst->find_function(SgName(function->get_declaration()->get_name().getString()+ending_func_name));
    ROSE_ASSERT(f_symbol);

    SgFunctionCallExp * func_call = SageBuilder::buildFunctionCallExp (f_symbol);
    func_call->get_file_info()->set_col(function->get_file_info()->get_col()+2);
        
    //Add arguments to the call (without specified variables)
    bool add_symbol_to_arg;
    SgInitializedNamePtrList& argList =  copy_f_def->get_args ();      
    for(SgInitializedNamePtrList::iterator it_arg = argList.begin(); it_arg != argList.end(); ++it_arg) {
        add_symbol_to_arg= true;
        for(int i=0; i < v_symbol.size(); ++i) {
          if ( (*it_arg)->get_symbol_from_symbol_table()->get_name() == v_symbol[i]->get_name() 
            && var[i]->specialization == variable_spe::EQUALS) {
            add_symbol_to_arg = false;
          }
        }
        if(add_symbol_to_arg)
          func_call->append_arg(SageBuilder::buildVarRefExp ((*it_arg)->get_symbol_from_symbol_table ()->get_name()));
    }

    // Build the return statement
    SgStatement * true_body  = NULL; 
    if (s2s_api::isFortranFile(s2s_api::getEnclosingsourceFile_RosePtr(function)->getFileName ())) {
      true_body  = SageBuilder::buildBasicBlock(
        SageBuilder::buildExprStatement(func_call), 
        SageBuilder::buildReturnStmt (SageBuilder::buildNullExpression ()));
    } else { // C/C++
      true_body  = SageBuilder::buildBasicBlock(
        SageBuilder::buildReturnStmt (func_call));
    }

    // Set the IFSTMT parent / children
    SgIfStmt * ifstmt = SageBuilder::buildIfStmt (ifcond, true_body, NULL);
    ifcond->set_parent(ifstmt);
    true_body->set_parent(ifstmt);
    ifstmt->set_parent(function);
    // if (! ifstmt->get_has_end_statement ()) {
    ifstmt->set_has_end_statement (true);
    // }

    //insert the if stmt after all declarations
    SageInterface::insertStatementAfter (
      SageInterface::findLastDeclarationStatement(isSgScopeStatement(function->get_body())),
      ifstmt, 
      true);

  }

  // Propagate the constant throught the function copied and replace all call the de variable "v_symbol" by the value val
  for(int i = 0; i < v_symbol.size(); ++i) {
    if (var[i]->specialization == variable_spe::EQUALS) {
      s2s_api::constantPropagation(copy_f_def->get_definition()->get_body(), isSgVariableSymbol(v_symbol[i]), var[i]->value);
    }
  }
  // if (LOG) s2s_api::log("In"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+"\nThe function : " + copy_f_def->get_name().getString () + " was specialized.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+".log");
  if (LOG) s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+" f:" + copy_f_def->get_name().getString () + "] specialization - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+".log");

  s2s_api::dead_code_elimination(copy_f_def->get_definition()->get_body(), var);
  if (LOG) s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+" f: " + copy_f_def->get_name().getString ()+ "] DCE - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+".log");
}

bool Function::modify_prefetch(std::string config) {
  if (DEBUG) DBG_MAQAO

  SgGlobal * globalScope = SageInterface::getGlobalScope(function);
  SgSymbolTable * gst = globalScope->get_symbol_table();

  //prefetch is the name of the function
  std::string prefetch_func_name = "prefetch";
  if (s2s_api::isCCXXFile(s2s_api::getEnclosingsourceFile_RosePtr(function)->getFileName ())) {
    prefetch_func_name = "prefetch_";
  }
  
  SgFunctionSymbol * f_symbol  = gst->find_function(SgName(prefetch_func_name));

  SgFunctionDeclaration * prefetch_func_decl = NULL;
  if (! f_symbol) {
    //BUILD SYMBOL
    //BUILD DECLARATION as externat the top of all 
    SgType* voidType = SageBuilder::buildVoidType ();
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    //Next 3 lines is the way Rose decide to declare a char* so we have to do with it
    int stringSize = 6;
    Sg_File_Info* fileInfo = Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode();
    SgIntVal* lengthExpression = new SgIntVal(fileInfo,stringSize,"6");

    paramList->append_arg(SageBuilder::buildInitializedName("prftch",SageBuilder::buildStringType (lengthExpression) ));
    SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();

    prefetch_func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(prefetch_func_name), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    prefetch_func_decl->set_parent(globalScope);
    voidType->set_parent(prefetch_func_decl);
    paramList->set_parent(prefetch_func_decl);
    exprListExp->set_parent(prefetch_func_decl);

    //insert declaration
    SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    SageInterface::insertStatementBefore (firstFuncDecl, prefetch_func_decl, true);
    f_symbol  = gst->find_function(SgName(prefetch_func_name));
    ROSE_ASSERT(f_symbol);
    SageInterface::setExtern(prefetch_func_decl);
  } else {
    // Do nothing ?
  }

  SgFunctionCallExp * func_call = SageBuilder::buildFunctionCallExp (f_symbol); 
  func_call->set_parent(function->get_body());
  SgStringVal * stringVal = SageBuilder::buildStringVal (config);
  func_call->append_arg(stringVal);
  stringVal->set_parent(func_call);

  //insert the if stmt after all declarations
  SageInterface::insertStatementAfter (
      SageInterface::findLastDeclarationStatement(isSgScopeStatement(function->get_body())),
      SageBuilder::buildExprStatement(func_call), 
      true);

  return true;
} 

Function* get_function_from_line(ASTFunction* ast, int lstart, int lend/* = -1*/, int delta/*=0*/) {
  if (DEBUG) DBG_MAQAO 
  Function * func = ast->get_function_from_line(lstart,lend);
  if(func) {
    return func;
  } else {
    // - dlt
    int dlt = 1;
    // if(!func)
    while(dlt <= delta) {
      func = ast->get_function_from_line(lstart-dlt, lend+dlt);
      if(func) {
        //Si les deux sont proches ou au moins l'un des deux est correcte
        if(((func->get_line_start() >= lstart-dlt || func->get_line_start() <= lstart+dlt) 
          && (func->get_line_end() >= lend-dlt || func->get_line_end() <= lend+dlt))
          || ((func->get_line_start() == lstart) || func->get_line_end() == lend))
        {
          return func;
        }
      }
      dlt++;
    }
    // + dlt
    dlt = 1;
    if(!func)
      while(dlt <= delta) {
        func = ast->get_function_from_line(lstart+dlt,lend-dlt);
        if(func) {
          //Si les deux sont proche ou au moins l'un des deux est correcte
          if(((func->get_line_start() > lstart-dlt || func->get_line_start() < lstart+dlt) 
              && (func->get_line_end() > lend-dlt || func->get_line_end() < lend+dlt))
            || ((func->get_line_start() == lstart) || func->get_line_end() == lend))
          {
            return func;
          }
        }
        dlt++;
      }
  }
  return NULL;
}

// Version 1.0 work and create :
// origfunc() {
//  <decls>
//  if(spe==speval ...)
//    origfunc_speespeval...()
//  else
//   if(...)
//    origfuncspev2...()
//    ...
//   else {
//      <orig_body> 
//   }
//}
void Function::specialize(std::vector<variable_spe*> var, bool create_call) {
  if (DEBUG) DBG_MAQAO 
  ROSE_ASSERT(var.size() != 0);

  if (!function) { return; }
  SgStatement * original_body = SageInterface::copyStatement(function->get_body());
  SgGlobal * globalScope = SageInterface::getGlobalScope(function);
  SgSymbolTable * st = function->get_symbol_table();
  SgSymbolTable * gst = globalScope->get_symbol_table();
  std::vector<SgSymbol*> v_symbol;
  std::vector<int>       v_val;

  //looking for variable to specialize into symbol tables
  for(int i=0; i < var.size(); i++) {
      SgSymbol* vsym = st->find_variable(std::string(var[i]->var_name));
      if (!vsym) {
        SgBasicBlock * bb = isSgBasicBlock(original_body);
        SgSymbolTable * stbb = bb->get_symbol_table();
        vsym = stbb->find_variable(std::string(var[i]->var_name));
        
        if (!vsym) {
          if(LOG) {
            std::cout << "The function "<< function->get_declaration()->get_name()
                      << " will not be specialize, because " << std::string(var[i]->var_name)
                      << " is not a parameter of the function or declare in the body of the function"
                      << std::endl;
          }
          return;
        }
      }
      if (!isSgTypeInt(vsym->get_type ())) {
        if(LOG) {
          std::cout << "The function "<< function->get_declaration()->get_name()
                    << " will not be specialize, because " << std::string(var[i]->var_name)
                    << " is not an INTEGER"
                    << std::endl;
        }
        //We will not operate if isn't a INTEGER
        return;
      } else {
        v_symbol.push_back(vsym);
        if (var[i]->specialization == variable_spe::EQUALS) { v_val.push_back(var[i]->value);}
        else if (var[i]->specialization == variable_spe::INF) { v_val.push_back(var[i]->sup_bound); }
        else if (var[i]->specialization == variable_spe::SUP) { v_val.push_back(var[i]->inf_bound); }
        else if (var[i]->specialization == variable_spe::INTERVAL) {
          
        }
      }
  }

  if (body_without_trans) {
    if (LOG) std::cout << "The function "<< function->get_declaration()->get_name()
                    << " will not be specialize, because of the body, impossible to copy it."<< std::endl;
    return;
  }

  //Build the new function
  SgFunctionDeclaration* copy_f_def = s2s_api::build_functionDecl(function->get_declaration(), var, body_without_trans);
  if (!copy_f_def) { return; }
  copy_f_def->set_endOfConstruct(TRANSFORMATION_FILE_INFO);                        
  copy_f_def->get_endOfConstruct()->set_parent(copy_f_def);

  // Specialize
  if (create_call) {
    // Create the If condtion 
    SgExprStatement * ifcond = build_if_cond(var[0], v_symbol[0], function);

    // With all variables to specialize
    for(int i = 1; i < var.size(); ++i) {
      ifcond = build_if_cond_from_ifcond(var[i], v_symbol[i], function, ifcond);
    }

    //****************
    //** FALSE BODY **
    //****************
    SgBasicBlock * false_body = SageBuilder::buildBasicBlock();  
    SgBasicBlock * declaration_and_used_part = SageBuilder::buildBasicBlock(); 
    bool declaration_part = true;
    for (SgStatementPtrList::iterator it = isSgBasicBlock(original_body)->get_statements().begin(); 
        it != isSgBasicBlock(original_body)->get_statements().end(); 
        ++it) 
    {
        if(declaration_part) {
          if(!isSgDeclarationStatement(*it) && !isSgUseStatement(*it)) {
            declaration_part = false;
            SgStatement * stmt = SageInterface::copyStatement(*it);
            stmt->set_parent(false_body);
            SageInterface::appendStatement(stmt,false_body);
          }
          else {
            SgStatement * stmt = SageInterface::copyStatement(*it);
            stmt->set_parent(function);
            SageInterface::appendStatement(stmt,declaration_and_used_part);
          }
        } else {
            SgStatement * stmt = SageInterface::copyStatement(*it);
            stmt->set_parent(false_body);
            SageInterface::appendStatement(stmt,false_body);
        }
    }

    false_body->set_symbol_table(isSgBasicBlock(original_body)->get_symbol_table());
    false_body->get_symbol_table()->set_parent(false_body);

    //***************
    //** TRUE BODY **
    //***************
    //Creation of the CALL stmt
    std::ostringstream convert;
    std::string ending_func_name = "_ASSIST";
    //Reconstruct the name of the function      
    for(int i=0; i < var.size(); ++i) {
        std::string opstring = "";
        if (var[i]->specialization == variable_spe::EQUALS) {
          convert << var[i]->value;
          opstring="e";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
        }
        else if (var[i]->specialization == variable_spe::INF) {
          convert << var[i]->sup_bound;
          opstring="i";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
        }
        else if (var[i]->specialization == variable_spe::SUP) {
          convert << var[i]->inf_bound;
          opstring="s";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
        }
        else if (var[i]->specialization == variable_spe::INTERVAL) {
          convert << (var[i]->inf_bound+1);
          opstring="b";
          ending_func_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
          convert.clear();
          convert.str("");
          convert << (var[i]->sup_bound-1);
          ending_func_name += "_"+convert.str();
        }          

        convert.clear();
        convert.str("");
    }

    //The character '-' isn't valide for a function name, (i.e "x=-1")
    std::replace(ending_func_name.begin(), ending_func_name.end(), '-', 'm');

    if (VERBOSE > 1) std::cout << "Searching : "<< function->get_declaration()->get_name().getString()+ending_func_name << std::endl;
    SgFunctionSymbol * f_symbol  = gst->find_function(SgName(function->get_declaration()->get_name().getString()+ending_func_name));
    ROSE_ASSERT(f_symbol);

    SgFunctionCallExp * func_call = SageBuilder::buildFunctionCallExp (f_symbol);
    func_call->get_file_info()->set_col(function->get_file_info()->get_col()+2);
        
    //Add arguments to the call (without specified variables)
    bool add_symbol_to_arg;
    SgInitializedNamePtrList& argList =  copy_f_def->get_args ();      
    for(SgInitializedNamePtrList::iterator it_arg = argList.begin(); it_arg != argList.end(); ++it_arg) {
        add_symbol_to_arg= true;
        for(int i=0; i < v_symbol.size(); ++i) {
          if ( (*it_arg)->get_symbol_from_symbol_table()->get_name() == v_symbol[i]->get_name() 
            && var[i]->specialization == variable_spe::EQUALS) {
            add_symbol_to_arg = false;
          }
        }
        if(add_symbol_to_arg)
          func_call->append_arg(SageBuilder::buildVarRefExp ((*it_arg)->get_symbol_from_symbol_table ()->get_name()));
    }

    SgStatement * true_body  = SageBuilder::buildBasicBlock(SageBuilder::buildExprStatement(func_call));

    // Set the IFSTMT parent / children
    SgIfStmt * ifstmt = SageBuilder::buildIfStmt (ifcond, true_body, false_body);
    ifcond->set_parent(ifstmt);
    true_body->set_parent(ifstmt);
    false_body->set_parent(ifstmt);

    // Remove the original body to replace it by the new one
    function->remove_statement(function->get_body());
    //Replace the body by the new if stmt 
    function->set_body(SageBuilder::buildBasicBlock(ifstmt));

    //insert used statement and declarations statements
    SageInterface::insertStatementList (ifstmt, declaration_and_used_part->get_statements(), true);

    // Add a call statement if the variable is equals to val
    ifstmt->set_parent(function);
  }
  // Propagate the constant throught the function copied and replace all call the de variable "v_symbol" by the value val
  for(int i = 0; i < v_symbol.size(); ++i) {
    if (var[i]->specialization == variable_spe::EQUALS) {
      s2s_api::constantPropagation(copy_f_def->get_definition()->get_body(), isSgVariableSymbol(v_symbol[i]), var[i]->value);
    }
  }
  // if (LOG) s2s_api::log("In"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+"\nThe function : " + copy_f_def->get_name().getString () + " was specialized.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+".log");
  if (LOG) s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+" f:" + copy_f_def->get_name().getString () + "] specialization - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+".log");

  s2s_api::dead_code_elimination(copy_f_def->get_definition()->get_body(), var);
  // if (LOG) s2s_api::log("In"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+"\nDead code elimination on : " + copy_f_def->get_name().getString (), "log_"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+".log");
  if (LOG) s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+" f: " + copy_f_def->get_name().getString ()+ "] DCE - success.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithoutPath ()+".log");
}
