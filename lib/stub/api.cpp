#include "api.hpp"
#include <fstream>
 
//////////////////////////////////////////
//                                      //
//                API                   //
//                                      //
//////////////////////////////////////////
namespace s2s_api {
SgProject* load_project(std::vector<std::string> arg) {
  if(DEBUG) DBG_MAQAO
  std::vector<std::string> argv;
  // Add all arguments for Rose
  argv.push_back("s2s");
  argv.push_back("-rose:collectAllCommentsAndDirectives ");
  argv.push_back("-rose:skip_syntax_check");
  argv.push_back("-rose:skipfinalCompileStep");
  argv.push_back("-c"); // we not compile the file 
  argv.push_back("-cpp"); // we do not want to preprocess the files
  //argv.push_back("-w"); // no influence; warning are still here but we leave it if they correct this into EDG
  argv.push_back("-std=c++0x"); // 
  argv.push_back("-std=gnu++0x"); // 
  //argv.push_back("-I/usr/lib/gcc/x86_64-linux-gnu/4.4/include"); // To load gcc4.4 includes for c files

  //Add all option put in arg
  for (int i=0; i < arg.size(); i++) {
    argv.push_back(arg[i]);
  }
  return frontend(argv);
}

SgSourceFile * getEnclosingsourceFile_RosePtr( const SgNode* astNode ) {
  if(DEBUG) DBG_MAQAO

  SgSourceFile* temp = SageInterface::getEnclosingNode<SgSourceFile>(astNode,true);
  if (temp) {
    return temp;
  }
  else {
    std::cout << "WARNING : SOURCE FILE NOT FOUND !" << std::endl;
    return NULL;
  }
}

//////////////////////
//   Check files    //
//////////////////////
bool isFortranFile(std::string f) {
  if(DEBUG) DBG_MAQAO
  // Check all extention of the file
  if (f.find (".f77") != std::string::npos 
      || f.find (".f90") != std::string::npos
      || f.find (".f03") != std::string::npos
      || f.find (".f") != std::string::npos
      || f.find (".F") != std::string::npos
      || f.find (".F77") != std::string::npos
      || f.find (".F90") != std::string::npos
      || f.find (".F03") != std::string::npos) {
    return true; 
  }
  return false;
}

bool isCCXXFile(std::string f) {
  if(DEBUG) DBG_MAQAO
  // Check all extention of the file
  if (f.find (".cpp") != std::string::npos 
      || f.find (".hpp") != std::string::npos
      || f.find (".cc") != std::string::npos
      || f.find (".hh") != std::string::npos
      || f.find (".C") != std::string::npos
      || f.find (".H") != std::string::npos
      || f.find (".CPP") != std::string::npos
      || f.find (".HPP") != std::string::npos
      || f.find (".cxx") != std::string::npos
      || f.find (".hxx") != std::string::npos
      || f.find (".c++") != std::string::npos
      || f.find (".h++") != std::string::npos
      || f.find (".c") != std::string::npos
      || f.find (".h") != std::string::npos)
    return true; 

  return false;
}

bool isCCXXHeader(std::string f) {
  if(DEBUG) DBG_MAQAO
  // Check all extention of the file
  if (f.find ("hpp") != std::string::npos 
      || f.find (".hh") != std::string::npos
      || f.find (".H") != std::string::npos
      || f.find (".HPP") != std::string::npos
      || f.find (".hxx") != std::string::npos
      || f.find (".h++") != std::string::npos
      || f.find (".h") != std::string::npos)
    return true; 

  return false;
}

SgFunctionDefinition * get_function_RosePtr(SgNode* astNode) {
  if(DEBUG) DBG_MAQAO
  const SgNode* parentNode = astNode;

  // if(DEBUG) std::cout << "Parent is " << parentNode->class_name() << std::endl;
  while ( (isSgFunctionDefinition(parentNode) == NULL) && (parentNode->get_parent() != NULL) ) {
    parentNode = parentNode->get_parent();
    // if(DEBUG) std::cout << "Parent is " << parentNode->class_name() << std::endl;
  }
  if (const SgFunctionDefinition* func = isSgFunctionDefinition(parentNode)) {
    return const_cast<SgFunctionDefinition*>(func);
  } else {
    return NULL; 
  } 
}

///////////////////////////
//  Directives functions //
///////////////////////////
std::vector<std::string> get_comment_and_directives(SgStatement * astNode) {
  if(DEBUG) DBG_MAQAO
  AttachedPreprocessingInfoType *ppInfo = isSgLocatedNode(astNode)->getAttachedPreprocessingInfo();
  AttachedPreprocessingInfoType::iterator i;
  std::vector<std::string> directive_list;

  if (ppInfo) {
    for (i = ppInfo->begin(); i != ppInfo->end(); ++i) {
      directive_list.push_back((*i)->getString());
    }
    // Directives for C/C++ are not handle as comments but as pragmas
    if (currentFileType == "c" || currentFileType == "cpp") {
      SgPragmaDeclaration* dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(isSgStatement(astNode)));
      while (dir) {
        directive_list.push_back(dir->get_pragma()->get_pragma());
        dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
      }
    }
  }
  return directive_list;
}

std::vector<std::string> get_directives(SgStatement * astNode) {
  if(DEBUG) DBG_MAQAO
  std::vector<std::string> directive_list;
  if (currentFileType == "fortran") {
    AttachedPreprocessingInfoType *ppInfo = isSgLocatedNode(astNode)->getAttachedPreprocessingInfo();
    AttachedPreprocessingInfoType::iterator i;
    if (!ppInfo) return directive_list;

    for (i = ppInfo->begin(); i != ppInfo->end(); ++i) {
      if ((*i)->getString().find ("!DIR$") != std::string::npos) {
        directive_list.push_back((*i)->getString());
      }
    }

  } else { 
    SgPragmaDeclaration* dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(isSgStatement(astNode)));
    while (dir) {
      directive_list.push_back(dir->get_pragma()->get_pragma());
      dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
    }
  }
  return directive_list;
}

bool copy_directives(SgStatement * from, SgStatement * to) {
  if(DEBUG) DBG_MAQAO
  std::vector<std::string> directives = get_directives(from);  
  for (int i=0; i < directives.size(); i++) {
    add_directive(to, directives[i], false);
  }

  return true;
}

std::vector<std::string> get_MAQAO_directives(SgStatement * astNode) {
  if (DEBUG) DBG_MAQAO

  if (!astNode) std::cout << "astNode is null" << std::endl;
  if (DEBUG) std::cout << "Looking for directives above " << astNode->class_name() << std::endl;
  std::vector<std::string> directive_list;

  if (currentFileType == "fortran") {
    AttachedPreprocessingInfoType *ppInfo = isSgLocatedNode(astNode)->getAttachedPreprocessingInfo();
    AttachedPreprocessingInfoType::iterator i;
    if (ppInfo) {
      for (i = ppInfo->begin(); i != ppInfo->end(); ++i) {
        if ((*i)->getString().find ("!DIR$ MAQAO") != std::string::npos) {
        if (DEBUG) std::cout << "found : " << (*i)->getString() << std::endl;
          directive_list.push_back((*i)->getString());
        }
      }
    }
  } else { 
    SgPragmaDeclaration* dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(isSgStatement(astNode)));
    while (dir) {
      int pos = dir->get_pragma()->get_pragma().find ("MAQAO ");
      if(pos != std::string::npos) {
        if (DEBUG) std::cout << "found : " << dir->get_pragma()->get_pragma().substr(pos) << std::endl;
        directive_list.push_back(dir->get_pragma()->get_pragma().substr(pos));
      }
      dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
    }
  }
  return directive_list;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiters) {
  std::vector <std::string> tokens;
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
  return tokens;
}

// TODO : check if we are in a specialized basic block (loop for example) 
// Used to dertermine if we have to erase the "IF_SPE" when the specialization is done
bool if_in_specialized_block (std::string varname, std::string compareType) {
  if(DEBUG) DBG_MAQAO
  return false; 
}

int  remove_MAQAO_directives(SgStatement * astNode, std::string dirToRemove/*=""*/) {
  if(DEBUG) DBG_MAQAO
  int nb_removed = 0;
  if (currentFileType == "fortran") {
    AttachedPreprocessingInfoType *ppInfo = isSgLocatedNode(astNode)->getAttachedPreprocessingInfo();
    AttachedPreprocessingInfoType::iterator it;

    if (ppInfo) {
      for (it = ppInfo->begin(); it != ppInfo->end();) {
        if ( (*it)->getString().find ("!DIR$ MAQAO") != std::string::npos 
           &&(*it)->getString().find (dirToRemove) != std::string::npos) 
        {
          if (DEBUG) std::cout  << "Remove :" << (*it)->getString() << std::endl;
          it = ppInfo->erase(it);
          nb_removed++;
        } else {
          it++;
        }
      }
    }
  } else { 
    SgPragmaDeclaration* dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(isSgStatement(astNode)));
    while (dir) {
      if(dir->get_pragma()->get_pragma().find ("MAQAO ") != std::string::npos 
          && dir->get_pragma()->get_pragma().find (dirToRemove) != std::string::npos) 
      {
        SgPragmaDeclaration* dir2 = dir;
        dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
        if (DEBUG) std::cout  << "Remove :" << dir2->get_pragma()->get_pragma() << std::endl;
        SageInterface::removeStatement (dir2, true);
        nb_removed++;
      } else {
        dir = isSgPragmaDeclaration(SageInterface::getPreviousStatement(dir));
      }
    }
  }
  return nb_removed;
}

void add_directive(SgStatement* stmt, std::string directive, bool nodoublon/*=true*/) {
  if(DEBUG) DBG_MAQAO
  if (!stmt) return; 
  if (SageInterface::is_Fortran_language ()) {
    std::string pragma ="DIR$ "+directive;
    PreprocessingInfo::DirectiveType dtype=PreprocessingInfo::F90StyleComment;
    
    //Check if the comment already exist to avoid to write twice a same directive
    if (nodoublon) {
      std::string pragma_to_compare = "!"+pragma;
      std::vector<std::string> current_comments = get_comment_and_directives(stmt);
      for (int i= 0; i< current_comments.size(); i++) {
        if (current_comments[i] == pragma_to_compare) { 
          if (DEBUG) { std::cout << "Directive " << pragma << " already exist on the statement." << std::endl;}
          return; 
        }
      }
    }
    //Attach the comment before the statement "stmt"
    SageInterface::attachComment(stmt, pragma, PreprocessingInfo::before, dtype);
  } else if (SageInterface::is_Cxx_language () || SageInterface::is_C_language ()) {
    //To avoid to write twice a same directive
    if (nodoublon) {
      std::vector<std::string> current_comments = get_comment_and_directives(stmt);
      for (int i= 0; i< current_comments.size(); i++) {
        if (current_comments[i] == directive) { 
          if (DEBUG) { std::cout << "Directive " << directive << " already exist on the statement." << std::endl;}
          return; 
        }
      }
    }
    SgPragmaDeclaration* pragma = SageBuilder::buildPragmaDeclaration(directive, stmt->get_scope ());
    SageInterface::insertStatementBefore(stmt, pragma, true);
    pragma->set_parent(stmt->get_parent());
    if (DEBUG) std::cout << "Insert : " << directive << std::endl;
    // PreprocessingInfo::DirectiveType dtype=PreprocessingInfo::F90StyleComment;
    // SageInterface::attachComment(stmt, pragma, PreprocessingInfo::before, dtype);
  
  }
}

bool isHeaderAlreadyExist (SgStatement* stmt, std::string header) {
  if(DEBUG) DBG_MAQAO
  if (SgGlobal* globalScope = SageInterface::getGlobalScope(stmt)) {
    bool find = false;
    SgDeclarationStatementPtrList & glbList = globalScope->get_declarations ();
    for (int i=0; i < glbList.size(); i++) {
      //must have this judgement, otherwise wrong file will be modified!
      //It could also be the transformation generated statements with #include attached
      if ((glbList[i]->get_file_info ())->isSameFile(globalScope->get_file_info ())
       || (glbList[i]->get_file_info ())->isTransformation()) 
      {
        //print preproc info
        AttachedPreprocessingInfoType *comments = glbList[i]->getAttachedPreprocessingInfo ();
        AttachedPreprocessingInfoType::iterator i;
        for (i = comments->begin (); i != comments->end (); i++)
        {
          if ((*i)->getString ().substr(0, (*i)->getString ().size()-1) == header) {
            // std::cout << "directive to remove : " << (*i)->getString ().c_str () << std::endl;
            return true;
          }
        }
        //Stop because we just want the first declaration to had include stmt
        break;
      }
    }
  }
  return false;
}

//////////////////////
//  PRINT Function  //
//////////////////////
/**
 * DEBUG function
 */
void printAllStmt(SgGlobal *root) {
  if(DEBUG) DBG_MAQAO
  SgDeclarationStatementPtrList& declList = root->get_declarations ();

  //Travel throught declarations Statements
  for (SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p) {
    // We only want to print what it is in the current file.    
    if (currentFile.find((*p)->getFilenameString()) == std::string::npos 
    && (*p)->getFilenameString() != "compilerGenerated") {
      continue;
    }

    printAllStmt(*p);
  }
}

void printAllStmt(SgDeclarationStatement *decl) {
  if(DEBUG) DBG_MAQAO

  if (SgModuleStatement* modstmt = isSgModuleStatement(decl)) {
    if (DEBUG) std::cout << "[printAllStmt] - SgModuleStatement" << std::endl;
    
    //printAllSrcLoopsFromModules(project);
    SgClassDefinition * modDef =  modstmt->get_definition ();
    SgDeclarationStatementPtrList & funcList =  modDef->getDeclarationList ();
    for (SgDeclarationStatementPtrList::iterator it = funcList.begin(); it != funcList.end(); ++it) {
      SgFunctionDeclaration *func = isSgFunctionDeclaration(*it);
      if (func == NULL)
        return;
      SgFunctionDefinition *defn = func->get_definition();
      if (defn == NULL)
        return;
      std::cout << func->get_name().getString() << " (";
      SgInitializedNamePtrList paramList = func->get_parameterList()->get_args ();
      SgSymbolTable * st = defn->get_symbol_table();

      for(int i=0; i < paramList.size(); i++) {
        SgSymbol* vsym = st->find_variable(paramList[i]->get_name().getString());
        std::cout << vsym->get_type ()->class_name() << ", ";
      }
      std::cout << ")" << std::endl;

      SgBasicBlock *body = defn->get_body();
      printAllStmt(body);
    }
  
  } else if (SgNamespaceDeclarationStatement* namespacedecl = isSgNamespaceDeclarationStatement(decl)) {
    if (DEBUG) std::cout << "[printAllStmt] - SgNamespaceDeclarationStatement : " << namespacedecl->get_name().getString() << std::endl;
    SgNamespaceDefinitionStatement * namespacedef = namespacedecl->get_definition ();
    if (!namespacedef) return;

    SgDeclarationStatementPtrList & namespaceDeclList = namespacedef->get_declarations ();
    for (int i=0; i < namespaceDeclList.size(); i++) {
      printAllStmt(namespaceDeclList[i]);
    }
  
  } else if (SgNamespaceDefinitionStatement*  namespacedef  = isSgNamespaceDefinitionStatement(decl)) {
    if (DEBUG) std::cout << "[printAllStmt] - SgNamespaceDefinitionStatement" << std::endl;

    SgDeclarationStatementPtrList & namespaceDeclList = namespacedef->get_declarations ();
    for (int i=1; i < namespaceDeclList.size(); i++) {
      printAllStmt(namespaceDeclList[i]);
    }
  
  } else if (SgTemplateInstantiationDecl*     templateDecl  = isSgTemplateInstantiationDecl(decl)) {
    //I'm not sure of what is represented by this class.
    if (DEBUG) std::cout << "[printAllStmt] - SgTemplateInstantiationDecl" << std::endl;
    if (SgClassDeclaration* templateClassDecl = isSgClassDeclaration(templateDecl)) {
      if (SgClassDefinition * classDef = templateClassDecl->get_definition ()) {
        SgDeclarationStatementPtrList & declList =  classDef->get_members ();
        if (declList.size() != 0) {
          for(int i=0; i < declList.size(); i++) {
            printAllStmt(declList[i]);
          } 
        } else { std::cout << "There is no members" << std::endl;}
      } else { std::cout << "There is no definition of the class" << std::endl;}
    } else  { std::cout << "It is not a class declaration" << std::endl;}

  } else if (SgTemplateInstantiationMemberFunctionDecl* classMemberfuncDecl = isSgTemplateInstantiationMemberFunctionDecl(decl)) {
    if (DEBUG) std::cout << "[printAllStmt] - SgTemplateInstantiationMemberFunctionDecl" << std::endl;

    SgDeclarationStatement* definingDeclaration = classMemberfuncDecl->get_definingDeclaration();
    SgMemberFunctionDeclaration* memberFunctionDeclaration = (definingDeclaration == NULL) ? NULL : isSgMemberFunctionDeclaration(definingDeclaration);
    SgFunctionDefinition *defn = memberFunctionDeclaration->get_definition ();
    if (defn == NULL) return;

    std::cout << memberFunctionDeclaration->get_name().getString() << " (";
    SgInitializedNamePtrList paramList = memberFunctionDeclaration->get_parameterList()->get_args ();
    SgSymbolTable * st = defn->get_symbol_table();

    for(int i=0; i < paramList.size(); i++) {
      SgSymbol* vsym = st->find_variable(paramList[i]->get_name().getString());
      std::cout << vsym->get_type ()->class_name() << ", ";
    }
    std::cout << ")" << std::endl;

    SgBasicBlock *body = defn->get_body();
    printAllStmt(body);

  } else if (SgTemplateDeclaration* templateDecl = isSgTemplateDeclaration(decl)) {
    if(DEBUG) std::cout << "[printAllStmt] - SgTemplateDeclaration" << std::endl;
    std::cout << "Warning: Templates statement are not managed by Rose. It only contains a string representing the body of the template." << std::endl;
    //std::cout << "current decl : " <<std::endl<< decl->unparseToString() << std::endl;
    //std::cout << "========" << std::endl;

    // SgDeclarationStatementPtrList & decllist = templateDecl->get_scope()->getDeclarationList();
    // // i=0 is the current template of the scope
    // for (int i=1; i < decllist.size(); i++) {
    //   std::cout << decllist[i]->class_name() << std::endl;
    //   std::cout << decllist[i]->unparseToString() << std::endl;
    // }
    // std::cout << "=================" << std::endl;
  } else {
    if(DEBUG)std::cout << "[ELSE] Class = " << decl->class_name() << std::endl;
    //std::cout << "[printAllStmt] - (it should be) SgFunctionDeclaration" << std::endl;

    SgFunctionDeclaration *func = isSgFunctionDeclaration(decl);
    if (func == NULL) return;

    SgFunctionDefinition *defn = func->get_definition();
    if (defn == NULL) return;

    std::cout << func->get_name().getString() << " (";
    SgInitializedNamePtrList paramList = func->get_parameterList()->get_args ();
    SgSymbolTable * st = defn->get_symbol_table();

    for(int i=0; i < paramList.size(); i++) {
      SgSymbol* vsym = st->find_variable(paramList[i]->get_name().getString());
      std::cout << vsym->get_type ()->class_name() << ", ";
    }
    std::cout << ")" << std::endl;

    SgBasicBlock *body = defn->get_body();
    printAllStmt(body);
  }
}

void printAllStmt(SgBasicBlock *body) {
  if(DEBUG) DBG_MAQAO
  
  SgStatementPtrList & stmtsList = body->get_statements();

  for (SgStatementPtrList::iterator p = stmtsList.begin(); p != stmtsList.end(); ++p) 
  {      
    std::cout << (*p)->class_name() << std::endl;
    if (SgFunctionDeclaration * func = isSgFunctionDeclaration(*p)) {
      if (SgFunctionDefinition *defn = func->get_definition()) {
        SgBasicBlock *body = defn->get_body();
        printAllStmt(body);
      }
    } else if (SgForStatement * loop = isSgForStatement(*p)) {
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body());
      if (subbody)
        printAllStmt(subbody);
    } else if (SgFortranDo * loop = isSgFortranDo(*p)) {
      printAllStmt(loop->get_body());
    } else if (SgDoWhileStmt * loop = isSgDoWhileStmt(*p)) {      
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
      if(subbody)
        printAllStmt(subbody);
    } else if (SgWhileStmt * loop = isSgWhileStmt(*p)) {
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
      if(subbody)
        printAllStmt(subbody);
    } else if (SgIfStmt * ifstmt = isSgIfStmt(*p)) {
      SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ());
      SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ());
      if(bodytrue)
        printAllStmt(bodytrue);
      if(bodyfalse)
        printAllStmt(bodyfalse);
    } else if (SgSwitchStatement* switchStmt = isSgSwitchStatement(*p)) {
        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body()))
          printAllStmt(subbody);
    } else if (SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body()))
          printAllStmt(subbody);
    } else if (SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body()))
          printAllStmt(subbody);
    } else if (SgBasicBlock * bb = isSgBasicBlock(*p)) {
      printAllStmt(bb);
    } else if (SgScopeStatement * scp = isSgScopeStatement(*p)) {
        if (DEBUG)
          std::cout << "----------THIS SCOPE WAS NOT HANDLED YET: " << scp->class_name() << std::endl;
    } else if (SgExprStatement * exprStmt = isSgExprStatement(*p)) {
      std::cout << exprStmt->get_expression()->class_name() << std::endl;
    }
  }
}

void printAllSrcLoops(SgBasicBlock *body) {
  if(DEBUG) DBG_MAQAO
  SgStatementPtrList & stmtsList = body->get_statements();

  for (SgStatementPtrList::iterator p = stmtsList.begin(); p != stmtsList.end(); ++p) 
  {      
    if (SgFunctionDeclaration *func = isSgFunctionDeclaration(*p)) {
      if (SgFunctionDefinition *defn = func->get_definition()) {
        SgBasicBlock *body = defn->get_body();
        printAllSrcLoops(body);
      }
    }else if (SgForStatement  * loop = isSgForStatement(*p)) {
      std::cout << "start " << loop->get_startOfConstruct ()->get_line() ;
      std::cout << " end " << loop->get_endOfConstruct ()->get_line() ;
      //std::cout << " file " << loop->getFilenameString ();
      std::cout << std::endl;
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body());
      if (subbody)
        printAllSrcLoops(subbody);
    } else if (SgFortranDo  * loop = isSgFortranDo(*p)) {
      std::cout << "start " << loop->get_startOfConstruct ()->get_line() ;
      std::cout << " end " << loop->get_endOfConstruct ()->get_line() ;
      // std::cout << " file " << loop->getFilenameString ();
      std::cout << std::endl;
      printAllSrcLoops(loop->get_body());
    } else if (SgDoWhileStmt  * loop = isSgDoWhileStmt(*p)) {
      std::cout << "start " << loop->get_startOfConstruct ()->get_line() ;
      std::cout << " end " << loop->get_endOfConstruct ()->get_line() ;
      // std::cout << " file " << loop->getFilenameString ();
      std::cout << std::endl;
      
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
      if(subbody)
        printAllSrcLoops(subbody);
    
    } else if (SgWhileStmt  * loop = isSgWhileStmt(*p)) {
      std::cout << "start " << loop->get_startOfConstruct ()->get_line() ;
      std::cout << " end " << loop->get_endOfConstruct ()->get_line() ;
      // std::cout << " file " << loop->getFilenameString ();
      std::cout << std::endl;
      
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
      if(subbody)
        printAllSrcLoops(subbody);
    
    } else if (SgIfStmt * ifstmt = isSgIfStmt(*p)) {
      SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ());
      SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ());
      if(bodytrue)
        printAllSrcLoops(bodytrue);
      if(bodyfalse)
        printAllSrcLoops(bodyfalse);
    } else if (SgSwitchStatement* switchStmt = isSgSwitchStatement(*p)) {
        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body()))
          printAllSrcLoops(subbody);
    } else if (SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body()))
          printAllSrcLoops(subbody);
    } else if (SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body()))
          printAllSrcLoops(subbody);
    } else if(SgScopeStatement * scp = isSgScopeStatement(*p)) {
        if (DEBUG)
          std::cout << "----------THIS SCOPE WAS NOT HANDLED YET: " << scp->class_name() << std::endl;
    } 
  }
}

void printAllSrcLoops(SgProject* project) {
  if(DEBUG) DBG_MAQAO
  SgSourceFile* file = isSgSourceFile(project->get_fileList()[0]);
  if (isFortranFile(file->getFileName ())) {
    for(int i=0; i < project->numberOfFiles () ; ++i) {
      file = isSgSourceFile(project->get_fileList()[i]);
      if (isFortranFile(file->getFileName ())) {
        break;
      }
    }
  }

  SgGlobal *root = file->get_globalScope();
  SgDeclarationStatementPtrList& declList = root->get_declarations ();

  //Travel throught declarations Statements
  for (SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p) {
    if (SgModuleStatement* modstmt = isSgModuleStatement(*p)) {
      //std::cout << "----------isSgModuleStmt l."<< modstmt->get_file_info()->get_line()<< std::endl;
      //printAllSrcLoopsFromModules(project);
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
        printAllSrcLoops(body);
      }
    } else 
    {
      SgFunctionDeclaration *func = isSgFunctionDeclaration(*p);
      if (func == NULL)
        continue;
      SgFunctionDefinition *defn = func->get_definition();
      if (defn == NULL)
        continue;

      SgBasicBlock *body = defn->get_body();
      printAllSrcLoops(body);
    }
  }
}

void printdebug (std::string s) {
  // Check if the iostream is in a file or in stdout
  //if () { 
    std::cout << "\033[1;36m" << s << "\033[0m" << std::endl;
  // else {
    // std::cout << s << std::endl;
  // }
}

void printerror (std::string s) {
  // Check if the iostream is in a file or in stdout
  //if () { 
    std::cout << "\066[1;31m" << s << "\066[0m" << std::endl;
  // else {
    // std::cout << s << std::endl;
  // }
}

std::string itoa(int i) {
  std::ostringstream stm;
  std::string s ="";
  stm  << i;
  s += stm.str();
  return s;
}

//////////////////////////
// Browse to build AST  //
//////////////////////////
void browseFile_and_apply_directives(SgGlobal* globalScope) {
  if (DEBUG) DBG_MAQAO
  if (DEBUG) std::cout << "--- APPLY DIRECTIVES ON FUNCTIONS ---" << std::endl;
  SgFunctionDeclaration* funcDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);

  while (funcDecl) {
    // No need to anlayze function we had created 
    // (and it avoid to crash due to the deepCopyNode function which not accept transformed nodes
    // (it should miss some information))
    // /!\ Avoid template functions because they are not handled by Rose
    if (!funcDecl->get_file_info()->isTransformation() && !isSgTemplateInstantiationMemberFunctionDecl(funcDecl)) {
      Function * func  = new Function(funcDecl->get_definition());
      func->apply_directive();
    }
    SgStatement* nextStmt = SageInterface::getNextStatement (funcDecl);
    //We have to avoid pragma declaration 
    while (isSgPragmaDeclaration(nextStmt) && nextStmt) nextStmt = SageInterface::getNextStatement (nextStmt);

    funcDecl = isSgFunctionDeclaration(nextStmt);    
    if (DEBUG && nextStmt) std::cout << nextStmt->class_name() << std::endl;
  }

  if(DEBUG) std::cout << "--- APPLY DIRECTIVES ON LOOPS ---" << std::endl;
  ASTRoot astL(globalScope);
  astL.apply_directive();
}

void browseFile_and_apply_directives(SgBasicBlock *body) {
  if(DEBUG) DBG_MAQAO
  int identifierLoops = 0;
  int id = identifierLoops++; 
  SgNode* node = SageInterface::getFirstStatement(body);

  while (node) {
    switch((node)->variantT ()) {
      case V_SgForStatement: {
        SgForStatement  * loop = isSgForStatement(node);
        if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body())) {
          browseFile_and_apply_directives(subbody);
        }

        Loop* newNode = new Loop(loop, id);
        newNode->apply_directive();
        break;
      }
      case V_SgFortranDo: {
       SgFortranDo * loop = isSgFortranDo(node);
        if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
           browseFile_and_apply_directives(subbody);
         
        Loop* newNode = new Loop(loop, id);
        newNode->apply_directive();
        break;
      }
      case V_SgDoWhileStmt: {
        SgDoWhileStmt * loop = isSgDoWhileStmt(node);
        if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
          browseFile_and_apply_directives(subbody);

        Loop* newNode = new Loop(loop, id);
        newNode->apply_directive();
        break;
      }
      case V_SgWhileStmt: {
        SgWhileStmt  * loop = isSgWhileStmt(node);
        if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
          browseFile_and_apply_directives(subbody);

        Loop* newNode = new Loop(loop, id);
        newNode->apply_directive();
        break;
      }
      case V_SgIfStmt: {
        SgIfStmt * ifstmt = isSgIfStmt(node);
        if(SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ())) {
          browseFile_and_apply_directives(bodytrue);
        }

        if(SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ())) {
          browseFile_and_apply_directives(bodyfalse);
        }
        break;
      }
      case V_SgSwitchStatement: {
        SgSwitchStatement* switchStmt = isSgSwitchStatement(node);
        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body())) {
          browseFile_and_apply_directives(subbody);
        }
        break;
      }
      case V_SgCaseOptionStmt: {
        SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(node);
        if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body())) {
          browseFile_and_apply_directives(subbody);
        }
        break;
      }
      case V_SgDefaultOptionStmt: {
        SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(node);
        if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body())) {
          browseFile_and_apply_directives(subbody);
        }
        break;
      }
      case V_SgBasicBlock: {
        SgBasicBlock* subbody = isSgBasicBlock(node);
        browseFile_and_apply_directives(subbody);
        break;
      }
      #if 0
      //Not handle yet.
      case V_SgExprStatement:
      SgExprStatement* exprstmt = isSgExprStatement(*p); // array(:) or array (:,:,...,:)
      if (SgAssignOp * assign = isSgAssignOp(exprstmt->get_expression())) {
        if (SgPntrArrRefExp* arrayRef = isSgPntrArrRefExp(assign)) {
          if (SgExprListExp* exlex = isSgExprListExp(arrayRef->get_rhs_operand())) {
            SgExpressionPtrList & exList = exlex->get_expressions ();
            for(int i=0; i < exList.size(); i++) {
              if (SgSubscriptExpression * subscrpt = isSgSubscriptExpression(exList[i])) {
                STLoops* newNode = new ASTLoops(exprstmt, id);
                internalLoopsList.push_back(newNode);
              }
            }

          } else if (SgSubscriptExpression * subscrpt = isSgSubscriptExpression(arrayRef->get_rhs_operand())) {
            ASTLoops* newNode = new ASTLoops(exprstmt, id);
            internalLoopsList.push_back(newNode);
            //No body to analyze
          }
        }
      }
      #endif
      case V_SgScopeStatement: {
        SgScopeStatement * scp = isSgScopeStatement(node);
        if(DEBUG > 1) {
          DBG_MAQAO std::cout << "-----THIS SCOPE WAS NOT HANDLED YET: " << scp->class_name() << std::endl;
        }
        // /!\
        // This function do the job, 
        // but if we launch an other analyze just after it crash because it doesn't delete something in the memory pool
        // Please don't use it !
        //createAST(SageBuilder::buildBasicBlock_nfi(scp->getStatementList()));
        break;
      }
  
    }
    if (isSgStatement(node))
      node = SageInterface::getNextStatement(isSgStatement(node));
    else {
      DBG_MAQAO
      std::cout << "/!\\ WARNING:  IT'S NOT A STATEMENT !! \n" << node->class_name() << std::endl;
      return;
    }
  }
}

//////////////////////
// String functions //
//////////////////////
int find_first_number(std::string str ) {
  if(DEBUG) DBG_MAQAO
  int pos=9999999, postmp, i =0;
  char buffer[33];

  for (int i = 0 ; i < 10; i++) {
    sprintf(buffer,"%d",i);
    postmp = str.find_first_of(buffer);
    if (pos > postmp && postmp != std::string::npos) {
      pos = postmp;
    }
  }
  if (pos==9999999) return -1;
  return pos;
}

int substr_counter_in_file(std::string substr, std::string file) {
  if(DEBUG) DBG_MAQAO
  std::string line;
  int cpt=0;
  std::ifstream myfile (file.c_str());
  if (myfile.is_open()) {
    while ( getline (myfile,line) ) {
      if (size_t pos = line.find(substr) != std::string::npos ) 
        cpt++;
    }
    myfile.close();
  }
  return cpt;
}

/////////////////////
// Build functions //
/////////////////////
SgBinaryOp * myBuildBinOp(SgExpression * var, SgExpression * incr, int op,  bool paren) {
  if(DEBUG) DBG_MAQAO
  SgBinaryOp * newBO = NULL;
  switch(op) {
    case V_SgAddOp :      newBO = SageBuilder::buildAddOp(var,incr);      break;
    case V_SgSubtractOp : newBO = SageBuilder::buildSubtractOp(var,incr); break;
    case V_SgMultiplyOp : newBO = SageBuilder::buildMultiplyOp(var,incr); break;
    case V_SgDivideOp :   newBO = SageBuilder::buildDivideOp(var,incr);   break;
    case V_SgAssignOp :   newBO = SageBuilder::buildAssignOp(var,incr);   break;
    case V_SgEqualityOp : newBO = SageBuilder::buildEqualityOp(var,incr); break;
    case V_SgNotEqualOp : newBO = SageBuilder::buildNotEqualOp(var,incr); break;
    case V_SgGreaterOrEqualOp : newBO = SageBuilder::buildGreaterOrEqualOp(var,incr); break;
    case V_SgGreaterThanOp :  newBO = SageBuilder::buildGreaterThanOp(var,incr);  break;
    case V_SgLessOrEqualOp :  newBO = SageBuilder::buildLessOrEqualOp(var,incr);  break;
    case V_SgLessThanOp :     newBO = SageBuilder::buildLessThanOp(var,incr); break;
    case V_SgAndOp :  newBO = SageBuilder::buildAndOp(var,incr);  break;
    case V_SgOrOp :   newBO = SageBuilder::buildOrOp(var,incr);   break;
    case V_SgModOp :  newBO = SageBuilder::buildModOp(var,incr);  break;
    default:
      NOT_IMPLEMENTED_YET
      break;
  }
  newBO->set_need_paren (paren);
  return newBO;
}

bool replace_expr (SgBasicBlock* body, SgVariableSymbol * var, SgExpression * newExpr) {
  if(DEBUG > 1) DBG_MAQAO
  if(!body) {return false;}
  
  SgStatementPtrList & stmtsList = body->get_statements();

  // for (SgStatementPtrList::iterator it = stmtsList.begin(); it != stmtsList.end(); ++it) {
  for (int i=0; i < stmtsList.size(); i++) {
    // -------------------------------------------------------------------------
    switch(stmtsList[i]->variantT()) {
      case V_SgForStatement : {
        SgForStatement  * loop = isSgForStatement(stmtsList[i]);
        SgStatementPtrList & initList = loop->get_init_stmt ();
        for (int i = 0; i < initList.size() ; i++) {
          if (SgExprStatement* initExp = isSgExprStatement(initList[i])) {
            replace_expr(initExp->get_expression(), var, newExpr);
          }
        }

        if (SgExprStatement* bound =  isSgExprStatement(loop->get_test ())) {
          replace_expr(bound->get_expression(), var, newExpr);
        }
        SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body());
        if (subbody) { replace_expr(subbody, var, newExpr); }
        break;
      }
      case V_SgFortranDo : {
        SgFortranDo  * loop = isSgFortranDo(stmtsList[i]);
        if( SgExpression* bound =  isSgExpression(loop->get_bound ())) {
          replace_expr(bound, var, newExpr);
        }
        replace_expr(loop->get_body(), var, newExpr);
        break;
      }
      case V_SgDoWhileStmt : {
        SgDoWhileStmt  * loop = isSgDoWhileStmt(stmtsList[i]);
        SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
        if(subbody) {replace_expr(subbody, var, newExpr);}
        break;
      }
      case V_SgWhileStmt : {
        SgWhileStmt  * loop = isSgWhileStmt(stmtsList[i]);
        if (isSgExprStatement(loop->get_condition ())) {
          SgExpression* expstmt = isSgExprStatement(loop->get_condition ())->get_expression();
          if (!replace_expr (expstmt, var, newExpr)) { return false; }
        }
        else if (SgExpression * expr = isSgExpression(loop->get_condition ())) {
          if (!replace_expr (expr, var, newExpr)) { return false; }
        }
        SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
        if(subbody) { replace_expr(subbody, var, newExpr); }
        break;
      }
      case V_SgIfStmt : {  
        SgIfStmt * ifstmt = isSgIfStmt(stmtsList[i]);
        if (isSgExprStatement(ifstmt->get_conditional ())) {
          SgExpression* expstmt = isSgExprStatement(ifstmt->get_conditional())->get_expression();
          if (!replace_expr (expstmt, var, newExpr)) {return false;}
        }
        else if (SgExpression* expr = isSgExpression(ifstmt->get_conditional ())) {
          if (!replace_expr (expr, var, newExpr)) {return false;}
        } else {
          std::cout << " Warning : we not handle the " 
                    << ifstmt->get_conditional ()->class_name() 
                    << " class yet to the constante propagation transformation!\n";
        }

        SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body());
        SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body());
        if(bodytrue)
          replace_expr(bodytrue, var, newExpr);
        if(bodyfalse)
          replace_expr(bodyfalse, var, newExpr);
        break;
      }
      case V_SgSwitchStatement : {
        SgSwitchStatement* switchStmt = isSgSwitchStatement(stmtsList[i]);

        if (isSgExprStatement(switchStmt->get_item_selector ())) {
          SgExpression* expstmt = isSgExprStatement(switchStmt->get_item_selector())->get_expression();
          if (!replace_expr (expstmt, var, newExpr)) { return false; }
        }
        else if (SgExpression* expr = isSgExpression(switchStmt->get_item_selector ())) {
          if (!replace_expr (expr, var, newExpr)) { return false; }
        } else {
          std::cout << "Warning :  we not handle the " 
                    << switchStmt->get_item_selector ()->class_name() 
                    << " class yet to the constante propagation transformation!\n";
        }

        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body())) { replace_expr(subbody, var, newExpr); }
        break;
      }
      case V_SgCaseOptionStmt : {
        SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(stmtsList[i]);
        if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body())) { replace_expr(subbody, var, newExpr); }
        break;
      }
      case V_SgDefaultOptionStmt :  {
        SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(stmtsList[i]);
        if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body())) { replace_expr(subbody, var, newExpr); }
        break;
      }
      case V_SgBasicBlock :  {
        if(SgBasicBlock * subbody = isSgBasicBlock(stmtsList[i])) { 
          replace_expr(subbody, var, newExpr); 
        }
        break;
      }
      case V_SgScopeStatement : {
        std::cout << "Warning the class : " << stmtsList[i]->class_name() << " is not handle yet to replace expression." << std::endl;
        return false;
        break;
      }
      case V_SgVariableDeclaration: {
        replace_expr(isSgVariableDeclaration(stmtsList[i])->get_definition()->get_vardefn ()->get_initializer (), var, newExpr); 
        break;
      }
      case V_SgPrintStatement: {
        SgExprListExp * printExpList = isSgPrintStatement(stmtsList[i])->get_io_stmt_list ();
        SgExpressionPtrList &  expList = printExpList->get_expressions ();
        for (int i=0; i < expList.size(); i++) {
          replace_expr(expList[i], var, newExpr); 
        }
        break;
      }
      case V_SgContinueStmt:
      case V_SgPragmaDeclaration: {
        break;
      }
      case V_SgExprStatement : {
        if(SgExprStatement * expr = isSgExprStatement(stmtsList[i])) { 
          replace_expr(expr->get_expression(), var, newExpr); 
        }
        break;
      }
      default: {
          // if(DEBUG) 
            std::cout << "[replace_expr: BB] Cannot analyze " << stmtsList[i]->class_name() << " is not handle yet." << std::endl;
        }
    }

    // -------------------------------------------------------------------------
    if (isSgExprStatement(stmtsList[i])) {
      SgExpression* expstmt = isSgExprStatement(stmtsList[i])->get_expression();
      if (!replace_expr (expstmt, var, newExpr)) { return false; }
    }
  }
  return true;
}

bool replace_expr (SgExpression* expstmt, SgVariableSymbol * var, SgExpression * newExpr) {
  if(DEBUG > 1) DBG_MAQAO
  if(!expstmt) return false;
  if (newExpr == expstmt->get_parent()) return false;
  
  // CALL stmt
  if (SgFunctionCallExp * fcallexp = isSgFunctionCallExp(expstmt)) {
    SgExpressionPtrList argList = fcallexp->get_args ()->get_expressions ();
    for (int i=0; i < argList.size(); i++) {
      replace_expr (argList[i], var, newExpr);
    }
  }
  // Expression list (represent parameters of a function call, for example)
  else if (isSgExprListExp(expstmt)) {
    SgExpressionPtrList expList = isSgExprListExp(expstmt)->get_expressions();
    for (int i=0; i < expList.size(); i++) {
      replace_expr (expList[i], var, newExpr);
    }
  }
  //Variable reference
  // It's the only moment where we replace a variable by the expression of substitution
  else if(isSgVarRefExp(expstmt)) {
    if(isSgVarRefExp(expstmt)->get_symbol()->get_name() == var->get_name()) {
      newExpr->set_parent((expstmt)->get_parent());
      SageInterface::replaceExpression(expstmt, newExpr);
    }
  }
  //Function reference 
  else if (SgFunctionRefExp * func_ref_expr = isSgFunctionRefExp(expstmt)) {
    SgInitializedNamePtrList argList = func_ref_expr->get_symbol()->get_declaration()->get_args();    
    if (DEBUG) {
      DBG_MAQAO
      std::cout << "[Replace_expr: expStmt] Warning this case was not handle yet : " << (expstmt)->class_name() <<  std::endl;
      std::cout << expstmt->unparseToString() << std::endl;
    }
  } 
  //Binary operation
  else if (SgBinaryOp * bo = isSgBinaryOp(expstmt)) {
    replace_expr (bo, var, newExpr);
  }
  // Example of SubscriptExpression : dgxdtfac_(1:cplex_fac,1:ndgxdtfac,ilmn) (ABINIT)
  //                                           lb|upbound
  else if (SgSubscriptExpression* subscript = isSgSubscriptExpression (expstmt)) {
    s2s_api::replace_expr (subscript->get_lowerBound (), var, newExpr);
    s2s_api::replace_expr (subscript->get_upperBound (), var, newExpr);
    s2s_api::replace_expr (subscript->get_stride ()    , var, newExpr);
  }
  // example of SgActualArgumentExpression : kind=dp
  else if (SgActualArgumentExpression* actexpr = isSgActualArgumentExpression (expstmt)) {
    s2s_api::replace_expr (actexpr->get_expression (), var, newExpr);
  }
  else if (SgUnaryOp* uop = isSgUnaryOp(expstmt)) {
    s2s_api::replace_expr(uop->get_operand_i(), var, newExpr);
  }
  else if (SgAssignInitializer* assign = isSgAssignInitializer(expstmt)) {
    s2s_api::replace_expr(assign->get_operand_i(), var, newExpr);
  } else if (SgAggregateInitializer* ai = isSgAggregateInitializer(expstmt)) {
    SgExpressionPtrList & exprList = ai->get_initializers ()->get_expressions ();
    for (int i=0; i < exprList.size(); i++) {
      replace_expr(exprList[i], var, newExpr);
    }
  }
  else if (SgConditionalExp* condExp  = isSgConditionalExp(expstmt)) {
    s2s_api::replace_expr(condExp->get_true_exp  (), var, newExpr);
    s2s_api::replace_expr(condExp->get_false_exp (), var, newExpr);
    s2s_api::replace_expr(condExp->get_conditional_exp (), var, newExpr);
  }
  //Other expression .... 
  else {

      if (!isSgValueExp(expstmt) && !isSgNullExpression(expstmt)) {
        std::cout << "[Replace_expr: expStmt] Warning this case was not handle yet : " << (expstmt)->class_name() << std::endl;
        std::cout << expstmt->unparseToString() << std::endl;
      }
  }
  return true;
}

bool replace_expr (SgBinaryOp* bo, SgVariableSymbol * var, SgExpression * newExpr) {
  if(DEBUG > 1) DBG_MAQAO
  if (!bo) return false;
  SgExpression * lhs, * rhs;
  lhs = bo->get_lhs_operand();
  rhs = bo->get_rhs_operand();

  if (SgAssignOp* assOp = isSgAssignOp(bo)) {
    if (SgPntrArrRefExp* pntrarr = isSgPntrArrRefExp(lhs)) {
      //The rhs is an SgExprListExp
      replace_expr(pntrarr->get_rhs_operand(), var, newExpr);
    }
  } else {
    if (SgBinaryOp* lbo = isSgBinaryOp(lhs)) {
      replace_expr(lbo, var, newExpr);
    } else {
    replace_expr(lhs, var, newExpr);
    }
  }

  //if the right side of the binary operation is a binary operation
  if (SgBinaryOp* rbo = isSgBinaryOp(rhs)) {
    replace_expr(rbo, var, newExpr);
  }
  else {
    replace_expr(rhs, var, newExpr);
  }

  return true;
}

bool constantPropagation (SgExpression* expstmt, SgVariableSymbol * var, int constToPropagate) {
 if(DEBUG) DBG_MAQAO
 SgIntVal * intVal = SageBuilder::buildIntVal(constToPropagate);
 return replace_expr(expstmt, var, intVal);
}

bool constantPropagation (SgBasicBlock* body, SgVariableSymbol * var, int constToPropagate) {
  if(DEBUG) DBG_MAQAO
  SgIntVal * intVal = SageBuilder::buildIntVal(constToPropagate);
  return replace_expr(body, var, intVal);
}

bool constantPropagation (SgStatement* stmt, SgVariableSymbol * var, int constToPropagate) {
 if(DEBUG) DBG_MAQAO
 SgIntVal * intVal = SageBuilder::buildIntVal(constToPropagate);

 switch(stmt->variantT()) {
  case V_SgExprStatement: {
    return replace_expr(isSgExprStatement(stmt)->get_expression (), var, intVal);
  }
  case V_SgFortranDo: {
    bool trans_bound = replace_expr(isSgFortranDo(stmt)->get_bound (), var, intVal);
    bool trans_body  = replace_expr(isSgFortranDo(stmt)->get_body(), var, intVal);
    bool trans_init  = replace_expr(isSgFortranDo(stmt)->get_initialization (), var, intVal);
    return  trans_bound || trans_body || trans_init;
  }
  case V_SgForStatement: {
    bool trans_bound = replace_expr(isSgExprStatement(isSgForStatement(stmt)->get_test())->get_expression(), var, intVal);
    bool trans_body  = replace_expr(isSgBasicBlock(isSgForStatement(stmt)->get_loop_body()), var, intVal);
    bool trans_init = false;

    SgStatementPtrList initList = isSgForStatement(stmt)->get_for_init_stmt ()->get_init_stmt ();
    for (int i = 0; i < initList.size(); i++) {
        if (replace_expr(isSgExprStatement(initList[i])->get_expression(), var, intVal)) trans_init = true;
    }
    return  trans_bound || trans_body || trans_init;
  }
  default: {
    if(DEBUG) {
      std::cout << "The type " << stmt->class_name() << " is not handle yet for propagate constantes." << std::endl;
    }
    return false;
  }
 }
}

///////////////////////////
// Partial DCE functions //
///////////////////////////
void dead_code_elimination (SgStatement* stmt, std::vector<variable_spe*> varspe) {
  if(DEBUG) DBG_MAQAO

  switch(stmt->variantT()) {
  case V_SgBasicBlock:  { dead_code_elimination(isSgBasicBlock(stmt), varspe);              break; }
  case V_SgFortranDo:   { dead_code_elimination(isSgFortranDo(stmt)->get_body(), varspe);   break; }
  case V_SgForStatement:{ dead_code_elimination(isSgForStatement(stmt)->get_loop_body(), varspe);   break; }
  case V_SgWhileStmt:   { dead_code_elimination(isSgWhileStmt(stmt)->get_body(), varspe);   break; }
  case V_SgDoWhileStmt: { dead_code_elimination(isSgDoWhileStmt(stmt)->get_body(), varspe); break; }
  case V_SgIfStmt:      {  
    SgIfStmt* ifstmt = isSgIfStmt(stmt);
    // TODO look for label above the stmt et if exist look inside varspe to compare labels    
    dead_code_elimination(isSgIfStmt(stmt)->get_true_body() , varspe);  
    dead_code_elimination(isSgIfStmt(stmt)->get_false_body(), varspe);
    break;
  }
  default: {
    if(DEBUG) {
      std::cout << "The type " << stmt->class_name() << " is not handle yet for propagate constantes." << std::endl;
    }
    return;
  }
 }
}

void dead_code_elimination(SgBasicBlock* body) {
  if(DEBUG) DBG_MAQAO
  if(!body) return;

  bool dead_code_here;
  bool transformation_applied; // will be used to determine how much time we have to run the deadcodeelimination
  do {
    transformation_applied = false;
    SgStatementPtrList & stmtsList = body->get_statements();

    for (SgStatementPtrList::iterator it = stmtsList.begin(); it != stmtsList.end(); ++it) {
      dead_code_here = false;

      switch((*it)->variantT()) {
        case V_SgForStatement : {
          SgForStatement  * loop = isSgForStatement(*it);
          if(SgExprStatement * exprStmt = isSgExprStatement(loop->get_test ())) {
            if (isAlwaysFalse(exprStmt->get_expression())) {
              dead_code_here = true;
              transformation_applied = false;
              if (LOG > 1) { log("Condition of the \"for loop\" is always false, the statement will be remove : ", (*it)); }
              // /!\ DELETE THE WHOLE LOOP !
              it--;
              body->remove_statement(*(it+1));
            }
          }
          //If no dead code look inside
          if (!dead_code_here) {
            if (SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body())) {
              dead_code_elimination(subbody);
            }
          }
          if (isSgBasicBlock(loop->get_loop_body())->get_statements().empty()) {
            if (LOG > 1) { log("The \"for loop\" is empty, the statement will be remove : ", (*it)); }
            it--;
            body->remove_statement(*(it+1));
          }
          break;
        }
        case V_SgFortranDo : {
          SgFortranDo  * loop = isSgFortranDo(*it);
          
          if( loop->get_body()->get_statements().empty()) {
            dead_code_here = true;
            transformation_applied = true;

            if (LOG > 1) { log("The \"do loop\" is empty, the statement will be remove : ", (*it)); }
            // /!\ DELETE THE WHOLE LOOP ! because the loop is empty
            SageInterface::removeStatement(*it, false);
            break;
          } 

          if( SgExpression* bound =  isSgExpression(loop->get_bound ())) {
            //Check if dead code here !
            SgExpression* loop_init = loop->get_initialization ();
            if (isSgAssignOp(loop_init)) {
              if(isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand())) {
                if(isAlwaysFalse(SageBuilder::buildLessOrEqualOp(isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand()), bound))) {
                  dead_code_here = true;
                  transformation_applied = true;
                  if (LOG > 1) { log("Condition of the \"do loop\" is always false, the statement will be remove : ", (*it)); }
                  // /!\ DELETE THE WHOLE LOOP !
                  it--;
                  body->remove_statement(*(it+1));
                } else { // if we had only one loop's iteration.
                  if(isAlwaysTrue(SageBuilder::buildEqualityOp(isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand()), bound))) {
                    dead_code_here = true;
                    transformation_applied = true;
                    if (LOG > 1) { log("The \"do loop\" contains only one iteration, the loop will be replace by its body : ", (*it)); }
                    // /!\ REPLACE THE LOOP BY ITS BODY !
                    it--;

                    s2s_api::constantPropagation(body, 
                      isSgVarRefExp(isSgAssignOp(loop_init)->get_lhs_operand())->get_symbol(), 
                                    isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand())->get_value());

                    SageInterface::replaceStatement(*(it+1), SageInterface::copyStatement(body));
                  }
                }
              }
            }
          }
          
          //If no dead code look inside
          if (!dead_code_here) {
            dead_code_elimination(loop->get_body());
          }
          if (loop->get_body()->get_statements().empty()){
            if (LOG > 1) { log("The \"do loop\" is empty, the statement will be remove : ", (*it)); }
            it--;
            body->remove_statement(*(it+1));
          }

          break;
        }
        case V_SgDoWhileStmt : {
          SgDoWhileStmt  * loop = isSgDoWhileStmt(*it);
          //look inside anyway because it have to e done at least once.
          SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
          if(subbody)
            dead_code_elimination(subbody);
          // If dead code because the conditionnal, juste erase the loop but not the body because it have to be done once at least
          if(SgExprStatement * exprStmt = isSgExprStatement(loop->get_condition())) {
            if (isAlwaysFalse(exprStmt->get_expression())) {
              dead_code_here = true;
              transformation_applied = true;
              if (LOG > 1) { log("remove statement : ", (*it)); }

              // /!\ DELETE THE WHOLE LOOP !
              //AND LEAVE ON ITERATION
              SageInterface::removeStatement(loop, false);
            }
          }
          if (isSgBasicBlock(loop->get_body())->get_statements().empty()){
            if (LOG > 1) { log("remove statement : ", (*it)); }
            it--;
            body->remove_statement(*(it+1));
          }
          break;
        }
        case V_SgWhileStmt : {
          SgWhileStmt  * loop = isSgWhileStmt(*it);
          if (isSgExprStatement(loop->get_condition ())) {
            SgExpression* expr = isSgExprStatement(loop->get_condition ())->get_expression();
            // check if dead code
            if (isAlwaysFalse(expr)) {
              dead_code_here = true;
              transformation_applied = true;

              if (LOG > 1) { log("remove statement : ", (*it)); }
              // /!\ DELETE THE WHOLE LOOP !
              SageInterface::removeStatement(loop, false);
            }
          }
          // if no dead code look inside
          if(!dead_code_here) {
            SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
            if(subbody)
              dead_code_elimination(subbody);   
          }
          if (isSgBasicBlock(loop->get_body())->get_statements().empty()){
            if (LOG > 1) { log("remove statement : ", (*it)); }
            it--;
            body->remove_statement(*(it+1));
          }
          break;
        }
        case V_SgIfStmt : {
          SgIfStmt * ifstmt = isSgIfStmt(*it);
          SgBasicBlock * trueBody = isSgBasicBlock(ifstmt->get_true_body());
          SgBasicBlock * falseBody = isSgBasicBlock(ifstmt->get_false_body());
          SgExpression * cond = isSgExprStatement(ifstmt->get_conditional())->get_expression();
          
          // TODO look for label above the stmt et if exist look inside varspe to compare labels    

          // if NOT do the usal things
          bool trueBodyEmpty = false;
          bool falseBodyEmpty = false;
          if (trueBody)
            if (trueBody->get_statements().empty())
              trueBodyEmpty = true;
          if (falseBody)
            if (falseBody->get_statements().empty())
              falseBodyEmpty = true;


          if( trueBodyEmpty && falseBodyEmpty) {
            if (LOG > 1) { log("the \"if\" statement is empty (true and false body are empty), remove : ", (*it)); }
            SageInterface::removeStatement(ifstmt, false);
            break;
          
          } else if (trueBodyEmpty ) {  
            dead_code_elimination(falseBody);
            ifstmt->set_conditional(SageBuilder::buildExprStatement(SageBuilder::buildNotOp(isSgExprStatement(ifstmt->get_conditional())->get_expression ())));
            ifstmt->set_true_body(SageInterface::copyStatement(falseBody));

            if (LOG > 1) { log("the true body is empty, replace it by the false body (and reverse condition) : ", (*it)); }
            SageInterface::removeStatement(ifstmt->get_false_body(), false);
            break;
          
          } else if (falseBodyEmpty) {
            dead_code_elimination(trueBody);

            if (trueBodyEmpty) {
              if (LOG > 1) { log("the \"if\" statement is empty (true and false body are empty), remove : ", (*it)); }
              SageInterface::removeStatement(ifstmt, false);
              break;
            }
          }

          if(cond) {
            if (isAlwaysTrue(cond)) {
              dead_code_elimination(trueBody);
              if (LOG > 1) { log("Condition  of the \"if statement\" is always true, replace the \"if statement\" by its true body : ", (*it)); }
              SageInterface::replaceStatement(ifstmt, SageInterface::copyStatement(trueBody));
            
            } else if (isAlwaysFalse(cond)) {
              dead_code_elimination(falseBody);
              if (LOG > 1) { log("Condition  of the \"if statement\" is always false, replace the \"if statement\" by its false body : ", (*it)); }
              SageInterface::replaceStatement(ifstmt, SageInterface::copyStatement(falseBody));
            }

            else {
              if (LOG > 1) { log("\n------- The condition of the \"if statement\" is indeterminable (not always true or false) : ", (*it)); }
              dead_code_elimination(trueBody);
              dead_code_elimination(falseBody);

              // Check if the dead_code elimination don't erase all the statements inside one or both bodies.
              if(trueBodyEmpty && falseBodyEmpty) {
                if (LOG > 1) { log("After elimination of dead code in the \"if statement\" the true and false body became empty, remove the \"if statement\" : ", (*it)); }
                SageInterface::removeStatement(ifstmt, false);
                break;
              
              } else if (trueBodyEmpty) {
                ifstmt->set_conditional(SageBuilder::buildExprStatement(SageBuilder::buildNotOp(isSgExprStatement(ifstmt->get_conditional())->get_expression ())));
                ifstmt->set_true_body(SageInterface::copyStatement(falseBody));
                
                if (LOG > 1) { log("After elimination of dead code in the \"if statement\" the true body is empty, replace it by the false body (and reverse condition) : ", (*it)); }
                SageInterface::removeStatement(ifstmt->get_false_body(), false);
                ifstmt->set_false_body(NULL);
                break;
              
              } else if (falseBodyEmpty) {
                if(DEBUG) std::cout << "Only false is empty l." << ifstmt->get_file_info()->get_line() << std::endl;
                // dead_code_elimination(trueBody);
                break;
              }
            }
          }
          break;
        }
        case V_SgSwitchStatement : {
          if (LOG > 1) { log("Warning : Switch case statement is not handle yet.", (*it)); }
          SgSwitchStatement* switchStmt = isSgSwitchStatement(*it);

          if (isSgExprStatement(switchStmt->get_item_selector ())) {
            SgExpression* expstmt = isSgExprStatement(switchStmt->get_item_selector())->get_expression();
            // check if the item selector is known, 
            // if it's the case, delete all case not usefull and also the switch, 
            // just leave the right body (without the break it it exist).
          
          }
          else if (SgExpression* expr = isSgExpression(switchStmt->get_item_selector ())) {
            // check if the item selector is known,
            // if it's the case, delete all case not usefull and also the switch,
            // just leave the right body;(without the break it it exist)

          } else {
            std::cout << " we not handle the " 
                      << switchStmt->get_item_selector ()->class_name() 
                      << " class yet to the constante propagation transformation!\n";
          }

          if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body()))
            dead_code_elimination(subbody);
          break;
        }
        case V_SgCaseOptionStmt : {
          SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(*it);
          if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body())) { dead_code_elimination(subbody); }
          break;
        }
        case V_SgDefaultOptionStmt :  {
          SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(*it);
          if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body())) { dead_code_elimination(subbody); }
          break;
        }
        case V_SgScopeStatement : {
          if (LOG > 1) { log("This kind of scope is not handle yet : " + (*it)->class_name(), (*it)); }
          return;
          break;
        }
      }
    }
  } while (transformation_applied);
}

void dead_code_elimination(SgBasicBlock* body, std::vector<variable_spe*> varspe) {
  if(DEBUG) DBG_MAQAO
  if(!body) return;

  bool dead_code_here;
  bool transformation_applied; // will be used to determine how much time we have to run the deadcodeelimination
  do {
    transformation_applied = false;
    SgStatementPtrList & stmtsList = body->get_statements();

    for (int it = 0; it < stmtsList.size(); it++) {
      dead_code_here = false;
      switch((stmtsList[it])->variantT()) {
        case V_SgForStatement : {
          SgForStatement  * loop = isSgForStatement(stmtsList[it]);
          if(SgExprStatement * exprStmt = isSgExprStatement(loop->get_test ())) {
            if (isAlwaysFalse(exprStmt->get_expression(),varspe)) {
              dead_code_here = true;
              transformation_applied = false;
              if (LOG > 1) { log("Condition of the \"for loop\" is always false, the statement will be remove : ", (stmtsList[it])); }
              // /!\ DELETE THE WHOLE LOOP !
              it--;
              body->remove_statement(stmtsList[it+1]);
            }
          }
          //If no dead code look inside
          if (!dead_code_here) {
            if (SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body())) {
              dead_code_elimination(subbody, varspe);
            }
          }
          if (isSgBasicBlock(loop->get_loop_body())->get_statements().empty()) {
            if (LOG > 1) { log("The \"for loop\" is empty, the statement will be remove : ", (stmtsList[it])); }
            it--;
            body->remove_statement(stmtsList[it+1]);
          }

          break;
        }
        case V_SgFortranDo : {
          SgFortranDo  * loop = isSgFortranDo(stmtsList[it]);
          
          if( loop->get_body()->get_statements().empty()) {
            dead_code_here = true;
            transformation_applied = true;

            if (LOG > 1) { log("The \"do loop\" is empty, the statement will be remove : ", (stmtsList[it])); }
            // /!\ DELETE THE WHOLE LOOP ! because the loop is empty
            SageInterface::removeStatement(stmtsList[it], false);
            break;
          } 

          if( SgExpression* bound =  isSgExpression(loop->get_bound ())) {
            //Check if dead code here !
            SgExpression* loop_init = loop->get_initialization ();
            if (isSgAssignOp(loop_init)) {
              if(isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand())) {
                if(isAlwaysFalse(SageBuilder::buildLessOrEqualOp(isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand()), bound),varspe)) {
                  dead_code_here = true;
                  transformation_applied = true;
                  if (LOG > 1) { log("Condition of the \"do loop\" is always false, the statement will be remove : ", (stmtsList[it])); }
                  // /!\ DELETE THE WHOLE LOOP !
                  it--;
                  body->remove_statement(stmtsList[it+1]);
                } else { // if we had only one loop's iteration.
                  if(isAlwaysTrue(SageBuilder::buildEqualityOp(isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand()), bound),varspe)) {
                    dead_code_here = true;
                    transformation_applied = true;
                    // /!\ REPLACE THE LOOP BY ITS BODY !
                    it--;
                    
                    s2s_api::constantPropagation(body, 
                      isSgVarRefExp(isSgAssignOp(loop_init)->get_lhs_operand())->get_symbol(),  // Symbol of the loop's iterator
                                    isSgIntVal(isSgAssignOp(loop_init)->get_rhs_operand())->get_value()); //value of the loop's iterator

                    if (LOG > 1) { log("The \"do loop\" contains only one iteration, the loop will be replace by its body : ", (stmtsList[it+1])); }
                    SgBasicBlock * bodyReplacement = isSgBasicBlock(SageInterface::copyStatement(loop->get_body()));
                    bodyReplacement->set_parent(stmtsList[it+1]->get_parent());
                    SageInterface::insertStatementAfter(stmtsList[it+1], bodyReplacement);

                    body->remove_statement(stmtsList[it+1]);
                  }
                }
              }
            }
          }
          
          //If no dead code look inside
          if (!dead_code_here) {
            dead_code_elimination(loop->get_body(), varspe);
          }
          if (loop->get_body()->get_statements().empty()){
            if (LOG > 1) { log("The \"do loop\" is empty, the statement will be remove : ", (stmtsList[it])); }
            it--;
            body->remove_statement(stmtsList[it+1]);
          }

          break;
        }
        case V_SgDoWhileStmt : {
          SgDoWhileStmt  * loop = isSgDoWhileStmt(stmtsList[it]);
          //look inside anyway because it have to e done at least once.
          SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
          if(subbody)
            dead_code_elimination(subbody, varspe);
          // If dead code because the conditionnal, juste erase the loop but not the body because it have to be done once at least
          if(SgExprStatement * exprStmt = isSgExprStatement(loop->get_condition())) {
            if (isAlwaysFalse(exprStmt->get_expression(), varspe)) {
              dead_code_here = true;
              transformation_applied = true;
              if (LOG > 1) { log("Condition of the \"do while loop\" is always false, the statement will be remove : ", (stmtsList[it])); }

              // /!\ DELETE THE WHOLE LOOP !
              //AND LEAVE ON ITERATION
              SageInterface::removeStatement(loop, false);
            }
          }
          if (isSgBasicBlock(loop->get_body())->get_statements().empty()){
            if (LOG > 1) { log("The \"do while loop\" is empty, the statement will be remove : ", (stmtsList[it])); }
            it--;
            body->remove_statement(stmtsList[it+1]);
          }
          break;
        }
        case V_SgWhileStmt : {
          SgWhileStmt  * loop = isSgWhileStmt(stmtsList[it]);
          if (isSgExprStatement(loop->get_condition ())) {
            SgExpression* expr = isSgExprStatement(loop->get_condition ())->get_expression();
            // check if dead code
            if (isAlwaysFalse(expr,varspe)) {
              dead_code_here = true;
              transformation_applied = true;

              if (LOG > 1) { log("Condition of the \"while loop\" is always false, the statement will be remove : ", (stmtsList[it])); }
              // /!\ DELETE THE WHOLE LOOP !
              SageInterface::removeStatement(loop, false);
            }
          }
          // if no dead code look inside
          if(!dead_code_here) {
            SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
            if(subbody) {
              dead_code_elimination(subbody, varspe); 
            }
          }
          if (isSgBasicBlock(loop->get_body())->get_statements().empty()){
            if (LOG > 1) { log("The \"while loop\" is empty, the statement will be remove : ", (stmtsList[it])); }
            it--;
            body->remove_statement(stmtsList[it+1]);
          }
          break;
        }
        case V_SgIfStmt : {
          SgIfStmt * ifstmt = isSgIfStmt(stmtsList[it]);
          SgBasicBlock * trueBody = isSgBasicBlock(ifstmt->get_true_body());
          SgBasicBlock * falseBody = isSgBasicBlock(ifstmt->get_false_body());
          SgExpression * cond = isSgExprStatement(ifstmt->get_conditional())->get_expression();
          if (trueBody && falseBody) {
            if (trueBody->get_statements().empty() && falseBody->get_statements().empty()) {
              if (LOG > 1) { log("the \"if\" statement is empty (true and false body are empty), remove : ", (stmtsList[it])); }
              SageInterface::removeStatement(ifstmt, false);
              break;
            } 
          }

          //Get MAQAO directive to see if they're any ANALYZE (IF / IFELSE / ELSE) and maybe remove the if 
          std::vector<std::string> dirList = get_MAQAO_directives(ifstmt);
          if (dirList.size() > 0) {
            bool isRemoved = false;
            for (int i=0; i < dirList.size(); i++) {
              int loc = dirList[i].find ("MAQAO ANALYZE");
              if (loc != std::string::npos) {
                std::string kind  = dirList[i].substr(loc+14, dirList[i].find (" -")-(loc+14));
                std::string label = dirList[i].substr(dirList[i].find ("\"")+1, (dirList[i].find ("\" ")-(dirList[i].find("\"")+1)));
                if (kind == "IF") {
                  for (int i = 0; i < varspe.size(); i++) {
                    if (std::string(varspe[i]->var_name) == "ifthen" && std::string(varspe[i]->label) == label) {
                      if (varspe[i]->value == 0) {
                        it -= remove_MAQAO_directives(ifstmt);
                        it--;
                        body->remove_statement(stmtsList[it+1]);
                        isRemoved = true;
                        break;
                      }
                    }
                  }
                }  else if (kind == "IFELSE") {
                  int ifthenValue;
                  int ifelseValue;
                  for (int i = 0; i < varspe.size(); i++) {
                    if (std::string(varspe[i]->var_name) == "ifthen" && std::string(varspe[i]->label) == label) {
                      ifthenValue = varspe[i]->value;
                    } else if (std::string(varspe[i]->var_name) == "ifelse" && std::string(varspe[i]->label) == label) {
                      ifelseValue = varspe[i]->value;
                    }
                  }
                  // if ithen is == 0 
                  if (!ifthenValue) {
                    //Check the false body and replace by it
                    dead_code_elimination(falseBody, varspe);
                    if (falseBody->get_statements().empty()) {
                      if (LOG > 1) { log("the \"if\" statement is empty (true and false body are empty), remove : ", (stmtsList[it])); }
                      it -= remove_MAQAO_directives(ifstmt);
                      it--;
                      body->remove_statement(stmtsList[it+1]);
                      isRemoved = true;
                      break;
                    } else {
                      if (LOG > 1) { log("the then path of the \"if\" statement is never took, replace the if by the else body ", (stmtsList[it])); }
                      SageInterface::replaceStatement(ifstmt, SageInterface::copyStatement(falseBody));
                      isRemoved = true;
                      break;
                    }
                  // if ifelse is == 0
                  } else if (!ifelseValue) {
                    dead_code_elimination(trueBody, varspe);
                    if (trueBody->get_statements().empty()) {
                      it -= remove_MAQAO_directives(ifstmt);
                      it--;
                      body->remove_statement(stmtsList[it+1]);
                      isRemoved = true;
                      break;
                    } else {
                      if (LOG > 1) { log("the else path of the \"if\" statement is never took, replace the if by the then body ", (stmtsList[it])); }
                      SageInterface::replaceStatement(ifstmt, SageInterface::copyStatement(falseBody)); 
                      isRemoved = true;
                      break;
                    } 
                  }
                } else {
                  DBG_MAQAO
                  std::cout << "[dead_code_elimination] Error: kind has a weird status" << std::endl;
                }
              }
            }
            //Their no if left
            if (isRemoved) break;
          }

          // Check if the "then" and the "else" body are empty
          if (trueBody) {
            if (trueBody->get_statements().empty()) {
              if (LOG > 1) { log("the true body is empty, replace it by the false body (and reverse condition) : ", (stmtsList[it])); }
              dead_code_elimination(falseBody, varspe);
              ifstmt->set_conditional(SageBuilder::buildExprStatement(SageBuilder::buildNotOp(isSgExprStatement(ifstmt->get_conditional())->get_expression ())));
              ifstmt->set_true_body(SageInterface::copyStatement(falseBody));

              SageInterface::removeStatement(ifstmt->get_false_body(), false);
              break;
            }
          }
          if(falseBody) {
            if (falseBody->get_statements().empty()) {              
              dead_code_elimination(trueBody, varspe);

              if (trueBody->get_statements().empty()) {

                if (LOG > 1) { log("the \"if\" statement is empty (true and false body are empty), remove : ", (stmtsList[it])); }
                SageInterface::removeStatement(ifstmt, false);
                break;
              }
            }
          }
          if(cond) {
            if (isAlwaysTrue(cond,varspe)) {
              dead_code_elimination(trueBody, varspe);
              if (LOG > 1) { log("Condition  of the \"if statement\" is always true, replace the \"if statement\" by its true body : ", (stmtsList[it])); }
              SageInterface::replaceStatement(ifstmt, SageInterface::copyStatement(trueBody));
            
            } else if (isAlwaysFalse(cond,varspe)) {
              dead_code_elimination(falseBody, varspe);
              if (LOG > 1) { log("Condition  of the \"if statement\" is always false, replace the \"if statement\" by its false body : ", (stmtsList[it])); }
              SageInterface::replaceStatement(ifstmt, SageInterface::copyStatement(falseBody));
            
            }
            else {
              if (LOG > 1) { log("\nThe condition of the \"if statement\" is indeterminable (not always true or false) : ", (stmtsList[it])); }
              if(trueBody) {
                dead_code_elimination(trueBody, varspe);
              }
              if (falseBody) {
                dead_code_elimination(falseBody, varspe);
              }

              // Check if the dead_code elimination don't erase all the statements inside one or both bodies.
              if (trueBody && falseBody)
              if( trueBody->get_statements().empty() && falseBody->get_statements().empty()) {
                if (LOG > 1) { log("After elimination of dead code in the \"if statement\" the true and false body became empty, remove the \"if statement\" : ", (stmtsList[it])); }
                SageInterface::removeStatement(ifstmt, false);
                break;
              
              } 
              if(trueBody)
              if (trueBody->get_statements().empty() ) {
                //dead_code_elimination(falseBody, varspe);
                ifstmt->set_conditional(SageBuilder::buildExprStatement(SageBuilder::buildNotOp(isSgExprStatement(ifstmt->get_conditional())->get_expression ())));
                ifstmt->set_true_body(SageInterface::copyStatement(falseBody));
                
                if (LOG > 1) { log("After elimination of dead code in the \"if statement\" the true body is empty, replace it by the false body (and reverse condition) : ", (stmtsList[it])); }
                SageInterface::removeStatement(ifstmt->get_false_body(), false);
                ifstmt->set_false_body(NULL);
                break;
              
              } 
              if(falseBody)
              if (falseBody->get_statements().empty()) {
                if(LOG > 1) std::cout << "only false is empty l." << ifstmt->get_file_info()->get_line() << std::endl;
                //dead_code_elimination(trueBody, varspe);
                break;
              }
            }
          }
          break;
        }
        case V_SgSwitchStatement : {
          if (LOG > 1) { log("Warning : Switch case statement is not handle yet.", (stmtsList[it])); }
          SgSwitchStatement* switchStmt = isSgSwitchStatement(stmtsList[it]);

          if (isSgExprStatement(switchStmt->get_item_selector ())) {
            SgExpression* expstmt = isSgExprStatement(switchStmt->get_item_selector())->get_expression();
            // check if th item selector is known, 
            // if it's the case, delete all case not usefull and also the switch, 
            // just leave the right body (without the break it it exist).
          
          }
          else if (SgExpression* expr = isSgExpression(switchStmt->get_item_selector ())) {
            // check if th item selector is known,
            // if it's the case, delete all case not usefull and also the switch,
            // just leave the right body;(without the break it it exist)

          } else {
            std::cout << "Warning : we not handle the " 
                      << switchStmt->get_item_selector ()->class_name() 
                      << " class yet to the constante propagation transformation!\n";
          }

          if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body())) {
            dead_code_elimination(subbody, varspe);
          }
          break;
        }
        case V_SgCaseOptionStmt : {
          SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(stmtsList[it]);
          if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body())) {
            dead_code_elimination(subbody, varspe);
          }
          break;
        }
        case V_SgDefaultOptionStmt :  {
          SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(stmtsList[it]);
          if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body())) {
            dead_code_elimination(subbody, varspe);
          }
          break;
        }
        case V_SgScopeStatement : {
          if (LOG > 1) 
          { log("This kind of scope is not handle yet : " + (stmtsList[it])->class_name(), (stmtsList[it])); }
          // return;
          break;
        }
        case V_SgBasicBlock: {
          SgBasicBlock* subbody = isSgBasicBlock(stmtsList[it]);
          dead_code_elimination(subbody, varspe);
        }
      }
    }
  } while (transformation_applied);
}

//////////////////////////////
// Condition eval functions //
//////////////////////////////
/** 
 * Only "isAlwaysTrue(SgExpression* expr, std::vector<variable_spe*> varspe)" is use now.
 */
bool isAlwaysTrue(SgExpression* expr) {
  if(DEBUG) DBG_MAQAO
  //check if the expression is 0 or false or the value is == 0
  if (SageInterface::isConstantTrue (expr)) return true;
  if (SageInterface::isConstantFalse(expr)) return false;

  //Check if the comparisson is True !
  if (SgBinaryOp* bo = isSgBinaryOp(expr)) {
    SgExpression * lhs = bo->get_lhs_operand();
    SgExpression * rhs = bo->get_rhs_operand();
    
    if(isSgVarRefExp(bo->get_lhs_operand())) {
      if(SgIntVal* possible_lhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_lhs_operand()))) {
        lhs = possible_lhs;
      }
    }
    if(isSgVarRefExp(bo->get_rhs_operand())) {
      if(SgIntVal* possible_rhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_rhs_operand()))) { 
        rhs = possible_rhs;
      }
    }

    switch (bo->variantT()) {
      case V_SgEqualityOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) {
         return isSgIntVal(lhs)->get_value () == isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgNotEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) {
          return isSgIntVal(lhs)->get_value () != isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgGreaterOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { 
          return isSgIntVal(lhs)->get_value () >= isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgGreaterThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { 
          return isSgIntVal(lhs)->get_value () > isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgLessOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { 
          return isSgIntVal(lhs)->get_value () <= isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgLessThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { 
          return isSgIntVal(lhs)->get_value () < isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgAndOp: {
       return isAlwaysTrue(lhs) && isAlwaysTrue(rhs);
        break;  
      }
      case V_SgOrOp: {
       return isAlwaysTrue(lhs) || isAlwaysTrue(rhs);
        break;
      }
    }
  }
  if (SgUnaryOp* uo = isSgUnaryOp(expr)) {
    switch (uo->variantT()) {
      case V_SgNotOp : 
        return isAlwaysFalse(uo->get_operand());
        break;
    }
  }
  //We don't know if the expression is always true
  return false;
}

bool isAlwaysTrue(SgExpression* expr, std::vector<variable_spe*> varspe) {
  if(DEBUG) DBG_MAQAO

  //check if the expression is 0 or false or the value is == 0
  if (SageInterface::isConstantTrue (expr)) return true;
  if (SageInterface::isConstantFalse(expr)) return false;

  //Check if the comparisson is True !
  if (SgBinaryOp* bo = isSgBinaryOp(expr)) {
    SgExpression * lhs = bo->get_lhs_operand();
    SgExpression * rhs = bo->get_rhs_operand();
    variable_spe * varlhs = NULL; 
    variable_spe * varrhs = NULL; 
    
    if(isSgVarRefExp(lhs)) {
      if(SgIntVal* possible_lhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_lhs_operand()))) {
        lhs = possible_lhs;
      }
      else {
        varlhs = find(isSgVarRefExp(lhs), varspe);
      }
    }

    if(isSgVarRefExp(rhs)) {
      if(SgIntVal* possible_rhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_rhs_operand()))) {
        rhs = possible_rhs;
      }
      else {
        varrhs = find(isSgVarRefExp(rhs), varspe);
      }
    }

    switch (bo->variantT()) {
      case V_SgEqualityOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () == isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgNotEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () != isSgIntVal(rhs)->get_value (); }
        
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::INF) {
            if (varrhs->specialization == variable_spe::SUP || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->sup_bound <= varrhs->inf_bound;
            }
          }
          if (varlhs->specialization == variable_spe::SUP) {
            if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->inf_bound >= varrhs->sup_bound;
            } 
          }
          if (varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::INF) {return varlhs->inf_bound >= varrhs->sup_bound;}
            else if (varlhs->specialization == variable_spe::SUP) {return varlhs->sup_bound <= varrhs->inf_bound;}
            else if (varlhs->specialization == variable_spe::INTERVAL) {
              return ((varlhs->inf_bound >= varrhs->sup_bound)||(varlhs->sup_bound <= varrhs->inf_bound));
            }
          }
        }
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::INF) {
            return varlhs->sup_bound <= isSgIntVal(rhs)->get_value ();
          }
          if (varlhs->specialization == variable_spe::SUP) {
            return varlhs->inf_bound >= isSgIntVal(rhs)->get_value ();
          }
          if (varlhs->specialization == variable_spe::INTERVAL) {
            return ((varlhs->sup_bound-1 < isSgIntVal(rhs)->get_value ()) 
                 || (varlhs->inf_bound+1 > isSgIntVal(rhs)->get_value ()));
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::INF) {
            return varrhs->sup_bound >= isSgIntVal(lhs)->get_value ();
          }
          if (varrhs->specialization == variable_spe::SUP) {
            return varrhs->inf_bound <= isSgIntVal(lhs)->get_value ();
          }
          if (varrhs->specialization == variable_spe::INTERVAL) {
            return ((varrhs->sup_bound-1 < isSgIntVal(lhs)->get_value ()) 
                 || (varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ()));
          }
        }
        break;
      }
      case V_SgGreaterOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () >= isSgIntVal(rhs)->get_value (); }
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::SUP ||  varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->inf_bound+1 >= varrhs->sup_bound-1;
            }
          }
        } 
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::SUP 
            || varlhs->specialization == variable_spe::INTERVAL) {
            return varlhs->inf_bound+1 >= isSgIntVal(rhs)->get_value ();
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::SUP) {
            return varrhs->inf_bound+1 <= isSgIntVal(lhs)->get_value ();
          } else if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->sup_bound-1 <= isSgIntVal(lhs)->get_value ();            
          }
        }
        break;
      }
      case V_SgGreaterThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () > isSgIntVal(rhs)->get_value (); }
        
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::SUP ||  varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->inf_bound+1 > varrhs->sup_bound-1;
            }
          }
        } 
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::SUP) {
            return varlhs->inf_bound >= isSgIntVal(rhs)->get_value ();
          } else if (varlhs->specialization == variable_spe::INTERVAL) {
            return (varlhs->inf_bound >= isSgIntVal(rhs)->get_value ());
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->sup_bound <= isSgIntVal(lhs)->get_value ();
          } else if (varrhs->specialization == variable_spe::SUP ) {
            return varrhs->inf_bound+1 < isSgIntVal(lhs)->get_value ();
          }
        }
        break;
      }
      case V_SgLessOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () <= isSgIntVal(rhs)->get_value (); }
        
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::INF 
            || varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::SUP 
              || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->sup_bound-1 <= varrhs->inf_bound+1;
            }
          }
        }

        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::INF 
            || varlhs->specialization == variable_spe::INTERVAL) {
            return varlhs->sup_bound-1 <= isSgIntVal(rhs)->get_value ();
          }
        }

        else if (varrhs && isSgIntVal(lhs)) {
          if ( varrhs->specialization == variable_spe::INF) {
            return varrhs->sup_bound-1 == isSgIntVal(lhs)->get_value ();
          }
          else if (varrhs->specialization == variable_spe::SUP ) {
            return varrhs->inf_bound+1 >= isSgIntVal(lhs)->get_value ();
          } else if (varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->inf_bound+1 >= isSgIntVal(lhs)->get_value ();
          }
        }
        break;
      }
      case V_SgLessThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () < isSgIntVal(rhs)->get_value (); }
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::INF || varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::SUP || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->sup_bound-1 < varrhs->inf_bound+1;
            } 
          }
        }
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::INF || varlhs->specialization == variable_spe::INTERVAL) {
            return varlhs->sup_bound-1 < isSgIntVal(rhs)->get_value ();
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::INF ) {
            return varrhs->sup_bound-1 > isSgIntVal(lhs)->get_value ();
          }
          else if (varrhs->specialization == variable_spe::SUP ) {
            return varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ();
          } else if (varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ();
          }
        }
        break;
      }
      case V_SgAndOp: {
        return isAlwaysTrue(lhs, varspe) && isAlwaysTrue(rhs, varspe);
        break;
      }
      case V_SgOrOp: {
        return isAlwaysTrue(lhs, varspe) || isAlwaysTrue(rhs, varspe);
        break;
      }
    }
  }
  if (SgUnaryOp* uo = isSgUnaryOp(expr)) {
    switch (uo->variantT()) {
      case V_SgNotOp : 
        return isAlwaysFalse(uo->get_operand(), varspe);
        break;
    }
  }
  //We don't know if the expression is always true
  return false;
}

/**
 * Only "isAlwaysFalse(SgExpression* expr, std::vector<variable_spe*> varspe)" is use now.
 */
bool isAlwaysFalse(SgExpression* expr)  {
  if(DEBUG) DBG_MAQAO
  //check if the expression is 0 or false or the value is == 0
  if (SageInterface::isConstantTrue (expr)) return false;
  if (SageInterface::isConstantFalse(expr)) return true;

  //Check if the comparisson is True !
  if (SgBinaryOp* bo = isSgBinaryOp(expr)) {
    SgExpression * lhs = bo->get_lhs_operand();
    SgExpression * rhs = bo->get_rhs_operand();
        
    if(isSgVarRefExp(bo->get_lhs_operand())) {
      if(SgIntVal* possible_lhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_lhs_operand()))) {
        lhs = possible_lhs;
      }
    }
    if(isSgVarRefExp(bo->get_rhs_operand())) {
      if(SgIntVal* possible_rhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_rhs_operand()))) {
        rhs = possible_rhs;
      }
    }

    switch (bo->variantT()) {
      case V_SgEqualityOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { 
          return isSgIntVal(lhs)->get_value () != isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgNotEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { 
          return isSgIntVal(lhs)->get_value () == isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgGreaterOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { 
          return isSgIntVal(lhs)->get_value () < isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgGreaterThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) {
          return isSgIntVal(lhs)->get_value () <= isSgIntVal(rhs)->get_value ();
        }
        break;
      }
      case V_SgLessOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) {
          return isSgIntVal(lhs)->get_value () > isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgLessThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) {
          return isSgIntVal(lhs)->get_value () >= isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgAndOp: {
        return isAlwaysFalse(lhs) || isAlwaysFalse(rhs);
        break;
      }
      case V_SgOrOp: {
        return isAlwaysFalse(lhs) && isAlwaysFalse(rhs);
        break;
      }
    }
  }
  if (SgUnaryOp* uo = isSgUnaryOp(expr)) {
    switch (uo->variantT()) {
      case V_SgNotOp : 
        return isAlwaysTrue(uo->get_operand());
      break;
    }
  }
  //We don't know if the expression is always true
  return false;
}

bool isAlwaysFalse(SgExpression* expr, std::vector<variable_spe*> varspe)  {
  if(DEBUG) DBG_MAQAO
  //check if the expression is 0 or false or the value is == 0
  if (SageInterface::isConstantTrue (expr)) return false;
  if (SageInterface::isConstantFalse(expr)) return true;

  //Check if the comparisson is True !
  if (SgBinaryOp* bo = isSgBinaryOp(expr)) {
    SgExpression * lhs = bo->get_lhs_operand();
    SgExpression * rhs = bo->get_rhs_operand();
    variable_spe * varlhs = NULL; 
    variable_spe * varrhs = NULL; 

    if(isSgVarRefExp(lhs)) {
      if(SgIntVal* possible_lhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_lhs_operand()))) {
        lhs = possible_lhs;
      }
      else {
        varlhs = find(isSgVarRefExp(lhs), varspe);
      }
    }

    if(isSgVarRefExp(rhs)) {
      if(SgIntVal* possible_rhs = trace_back_to_last_affectation(isSgVarRefExp(bo->get_rhs_operand()))) {
        rhs = possible_rhs;
      }
      else {
        varrhs = find(isSgVarRefExp(rhs), varspe);
      }
    }

    switch (bo->variantT()) {
      case V_SgEqualityOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () != isSgIntVal(rhs)->get_value (); }
        
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::INF) {
            if (varrhs->specialization == variable_spe::SUP || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->sup_bound <= varrhs->inf_bound;
            }
          }
          if (varlhs->specialization == variable_spe::SUP) {
            if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->inf_bound >= varrhs->sup_bound;
            } 
          }
          if (varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::INF) return varlhs->inf_bound >= varrhs->sup_bound;
            else if (varlhs->specialization == variable_spe::SUP) return varlhs->sup_bound <= varrhs->inf_bound;
            else if (varlhs->specialization == variable_spe::INTERVAL) {
              return ((varlhs->inf_bound  >= varrhs->sup_bound)
                     ||(varlhs->sup_bound <= varrhs->inf_bound));
            }
          }
        }
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::INF) {
            return varlhs->sup_bound-1 < isSgIntVal(rhs)->get_value ();
          }
          if (varlhs->specialization == variable_spe::SUP) {
            return varlhs->inf_bound+1 > isSgIntVal(rhs)->get_value ();
          }
          if (varlhs->specialization == variable_spe::INTERVAL) {
            return ((varlhs->sup_bound-1 < isSgIntVal(rhs)->get_value ()) 
                 || (varlhs->inf_bound+1 > isSgIntVal(rhs)->get_value ()));
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::INF) {
            return varrhs->sup_bound-1 < isSgIntVal(lhs)->get_value ();
          }
          if (varrhs->specialization == variable_spe::SUP) {
            return varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ();
          }
          if (varrhs->specialization == variable_spe::INTERVAL) {
            return ((varrhs->sup_bound-1 < isSgIntVal(lhs)->get_value ()) 
                 || (varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ()));
          }
        }
        break;
      }
      case V_SgNotEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () == isSgIntVal(rhs)->get_value (); }
        break;
      }
      case V_SgGreaterOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () < isSgIntVal(rhs)->get_value (); }
        
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::INF || varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::SUP || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->sup_bound-1 < varrhs->inf_bound+1;
            } 
          }
        }
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::INF || varlhs->specialization == variable_spe::INTERVAL) {
            return varlhs->sup_bound-1 < isSgIntVal(rhs)->get_value ();
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::SUP ) {
            return varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ();
          } else if (varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ();
          }
        }
        break;
      }
      case V_SgGreaterThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () <= isSgIntVal(rhs)->get_value (); }
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::INF || varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::SUP || varrhs->specialization == variable_spe::INTERVAL) 
              return varlhs->sup_bound-1 <= varrhs->inf_bound+1;
          }
        }
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::INF || varlhs->specialization == variable_spe::INTERVAL) {
            return varlhs->sup_bound-1 <= isSgIntVal(rhs)->get_value ();
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::SUP ) {
            return varrhs->inf_bound+1 >= isSgIntVal(lhs)->get_value ();
          } else if (varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->inf_bound+1 >= isSgIntVal(lhs)->get_value ();
          }
        }
        break;
      }
      case V_SgLessOrEqualOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () > isSgIntVal(rhs)->get_value (); }
        
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::SUP ||  varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->inf_bound+1 > varrhs->sup_bound-1;
            }
          }
        } 
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::SUP) {
            return varlhs->inf_bound >= isSgIntVal(rhs)->get_value ();
          } else if (varlhs->specialization == variable_spe::INTERVAL) {
            return (varlhs->inf_bound >= isSgIntVal(rhs)->get_value ());
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->sup_bound <= isSgIntVal(lhs)->get_value ();
          }else if (varrhs->specialization == variable_spe::SUP ) {
            return varrhs->inf_bound+1 > isSgIntVal(lhs)->get_value ();
          }
        }
        break;
      }
      case V_SgLessThanOp: {
        if (isSgIntVal(lhs) && isSgIntVal(rhs)) { return isSgIntVal(lhs)->get_value () >= isSgIntVal(rhs)->get_value (); }
        if (varlhs && varrhs) {
          if (varlhs->specialization == variable_spe::SUP ||  varlhs->specialization == variable_spe::INTERVAL) {
            if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
              return varlhs->inf_bound+1 >= varrhs->sup_bound-1;
            }
          }
        } 
        else if (varlhs && isSgIntVal(rhs)) {
          if (varlhs->specialization == variable_spe::SUP 
            || varlhs->specialization == variable_spe::INTERVAL) {
            return varlhs->inf_bound+1 >= isSgIntVal(rhs)->get_value ();
          }
        }
        else if (varrhs && isSgIntVal(lhs)) {
          if (varrhs->specialization == variable_spe::SUP) {
            return varrhs->inf_bound+1 >= isSgIntVal(lhs)->get_value ();
          } else if (varrhs->specialization == variable_spe::INF || varrhs->specialization == variable_spe::INTERVAL) {
            return varrhs->sup_bound-1 <= isSgIntVal(lhs)->get_value ();            
          }
        }
        break;
      }
      case V_SgAndOp: {
        return isAlwaysFalse(lhs, varspe) || isAlwaysFalse(rhs, varspe);
        break;
      }
      case V_SgOrOp: {
        return isAlwaysFalse(lhs, varspe) && isAlwaysFalse(rhs, varspe);
        break;
      }
    }
  }
  if (SgUnaryOp* uo = isSgUnaryOp(expr)) {
    switch (uo->variantT()) {
      case V_SgNotOp : 
        return isAlwaysTrue(uo->get_operand(), varspe);
      break;
    }
  }
  //We don't know if the expression is always true
  return false;
}

////////////////////////////
// Localization functions //
////////////////////////////
SgFunctionDeclaration* findFunction(SgScopeStatement* scope, std::string funcName) {
  if (DEBUG) DBG_MAQAO
  SgFunctionDeclaration* funcDecl = SageInterface::findFirstDefiningFunctionDecl(scope);

  while (funcDecl) {
    if (funcDecl->get_name().getString() == funcName) {
      return funcDecl;
    } 
    
    SgStatement* nextStmt = SageInterface::getNextStatement (funcDecl);
    while (isSgPragmaDeclaration(nextStmt) && nextStmt) {
      nextStmt = SageInterface::getNextStatement (nextStmt);
    }

    funcDecl = isSgFunctionDeclaration(nextStmt);    
    if (DEBUG && nextStmt) std::cout << nextStmt->class_name() << std::endl;
  }
  return NULL;
}

/** 
 * INTERNAL USED 
 * check if a node is in a loop
 */
bool is_in_loop(SgNode * astNode) {
  if(DEBUG) DBG_MAQAO
  const SgNode* parentNode = astNode;
  while ( ((isSgFortranDo(parentNode) == NULL) 
            || (isSgWhileStmt(parentNode) == NULL) 
            || (isSgDoWhileStmt(parentNode) == NULL) 
            || (isSgForStatement(parentNode) == NULL))
          && (parentNode->get_parent() != NULL) )
  {
    parentNode = parentNode->get_parent();
  }
  if ((isSgFortranDo(parentNode) == NULL) 
      || (isSgWhileStmt(parentNode) == NULL) 
      || (isSgDoWhileStmt(parentNode) == NULL) 
      || (isSgForStatement(parentNode) == NULL))
    return true;
  else 
    return false; 
}

/** 
 * INTERNAL USED 
 * check if a variable is assigned in a specific basic block 
 */
bool var_is_lhs_scope(SgBasicBlock* body, SgVarRefExp * vre) {
  if(DEBUG) DBG_MAQAO
  if(!body) return false;
  int vreLine = vre->get_file_info()->get_line();
  
  SgStatementPtrList & stmtsList = body->get_statements();
  for (SgStatementPtrList::iterator it = stmtsList.begin(); it != stmtsList.end(); ++it) {
    if (SgExprStatement* exprStmt = isSgExprStatement(*it)) {
      if (SgBinaryOp* bo =isSgBinaryOp(exprStmt->get_expression())) { 
        if (isSgIntVal(bo->get_rhs_operand()) && isSgVarRefExp(bo->get_lhs_operand())) {
          if (vre->get_symbol () == isSgVarRefExp(bo->get_lhs_operand())->get_symbol())
              return true;
        }
      }
    //TODO : if scope is met check if there no affectation with vre ash lhs inside or returnValue take NULL
    } else if (SgIfStmt * ifstmt = isSgIfStmt(*it)) {
      SgBasicBlock * trueBody = isSgBasicBlock(ifstmt->get_true_body());
      SgBasicBlock * falseBody = isSgBasicBlock(ifstmt->get_false_body());
      if (var_is_lhs_scope(trueBody, vre)) return true;
      if (var_is_lhs_scope(falseBody, vre)) return true;

    } else if (SgDoWhileStmt * dowhilestmt  = isSgDoWhileStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(dowhilestmt->get_body());
      if (var_is_lhs_scope(body, vre)) return true;

    } else if (SgWhileStmt * whilestmt  = isSgWhileStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(whilestmt->get_body());
      if (var_is_lhs_scope(body, vre)) return true;

    } else if (SgFortranDo * fortrando  = isSgFortranDo (*it)) {
      SgBasicBlock * body = isSgBasicBlock(fortrando->get_body());
      if (var_is_lhs_scope(body, vre)) return true;

    } else if (SgForStatement * forstmt  = isSgForStatement (*it)) {
      SgBasicBlock * body = isSgBasicBlock(forstmt->get_loop_body());
      if (var_is_lhs_scope(body, vre)) return true;

    } else if (SgSwitchStatement * switchstmt  = isSgSwitchStatement (*it)) {
      SgBasicBlock * body = isSgBasicBlock(switchstmt->get_body ());
      if (var_is_lhs_scope(body, vre)) return true;

    } else if (SgCaseOptionStmt * caseoption  = isSgCaseOptionStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(caseoption->get_body ());
      if (var_is_lhs_scope(body, vre)) return true;

    } else if (SgDefaultOptionStmt * defaultoption  = isSgDefaultOptionStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(defaultoption->get_body ());
      if (var_is_lhs_scope(body, vre)) return true;

    } else if(SgScopeStatement * scope = isSgScopeStatement(*it)) {
    }
  }
  return false;
}

SgIntVal* trace_back_to_last_affectation(SgVarRefExp * vre) {
  if(DEBUG) DBG_MAQAO
  if(isSgPntrArrRefExp(vre)) return NULL;
  if(is_in_loop(vre)) return NULL;

  SgFunctionDefinition * func = get_function_RosePtr(vre);
  if(!func) return NULL;
  
  SgIntVal * returnValue = NULL;
  int vreLine = vre->get_file_info()->get_line();
  
  SgStatementPtrList & stmtsList = func->get_body()->get_statements();
  for (SgStatementPtrList::iterator it = stmtsList.begin(); 
       (it != stmtsList.end() && (*it)->get_file_info()->get_line() < vreLine); 
       ++it) 
  {
    if (SgExprStatement* exprStmt = isSgExprStatement(*it)) {
      if (SgBinaryOp* bo =isSgBinaryOp(exprStmt->get_expression())) { 
        if (isSgIntVal(bo->get_rhs_operand()) && isSgVarRefExp(bo->get_lhs_operand())) {
          if (vre->get_symbol () == isSgVarRefExp(bo->get_lhs_operand())->get_symbol())
            returnValue = isSgIntVal(bo->get_rhs_operand());
        }
      }
    } else if (SgIfStmt * ifstmt = isSgIfStmt(*it)) {
      SgBasicBlock * trueBody = isSgBasicBlock(ifstmt->get_true_body());
      SgBasicBlock * falseBody = isSgBasicBlock(ifstmt->get_false_body());
      if (var_is_lhs_scope(trueBody, vre)) returnValue = NULL;
      if (var_is_lhs_scope(falseBody, vre)) returnValue = NULL;
    } else if (SgDoWhileStmt * dowhilestmt  = isSgDoWhileStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(dowhilestmt->get_body());
      if (var_is_lhs_scope(body, vre)) returnValue = NULL;
    } else if (SgWhileStmt * whilestmt  = isSgWhileStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(whilestmt->get_body());
      if (var_is_lhs_scope(body, vre)) returnValue = NULL;
    } else if (SgFortranDo * fortrando  = isSgFortranDo (*it)) {
      SgBasicBlock * body = isSgBasicBlock(fortrando->get_body());
      if (var_is_lhs_scope(body, vre)) returnValue = NULL;
    } else if (SgForStatement * forstmt  = isSgForStatement (*it)) {
      SgBasicBlock * body = isSgBasicBlock(forstmt->get_loop_body());
      if (var_is_lhs_scope(body, vre)) returnValue = NULL;
    } else if (SgSwitchStatement * switchstmt  = isSgSwitchStatement (*it)) {
      SgBasicBlock * body = isSgBasicBlock(switchstmt->get_body ());
      if (var_is_lhs_scope(body, vre)) returnValue = NULL;
    } else if (SgCaseOptionStmt * caseoption  = isSgCaseOptionStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(caseoption->get_body ());
      if (var_is_lhs_scope(body, vre)) returnValue = NULL;
    } else if (SgDefaultOptionStmt * defaultoption  = isSgDefaultOptionStmt (*it)) {
      SgBasicBlock * body = isSgBasicBlock(defaultoption->get_body ());
      if (var_is_lhs_scope(body, vre)) returnValue = NULL;
    } else if(SgScopeStatement * scope = isSgScopeStatement(*it)) {
      if (DEBUG > 1) DBG_MAQAO
    }
  }
  return returnValue;
}

/** 
 * INTERNAL USED 
 * check if a variable is assigned in a specific basic block 
 */
bool isinExpr(SgExpression * expr, SgVariableSymbol* vsymb) {
  if(DEBUG) DBG_MAQAO
  if(!expr || !vsymb) return false;

  if (SgBinaryOp * bo = isSgBinaryOp(expr)) {
    if (isinExpr(bo->get_rhs_operand(), vsymb)) return true;
    if (isinExpr(bo->get_lhs_operand(), vsymb)) return true;
  }
  else if (SgFunctionCallExp * fcallexp = isSgFunctionCallExp(expr)) {
    SgExpressionPtrList argList = fcallexp->get_args ()->get_expressions ();
    for (SgExpressionPtrList::iterator itArgList = argList.begin(); itArgList != argList.end(); ++itArgList) {
      if(isinExpr ((*itArgList), vsymb)) return true;
    }
  }
  else if (isSgExprListExp(expr)) {
    SgExpressionPtrList expList = isSgExprListExp(expr)->get_expressions();
    for (SgExpressionPtrList::iterator it = expList.begin(); it != expList.end(); it++) {
      if (isinExpr ((*it), vsymb)) 
        return true;
    }
  }
  else if(SgVarRefExp* varref = isSgVarRefExp(expr)) {
    if(varref->get_symbol()->get_name().getString() == vsymb->get_name().getString()) 
      return true;
  } 
  else if (isSgValueExp(expr) || isSgNullExpression(expr)) {
    return false;
  } 
  else {
    if (DEBUG) DBG_MAQAO
    std::cout << "The class "  << expr->class_name() << " is not handle yet " << std::endl;
  }
  return false;
}

SgBasicBlock* copyBasicBlock(SgBasicBlock* body, std::vector<SgSymbol*> v_symbol, std::vector<int> val){
  if(DEBUG) DBG_MAQAO
  SgBasicBlock * bodycpy = new SgBasicBlock();
  SageInterface::setOneSourcePositionForTransformation(bodycpy);

  SgStatementPtrList body_stmt_list = body->get_statements();
  for (SgStatementPtrList::iterator it = body_stmt_list.begin(); it != body_stmt_list.end(); ++it) 
  {
    if (SgVariableDeclaration* vardecl = isSgVariableDeclaration(*it)) {

      // Extract variables and check the type
      SgInitializedNamePtrList varlist =  vardecl->get_variables();
      for (int j = 0; j < varlist.size(); j++) {
        if(isSgArrayType(varlist[j]->get_type())) {
          // if(isSgArrayType(vardecl->get_definition()->get_type())) {

          SgExprListExp * ele = isSgArrayType(varlist[j]->get_type())->get_dim_info();

          SgExpressionPtrList expListPtr = ele->get_expressions();
          SgExprListExp * new_ele = new SgExprListExp();
          SageInterface::setOneSourcePositionForTransformation(new_ele);

          // Copy all parameter of the list of expression
          for (int i =0; i < expListPtr.size(); i++) {
            new_ele->append_expression(SageInterface::copyExpression(expListPtr[i]));
          }

          // Replace the variable by the constant anywhere in the declaration (in the new list of expression extract previously)
          SgExpressionPtrList & expreList =  new_ele->get_expressions ();
          for(int i=0; i < v_symbol.size(); i++) {
            for(SgExpressionPtrList::iterator it = expreList.begin(); it != expreList.end(); ++it) {
              replace_expr((*it), isSgVariableSymbol(v_symbol[i]), SageBuilder::buildIntVal(val[i]));
            }   
          }

          //Create the the type (to duplicate without pointer problems)
          SgArrayType * arraytype = new SgArrayType();
          arraytype->set_base_type (isSgArrayType(varlist[j]->get_type())->get_base_type ());
          arraytype->set_index (isSgArrayType(varlist[j]->get_type())->get_index ());
          arraytype->set_dim_info (new_ele);
          arraytype->set_rank (new_ele->get_expressions().size());

          //Create the first variable of the declaration
          SgInitializedName * var_to_append = SageBuilder::buildInitializedName(
                      varlist[j]->get_name(), 
                      arraytype, 
                      varlist[j]->get_initializer ());
          
          // Create the declaration
          SgVariableDeclaration * nd = new SgVariableDeclaration(
                      var_to_append->get_name(), 
                      arraytype, 
                      var_to_append->get_initializer ());
          
          // Set the dimention of the array
          isSgArrayType(nd->get_definition()->get_type())->set_dim_info(new_ele);
          // Set differents parameters
          nd->get_declarationModifier().get_typeModifier() = varlist[j]->get_declaration()->get_declarationModifier().get_typeModifier();

          // Append all variable of the declaration
          // for(int i=1; i < varlist.size(); i++) {
          // for (j; j < varlist.size(); j++) {
          //     SgInitializedName * var_to_append = SageBuilder::buildInitializedName(
          //             varlist[j]->get_name(), 
          //             arraytype, 
          //             varlist[j]->get_initializer ());
          //   nd->append_variable(var_to_append, var_to_append->get_initializer());
          // }

          // Set and fiw differents position of the node
          SageInterface::setSourcePositionForTransformation(nd);
          SageInterface::setOneSourcePositionForTransformation(nd);
          //Insert the statement create in the new body
          SageInterface::appendStatement(nd, bodycpy);

        }
        //If it's not an array but juste a classic declaration
        else { 
          // SgInitializedNamePtrList varlist =  vardecl->get_variables();
            bool hasToBeRemove = false;

            for(int i=0; i < v_symbol.size(); i++) {
              if (varlist[j]->get_name() == v_symbol[i]->get_name() ) {
                hasToBeRemove = true;
                continue;
              }
            }

            //do not copy the var declaration if the var will be replace by a constante
            if (hasToBeRemove) continue;
            else {
              SgVariableDeclaration * nd = new SgVariableDeclaration(
                                              varlist[j]->get_name(), 
                                              varlist[j]->get_type(), 
                                              varlist[j]->get_initializer ());

            nd->get_declarationModifier().get_typeModifier() = varlist[j]->get_declaration()->get_declarationModifier().get_typeModifier();

              SageInterface::setOneSourcePositionForTransformation(
                                              isSgDeclarationStatement(varlist[j]->get_declptr()));
              SageInterface::setSourcePositionForTransformation(nd);
              SageInterface::appendStatement(nd, bodycpy);

          }
        }
      }
    } else {
      SgStatement * stmt = SageInterface::copyStatement(*it);
      SageInterface::appendStatement(stmt, bodycpy);
    }
  }

  return bodycpy;
}

void insert_declaration_into_function(SgNode * node, SgVariableDeclaration* decl) {
  if(DEBUG) DBG_MAQAO
  SgFunctionDefinition * fd = get_function_RosePtr(node);
  SgBasicBlock *  body = fd->get_body ();

  ROSE_ASSERT(body != NULL);
  if (!body->lookup_variable_symbol(decl->get_variables()[0]->get_name()))
    SageInterface::insertStatementAfterLastDeclaration(decl,body);
}

variable_spe* find(SgVarRefExp* vre, std::vector<variable_spe*> varspe) {
  if(DEBUG) DBG_MAQAO
  for (int i=0; i < varspe.size(); i++) {
    if (std::string(varspe[i]->var_name) == vre->get_symbol()->get_name ().getString()) {
      return varspe[i];
    }
  }
  return NULL;
}

////////////////////////////
// Library call functions //
////////////////////////////
void addvprofCall(SgGlobal * globalScope) {
  SgFunctionDeclaration* funcDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);

  while (funcDecl) {
    if (!funcDecl->get_file_info()->isTransformation() && !SageInterface::isMain (funcDecl)) {
      std::string argsString = "";
      SgInitializedNamePtrList & argsList = funcDecl->get_args ();
      
      for (int i=0; i < argsList.size(); i++) {
        SgType * argType = argsList[i]->get_type ();
      
        if (isSgTypeInt(argType->dereference ()) 
          || isSgTypeLong(argType->dereference ()) 
          || isSgTypeFloat(argType->dereference ()) 
          || isSgTypeDouble(argType->dereference ())
          || isSgArrayType(argType->dereference ())
          || isSgPointerType(argType->dereference ())) {

          if(SgArrayType* arrayType = isSgArrayType(argType)) {
            if (!(isSgTypeInt(arrayType->get_base_type()) 
              || isSgTypeLong(arrayType->get_base_type()) 
              || isSgTypeFloat(arrayType->get_base_type()) 
              || isSgTypeDouble(arrayType->get_base_type()))) 
            {
              continue;
            }
          }
          if(SgPointerType* arrayType = isSgPointerType(argType)) {
            while (isSgPointerType(arrayType->get_base_type())) { arrayType = isSgPointerType(arrayType->get_base_type()); }

            if (!(isSgTypeInt(arrayType->get_base_type()) 
              || isSgTypeLong(arrayType->get_base_type()) 
              || isSgTypeFloat(arrayType->get_base_type()) 
              || isSgTypeDouble(arrayType->get_base_type()))) 
            {
              continue;
            }
          }
          argsString += argsList[i]->get_name().getString() + ",";
        }
      }

      //Add calls to the vprof lib to the function.
      add_lib_call(funcDecl->get_definition(), argsString);
    }
    
    SgStatement* nextStmt = SageInterface::getNextStatement (funcDecl);

    while (isSgPragmaDeclaration(nextStmt) && nextStmt) {
      nextStmt = SageInterface::getNextStatement (nextStmt);
    }

    funcDecl = isSgFunctionDeclaration(nextStmt);    
    if (DEBUG && nextStmt) std::cout << nextStmt->class_name() << std::endl;
  }
}

void add_init_call (SgFunctionDefinition* main, int nbThreads, int model) {
  if(DEBUG) DBG_MAQAO

  SgGlobal * globalScope = SageInterface::getGlobalScope(main);
  SgSymbolTable * gst = globalScope->get_symbol_table();

  /////////////////////////////
  // Name of vprof functions //
  //       in the lib        //
  /////////////////////////////
  std::string func_init32 = "vproffct_init_int32";
  std::string func_init64 = "vproffct_init_int64";
  std::string func_initfloat = "vproffct_init_float";
  std::string func_initdouble = "vproffct_init_double";
  std::string func_initptr = "vproffct_init_ptr";
  std::string func_initifthen = "vproffct_init_ifthen";
  std::string func_initifelse = "vproffct_init_ifelse";
  std::string func_init = "vproffct_init";
  std::string func_dump = "vproffct_dump";
  std::string func_free = "vproffct_free";

  /////////////////////////////
  //  Creaton of the nodes   //
  /////////////////////////////
  SgFunctionSymbol * f_symbolint32  = gst->find_function(SgName(func_init32));
  SgFunctionSymbol * f_symbolint64  = gst->find_function(SgName(func_init64));
  SgFunctionSymbol * f_symbolfloat  = gst->find_function(SgName(func_initfloat));
  SgFunctionSymbol * f_symboldouble = gst->find_function(SgName(func_initdouble));
  SgFunctionSymbol * f_symbolptr    = gst->find_function(SgName(func_initptr));
  SgFunctionSymbol * f_symbolifthen = gst->find_function(SgName(func_initifthen));
  SgFunctionSymbol * f_symbolifelse = gst->find_function(SgName(func_initifelse));
  SgFunctionSymbol * f_symbolinit   = gst->find_function(SgName(func_init));
  SgFunctionSymbol * f_symboldump   = gst->find_function(SgName(func_dump));
  SgFunctionSymbol * f_symbolfree   = gst->find_function(SgName(func_free));

  /////////////////////////////
  //  Build the return type  //
  //  and prepared the list  //
  //         of args         //
  /////////////////////////////
  SgType* voidType = SageBuilder::buildVoidType ();
  SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();
  // SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();

  /////////////////////////////////
  // Include header if not exist //
  /////////////////////////////////
  if ( !gst->find_function(SgName("vproffct_store_int32")) && !gst->find_function(SgName("vproffct_store_int32_") )
    && !gst->find_function(SgName("vproffct_store_int64")) && !gst->find_function(SgName("vproffct_store_int64_"))
    && !gst->find_function(SgName("vproffct_store_float")) && !gst->find_function(SgName("vproffct_store_float_"))
    && !gst->find_function(SgName("vproffct_store_ptr")) && !gst->find_function(SgName("vproffct_store_ptr_"))
    && !gst->find_function(SgName("vproffct_store_ifelse")) && !gst->find_function(SgName("vproffct_store_ifthen_"))
    && !gst->find_function(SgName("vproffct_store_double")) && !gst->find_function(SgName("vproffct_store_double_"))
    && !SageInterface::is_Fortran_language()) 
  {
    SageInterface::insertHeader ("libvprof.fct.h" ,PreprocessingInfo::after, false/*bool isSystemHeader*/, globalScope /*scope*/);
  }

  ////////////////////////////////
  // Build header of lib's      //
  // funtions, if not exist,    //
  // as external because        //
  // they're defined in the lib //
  ////////////////////////////////
  if (! f_symbolint32) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    paramList->append_arg(SageBuilder::buildInitializedName("nbofVar",SageBuilder::buildIntType()));
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_init32), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolint32  = gst->find_function(SgName(func_init32));
    ROSE_ASSERT(f_symbolint32);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symbolint64) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    paramList->append_arg(SageBuilder::buildInitializedName("nbofVar",SageBuilder::buildIntType()));
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_init64), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolint64  = gst->find_function(SgName(func_init64));
    ROSE_ASSERT(f_symbolint64);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symbolfloat) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    paramList->append_arg(SageBuilder::buildInitializedName("nbofVar",SageBuilder::buildIntType()));
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_initfloat), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolfloat  = gst->find_function(SgName(func_initfloat));
    ROSE_ASSERT(f_symbolfloat);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symboldouble) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    paramList->append_arg(SageBuilder::buildInitializedName("nbofVar",SageBuilder::buildIntType()));
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_initdouble), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symboldouble  = gst->find_function(SgName(func_initdouble));
    ROSE_ASSERT(f_symboldouble);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symbolptr) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    paramList->append_arg(SageBuilder::buildInitializedName("nbofVar",SageBuilder::buildIntType()));
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_initptr), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolptr  = gst->find_function(SgName(func_initptr));
    ROSE_ASSERT(f_symbolptr);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symbolifthen) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    paramList->append_arg(SageBuilder::buildInitializedName("nbofVar",SageBuilder::buildIntType()));
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_initifthen), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolifthen  = gst->find_function(SgName(func_initifthen));
    ROSE_ASSERT(f_symbolifthen);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symbolifelse) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();
    paramList->append_arg(SageBuilder::buildInitializedName("nbofVar",SageBuilder::buildIntType()));
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_initifelse), 
                                           voidType, //SgType *return_type, 
                                           paramList,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolifelse  = gst->find_function(SgName(func_initifelse));
    ROSE_ASSERT(f_symbolifelse);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symbolinit) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramListinit = SageBuilder::buildFunctionParameterList();

    paramListinit->append_arg(SageBuilder::buildInitializedName("nbthreads",SageBuilder::buildIntType()));
    paramListinit->append_arg(SageBuilder::buildInitializedName("model",SageBuilder::buildIntType()));

    int stringSize = 13;
    Sg_File_Info* fileInfo = Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode();
    SgIntVal* lengthExpression = new SgIntVal(fileInfo,stringSize,"13");
    paramListinit->append_arg(SageBuilder::buildInitializedName("maqao_bin_tmp",SageBuilder::buildStringType (lengthExpression) ));

    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_init), 
                                           voidType, //SgType *return_type, 
                                           paramListinit,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramListinit->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolinit  = gst->find_function(SgName(func_init));
    ROSE_ASSERT(f_symbolinit);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symboldump) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramListdump = SageBuilder::buildFunctionParameterList();

    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_dump), 
                                           voidType, //SgType *return_type, 
                                           paramListdump,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramListdump->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symboldump  = gst->find_function(SgName(func_dump));
    ROSE_ASSERT(f_symboldump);

    SageInterface::setExtern(func_decl);
  }

  if (! f_symbolfree) {
    SgFunctionDeclaration * func_decl = NULL;
    SgFunctionParameterList* paramListdump = SageBuilder::buildFunctionParameterList();

    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                           SgName(func_free), 
                                           voidType, //SgType *return_type, 
                                           paramListdump,  //SgFunctionParameterList *parlist, 
                                           globalScope,    //SgScopeStatement *scope, 
                                           exprListExp);//,   //SgExprListExp *decoratorList, 

    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramListdump->set_parent(func_decl);
    exprListExp->set_parent(func_decl);

    //insert declaration
    // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
    // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

    //Check if the symbol was well store
    f_symbolfree  = gst->find_function(SgName(func_free));
    ROSE_ASSERT(f_symbolfree);

    SageInterface::setExtern(func_decl);
  }

  //////////////////////////
  //  Build calls to the  //
  //  lib's functions     //
  //////////////////////////
  SgFunctionCallExp * func_callint32  = SageBuilder::buildFunctionCallExp (f_symbolint32); 
  func_callint32->set_parent(main->get_body());

  SgFunctionCallExp * func_callint64  = SageBuilder::buildFunctionCallExp (f_symbolint64); 
  func_callint64->set_parent(main->get_body());

  SgFunctionCallExp * func_callfloat  = SageBuilder::buildFunctionCallExp (f_symbolfloat); 
  func_callfloat->set_parent(main->get_body());

  SgFunctionCallExp * func_calldouble = SageBuilder::buildFunctionCallExp (f_symboldouble); 
  func_calldouble->set_parent(main->get_body());

  SgFunctionCallExp * func_callifthen  = SageBuilder::buildFunctionCallExp (f_symbolifthen); 
  func_callifthen->set_parent(main->get_body());

  SgFunctionCallExp * func_callifelse = SageBuilder::buildFunctionCallExp (f_symbolifelse); 
  func_callifelse->set_parent(main->get_body());

  SgFunctionCallExp * func_callptr = SageBuilder::buildFunctionCallExp (f_symbolptr); 
  func_callptr->set_parent(main->get_body());

  SgFunctionCallExp * func_callinit   = SageBuilder::buildFunctionCallExp (f_symbolinit); 
  func_callinit->set_parent(main->get_body());

  SgFunctionCallExp * func_calldump   = SageBuilder::buildFunctionCallExp (f_symboldump); 
  func_calldump->set_parent(main->get_body());

  SgFunctionCallExp * func_callfree   = SageBuilder::buildFunctionCallExp (f_symbolfree); 
  func_callfree->set_parent(main->get_body());
  
  ///////////////////////////////
  // Browse the config file    //
  // searching all int & float //
  // already counted           //
  ///////////////////////////////
  int nbInt32=0, nbInt64=0, nbFloat=0, nbDouble=0, nbptr=0, nbIfThen=0, nbIfElse=0;

  nbInt32 =substr_counter_in_file("int32", "maqaoCptTmpVarSpe.cpt");
  nbInt64 =substr_counter_in_file("int64", "maqaoCptTmpVarSpe.cpt");
  nbFloat =substr_counter_in_file("float", "maqaoCptTmpVarSpe.cpt");
  nbDouble=substr_counter_in_file("double","maqaoCptTmpVarSpe.cpt");

  nbIfThen=substr_counter_in_file("ifthen","maqaoCptTmpVarSpe.cpt");
  nbIfElse=substr_counter_in_file("ifelse","maqaoCptTmpVarSpe.cpt");

  nbptr =substr_counter_in_file("int32*", "maqaoCptTmpVarSpe.cpt");
  nbptr+=substr_counter_in_file("int64*", "maqaoCptTmpVarSpe.cpt");
  nbptr+=substr_counter_in_file("float*", "maqaoCptTmpVarSpe.cpt");
  nbptr+=substr_counter_in_file("double*","maqaoCptTmpVarSpe.cpt");

  //////////////////////////
  // Initialize functions //
  // with the right value //
  //////////////////////////
  SgIntVal * int32Val = SageBuilder::buildIntVal (nbInt32);
  func_callint32->append_arg(int32Val);
  int32Val->set_parent(func_callint32);

  SgIntVal * int64Val = SageBuilder::buildIntVal (nbInt64);
  func_callint64->append_arg(int64Val);
  int64Val->set_parent(func_callint64);

  SgIntVal * floatVal = SageBuilder::buildIntVal (nbFloat);
  func_callfloat->append_arg(floatVal);
  floatVal->set_parent(func_callfloat);

  SgIntVal * doubleVal = SageBuilder::buildIntVal (nbDouble);
  func_calldouble->append_arg(doubleVal);
  doubleVal->set_parent(func_calldouble);

  SgIntVal * ptrVal = SageBuilder::buildIntVal (nbptr);
  func_callptr->append_arg(ptrVal);
  ptrVal->set_parent(func_callptr);

  SgIntVal * ifthenVal = SageBuilder::buildIntVal (nbIfThen);
  func_callifthen->append_arg(ifthenVal);
  ifthenVal->set_parent(func_callifthen);

  SgIntVal * ifelseVal = SageBuilder::buildIntVal (nbptr);
  func_callifelse->append_arg(ifelseVal);
  ifelseVal->set_parent(func_callifelse);

  // Init
  //Number of threads
  SgIntVal * nbThds = SageBuilder::buildIntVal (nbThreads);
  func_callinit->append_arg(nbThds);
  nbThds->set_parent(func_callinit);

  //model
  SgIntVal * modelVal = SageBuilder::buildIntVal (model);
  func_callinit->append_arg(modelVal);
  modelVal->set_parent(func_callinit);

  //string
  SgStringVal * strVal = SageBuilder::buildStringVal ("maqao_bin_tmp");
  func_callinit->append_arg(strVal);
  strVal->set_parent(func_callinit);
  
  //////////////////////////////////
  // Add call of initialization   //
  // after all first declarations //
  //////////////////////////////////
  if (SgStatement* stmtToInsert = SageInterface::findLastDeclarationStatement(isSgScopeStatement(main->get_body()))) {
    //Find last declaration after first salve.
    SgStatement* rt = NULL;
    SgStatementPtrList stmt_list = main->get_body()->generateStatementList ();

    for (size_t i = 0; i<stmt_list.size(); i++) {
      SgStatement* cur_stmt = stmt_list[i];
      if (isSgDeclarationStatement(cur_stmt)) {
        rt = cur_stmt;
      } else {
        break;
      }
    }
    stmtToInsert = rt;

    // Init
    if(nbDouble || nbFloat || nbInt64 || nbInt32 || nbptr) {
      SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callinit), true);
    }

    // Insert calls stmts after all declarations
    if(nbptr) SageInterface::insertStatementAfter   ( stmtToInsert, SageBuilder::buildExprStatement(func_callptr), true);
    if(nbDouble) SageInterface::insertStatementAfter( stmtToInsert, SageBuilder::buildExprStatement(func_calldouble), true);
    if(nbFloat) SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callfloat), true);
    if(nbInt64) SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callint64), true);
    if(nbInt32) SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callint32), true);
    if(nbIfThen) SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callifthen), true);
    if(nbIfElse) SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callifelse), true);
  } else {
    if(DEBUG) DBG_MAQAO
      if(nbDouble || nbFloat || nbInt64 || nbInt32 || nbptr) 
        main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callinit));
      
      if(nbptr)    main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callptr));
      if(nbDouble) main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_calldouble));
      if(nbFloat)  main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callfloat));
      if(nbInt64)  main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callint64));
      if(nbInt32)  main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callint32));
      if(nbIfThen) main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callifthen));
      if(nbIfElse) main->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callifelse));      
  }
  // Do NOT forget the dump at the end of the main
  SgStatement* lastStmtOfMain = SageInterface::getLastStatement (main->get_body());

  if (isSgReturnStmt(lastStmtOfMain) || isSgStopOrPauseStatement(lastStmtOfMain)) {
    SageInterface::insertStatementBefore (
    lastStmtOfMain,
    SageBuilder::buildExprStatement(func_calldump), 
    true);

    SageInterface::insertStatementBefore (
    lastStmtOfMain,
    SageBuilder::buildExprStatement(func_callfree), 
    true);
  } else {
    main->get_body()->append_statement (SageBuilder::buildExprStatement(func_calldump));
    main->get_body()->append_statement (SageBuilder::buildExprStatement(func_callfree));
  }
}

// varNames contains all var names separate by ','
void add_lib_call (SgStatement* stmt, std::string varNames, std::string uniqueName/*="-1"*/) {
  if(DEBUG) DBG_MAQAO
  // Get all var names
  std::string buff ="";
  std::vector<std::string> vars;
  const char separator = ',';
  SgFunctionDefinition* function =isSgFunctionDefinition(stmt);

  if (!function) {
    function = get_function_RosePtr(stmt);
    if (!function) {
      DBG_MAQAO
      std::cout << " ERROR no function found for the statement : " << stmt->unparseToString() << std::endl;
    }
  }
  ///////////////////////////////////////
  // Build the list of variables       //
  // by extracting variables names     //
  // from a string where all variables //
  // are wrote and separate by a coma  //
  ///////////////////////////////////////
  for(int i=0; i < varNames.size(); i++) {
    if(varNames[i] != separator && varNames[i] != ' ') {
      buff+=varNames[i]; 
    }
    else {
      if(varNames[i] == separator && buff != "") { 
        vars.push_back(buff); 
        buff = ""; 
      }
    }
  }
  if(buff != "") { vars.push_back(buff); }
  // End get all var names

  //Browse all var names and for each one, search in symbol table their types and add the right call
  SgSymbolTable * st = function->get_symbol_table();

  //looking for variable to specialize into symbol tables
  for(int i=0; i < vars.size(); i++) { 
    SgSymbol* vsym = st->find_variable(vars[i]);
    if (!vsym) {
      SgBasicBlock * bb = isSgBasicBlock(function->get_body());
      SgSymbolTable * stbb = bb->get_symbol_table();
      vsym = stbb->find_variable(vars[i]);
        
      if (!vsym) {
        if(LOG) {
          std::cout << "The function "<< function->get_declaration()->get_name()
                    << " will not be handle, because " << vars[i]
                    << " is not a parameter of the function or declare in the body of the function"
                    << std::endl;
        }
        return;
      }
    }
    
    SgGlobal * globalScope = SageInterface::getGlobalScope(function);
    SgSymbolTable * gst = globalScope->get_symbol_table();

    //All symbol of function of the lib
    std::string func_init32 = "vproffct_store_int32";
    std::string func_init64 = "vproffct_store_int64";
    std::string func_initfloat = "vproffct_store_float";
    std::string func_initdouble = "vproffct_store_double";
    std::string func_initptr = "vproffct_store_ptr";

    SgFunctionSymbol * f_symbolint32  = gst->find_function(SgName(func_init32));
    SgFunctionSymbol * f_symbolint64  = gst->find_function(SgName(func_init64));
    SgFunctionSymbol * f_symbolfloat  = gst->find_function(SgName(func_initfloat));
    SgFunctionSymbol * f_symboldouble  = gst->find_function(SgName(func_initdouble));
    SgFunctionSymbol * f_symbolptr  = gst->find_function(SgName(func_initptr));

    //Have we to add the include ?!
    if (!isHeaderAlreadyExist(stmt, std::string("#include \"libvprof.fct.h\" ")) && !SageInterface::is_Fortran_language())
    {
      SageInterface::insertHeader ("libvprof.fct.h" ,PreprocessingInfo::after, false/*bool isSystemHeader*/, globalScope /*scope*/);
    }
  
    bool isInt = false, isLong = false, isFloat = false, isDouble = false;
    bool isArrInt = false, isArrLong = false, isArrFloat = false, isArrDouble = false;

    //Try to determine the type of the variable to analyze, must be like this for fortran
    if (SgArrayType* atype = isSgArrayType(vsym->get_type ())) {
      SgType* base_type = atype->get_base_type();

      if (SgTypeInt* type = isSgTypeInt(base_type)) {
        if (SageInterface::is_Fortran_language()) {
          if (type->get_type_kind ()) {
            if (type->get_type_kind ()->unparseToString() == "8") { // if is declare as INTEGER * 8
              isArrLong = true;
            } else { // if it's an INTEGER * 4
              isArrInt = true;
            }
          } else { // if it's an INTEGER
            isArrInt = true;
          }
        } else { // if C/C++ int is int and not a long
          isArrInt = true;
        }
      } else if (isSgTypeLong(base_type)) {
          isArrLong = true;
      } else if(isSgTypeFloat(base_type)) {
        if (SageInterface::is_Fortran_language() 
          && isSgTypeFloat(base_type)->get_type_kind ()->unparseToString() == "8") {
          isArrDouble= true;
        } else {
          isArrFloat = true;
        }
      } else if(isSgTypeDouble(base_type)) {
        isArrDouble = true;
      }
    } else if (SgPointerType* atype = isSgPointerType(vsym->get_type ())) {
      SgType* base_type = atype->get_base_type();
      while (isSgPointerType(base_type)) { base_type = isSgPointerType(base_type)->get_base_type(); }
      if (SgTypeInt* type = isSgTypeInt(base_type)) {
        if (SageInterface::is_Fortran_language()) {
          if (type->get_type_kind ()) {
            if (type->get_type_kind ()->unparseToString() == "8") { // if is declare as INTEGER * 8
              isArrLong = true;
            } else { // if it's declare as INTEGER * 4
              isArrInt = true;
            }
          } else { // if it's declare just as INTEGER
            isArrInt = true;
          }
        } else { // if C/C++ int is int and not a long
          isArrInt = true;
        }
      } else if (isSgTypeLong(base_type)) {
          isArrLong = true;
      } else if(isSgTypeFloat(base_type)) {
        if (SageInterface::is_Fortran_language() && isSgTypeFloat(base_type)->get_type_kind ()->unparseToString() == "8") {
          isArrDouble= true;
        } else {
          isArrFloat = true;
        }
      } else if(isSgTypeDouble(base_type)) {
        isArrDouble = true;
      }
    } else if (SgTypeInt* type = isSgTypeInt(vsym->get_type ()->dereference ())) {
      if (SageInterface::is_Fortran_language()) {
        if (type->get_type_kind ()) {
          if (type->get_type_kind ()->unparseToString() == "8") { // if is declare as INTEGER * 8
            isLong = true;
          } else { // if it's declare as INTEGER * 4
            isInt = true;
          }
        } else { // if it's declare just as INTEGER
          isInt = true;
        }
      } else { // if C/C++ int is int and not a long
        isInt = true;
      }
    } else if (isSgTypeLong(vsym->get_type ()->dereference ())) {
        isLong = true;
    } else if(isSgTypeFloat(vsym->get_type ()->dereference ())) {
      if (SageInterface::is_Fortran_language() && isSgTypeFloat(vsym->get_type ())->get_type_kind ()->unparseToString() == "8") {
        isDouble= true;
      } else {
        isFloat = true;
      }
    } else if(isSgTypeDouble(vsym->get_type ()->dereference ())) {
      isDouble = true;
    } 

    if (!(isInt || isLong || isArrLong || isArrInt || isArrFloat || isArrDouble ||isFloat||isDouble)) { continue; }

    std::ofstream myfile;
    myfile.open ("maqaoCptTmpVarSpe.cpt", std::ios::app);
    int ref =-1;
    // One time we get the type we add the call to the right function
    if (isInt) {
      // If the symbol doesn't already exist, creat it
      if (! f_symbolint32) {
        //BUILD SYMBOL
        //BUILD DECLARATION as externat the top of all 
        SgType* voidType = SageBuilder::buildVoidType ();
        SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();

        paramList->append_arg(SageBuilder::buildInitializedName("val",SageBuilder::buildIntType()));
        paramList->append_arg(SageBuilder::buildInitializedName("varName",SageBuilder::buildIntType()));

        SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();
        SgFunctionDeclaration * func_decl = NULL;
        func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                               SgName(func_init32), 
                                               voidType, //SgType *return_type, 
                                               paramList,  //SgFunctionParameterList *parlist, 
                                               globalScope,    //SgScopeStatement *scope, 
                                               exprListExp);//,   //SgExprListExp *decoratorList, 

        func_decl->set_parent(globalScope);
        voidType->set_parent(func_decl);
        paramList->set_parent(func_decl);
        exprListExp->set_parent(func_decl);

        //insert declaration
        // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
        // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, true);

        //Check if the symbol was well store
        f_symbolint32  = gst->find_function(SgName(func_init32));
        ROSE_ASSERT(f_symbolint32);

        SageInterface::setExtern(func_decl);
      }

      // Now we're sure the symbol exist, we will create the call in the function.
      SgFunctionCallExp * func_callint32  = SageBuilder::buildFunctionCallExp (f_symbolint32); 
      func_callint32->set_parent(function->get_body());

      // Push the right value for the right call
      ref = substr_counter_in_file("int32", "maqaoCptTmpVarSpe.cpt");

      SgVarRefExp * int32Var = SageBuilder::buildVarRefExp (SgName(vars[i]), function->get_body());
      SgIntVal * refVal = SageBuilder::buildIntVal(ref);

      func_callint32->append_arg(refVal);
      if (SageInterface::is_C_language () || SageInterface::is_Cxx_language() || SageInterface::is_C99_language ()) {
        func_callint32->append_arg(SageBuilder::buildAddressOfOp(int32Var));
      }
      else {
        func_callint32->append_arg(int32Var);
      }

      refVal->set_parent(func_callint32);
      int32Var->set_parent(func_callint32);

      if (isSgFunctionDefinition(stmt)) {
        if (SgStatement* stmtToInsert = SageInterface::findLastDeclarationStatement(
                                              isSgScopeStatement(function->get_body()))) {
          // Insert calls stmts after all declarations
          SageInterface::insertStatementAfter (stmtToInsert, SageBuilder::buildExprStatement(func_callint32),  false);
        } else {
          function->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callint32));
        }
      } else {
          SageInterface::insertStatementBefore (stmt, SageBuilder::buildExprStatement(func_callint32),  false);
      }

      // Write in the file
      myfile << "{ kind=\"int32\""<< ",";
      myfile << " " << "dim=0"<< ","; //nb dim
      
    } else if(isLong || isArrLong || isArrInt || isArrFloat || isArrDouble) {
      // If the symbol doesn't already exist, creat it
      if (! f_symbolint64) {
        //BUILD SYMBOL
        //BUILD DECLARATION as externat the top of all 
        SgType* voidType = SageBuilder::buildVoidType ();
        SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();

        paramList->append_arg(SageBuilder::buildInitializedName("val",SageBuilder::buildLongType()));
        paramList->append_arg(SageBuilder::buildInitializedName("varName",SageBuilder::buildLongType()));

        SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();
        SgFunctionDeclaration * func_decl = NULL;
        func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                               SgName(func_init64), 
                                               voidType, //SgType *return_type, 
                                               paramList,  //SgFunctionParameterList *parlist, 
                                               globalScope,    //SgScopeStatement *scope, 
                                               exprListExp);//,   //SgExprListExp *decoratorList, 

        func_decl->set_parent(globalScope);
        voidType->set_parent(func_decl);
        paramList->set_parent(func_decl);
        exprListExp->set_parent(func_decl);

        //insert declaration
        // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
        // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, false);

        //Check if the symbol was well store
        f_symbolint64  = gst->find_function(SgName(func_init64));
        ROSE_ASSERT(f_symbolint64);

        SageInterface::setExtern(func_decl);
      }

      // Now we're sure the symbol exist, we will create the call in the function.
      SgFunctionCallExp * func_callint64  = SageBuilder::buildFunctionCallExp (f_symbolint64); 
      func_callint64->set_parent(function->get_body());

      // Push the right value for the right call
      ref = substr_counter_in_file("int64", "maqaoCptTmpVarSpe.cpt");
      // if(DEBUG) std::cout << "refLong = " << ref << std::endl;
      SgVarRefExp * int64Var = SageBuilder::buildVarRefExp (SgName(vars[i]), function->get_body());
      SgIntVal * refVal = SageBuilder::buildIntVal(ref);

      func_callint64->append_arg(refVal);
      if (SageInterface::is_C_language () || SageInterface::is_Cxx_language() || SageInterface::is_C99_language ()) {
        func_callint64->append_arg(SageBuilder::buildAddressOfOp(int64Var));
      }
      else {
        func_callint64->append_arg(int64Var);
      }

      refVal->set_parent(func_callint64);
      int64Var->set_parent(func_callint64);

      if (isSgFunctionDefinition(stmt)) {
        if (SgStatement* stmtToInsert = SageInterface::findLastDeclarationStatement(isSgScopeStatement(function->get_body()))) {
          // Insert calls stmts after all declarations
          SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callint64),  false);
        } else {
          function->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callint64));
        }
      } else {
          SageInterface::insertStatementBefore (stmt, SageBuilder::buildExprStatement(func_callint64),  false);
      }
      // Write in the file
        myfile << "{ kind=\"int64\""<< ",";

      if (SgArrayType* atype = isSgArrayType(vsym->get_type ())) {
          myfile << " dim=" << atype->get_dim_info ()->get_expressions ().size()<< ",";
      } else if (SgPointerType* atype = isSgPointerType(vsym->get_type ())) {
        int nbDim =0;
        do {
          nbDim++;
        } while (atype = isSgPointerType(atype->get_base_type()));
          myfile << " dim=" << nbDim<< ",";
      } else {
        myfile << " " << "dim=0"<< ",";
      }
    
    } else if(isFloat) {
      // If the symbol doesn't already exist, creat it
      if (! f_symbolfloat) {
        //BUILD SYMBOL
        //BUILD DECLARATION as externat the top of all 
        SgType* voidType = SageBuilder::buildVoidType ();
        SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();

        paramList->append_arg(SageBuilder::buildInitializedName("val",SageBuilder::buildFloatType()));
        paramList->append_arg(SageBuilder::buildInitializedName("varName",SageBuilder::buildFloatType()));

        SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();
        SgFunctionDeclaration * func_decl = NULL;
        func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                               SgName(func_initfloat), 
                                               voidType, //SgType *return_type, 
                                               paramList,  //SgFunctionParameterList *parlist, 
                                               globalScope,    //SgScopeStatement *scope, 
                                               exprListExp);//,   //SgExprListExp *decoratorList, 

        func_decl->set_parent(globalScope);
        voidType->set_parent(func_decl);
        paramList->set_parent(func_decl);
        exprListExp->set_parent(func_decl);

        //insert declaration
        // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
        // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, false);

        //Check if the symbol was well store
        f_symbolfloat  = gst->find_function(SgName(func_initfloat));
        ROSE_ASSERT(f_symbolfloat);

        SageInterface::setExtern(func_decl);
      }

      // Now we're sure the symbol exist, we will create the call in the function.
      SgFunctionCallExp * func_callfloat  = SageBuilder::buildFunctionCallExp (f_symbolfloat); 
      func_callfloat->set_parent(function->get_body());

      // Push the right value for the right call
      ref = substr_counter_in_file("float", "maqaoCptTmpVarSpe.cpt");
      // if(DEBUG) std::cout << "ref-float = " << ref << std::endl;

      SgVarRefExp * floatVar = SageBuilder::buildVarRefExp (SgName(vars[i]), function->get_body());
      SgIntVal * refVal = SageBuilder::buildIntVal(ref);

      func_callfloat->append_arg(refVal);
      if (SageInterface::is_C_language () || SageInterface::is_Cxx_language() || SageInterface::is_C99_language ()) {
        func_callfloat->append_arg(SageBuilder::buildAddressOfOp(floatVar));
      }
      else {
        func_callfloat->append_arg(floatVar);
      }

      refVal->set_parent(func_callfloat);
      floatVar->set_parent(func_callfloat);


      if (isSgFunctionDefinition(stmt)) {
        if (SgStatement* stmtToInsert = SageInterface::findLastDeclarationStatement(isSgScopeStatement(function->get_body()))) {
          // Insert calls stmts after all declarations
          SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_callfloat),  false);
        } else {
          function->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_callfloat));
        }
      } else {
          SageInterface::insertStatementBefore (stmt, SageBuilder::buildExprStatement(func_callfloat),  false);
      } 
      //Write in the file
      myfile << "{ kind=\"float\""<< ",";
      myfile << " " << "dim=0"<< ","; //nb dim  
             
    } else if(isDouble) {
      // If the symbol doesn't already exist, creat it
      if (! f_symboldouble) {
        //BUILD SYMBOL
        //BUILD DECLARATION as externat the top of all 
        SgType* voidType = SageBuilder::buildVoidType ();
        SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();

        paramList->append_arg(SageBuilder::buildInitializedName("val",SageBuilder::buildDoubleType()));
        paramList->append_arg(SageBuilder::buildInitializedName("varName",SageBuilder::buildDoubleType()));

        SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();
        SgFunctionDeclaration * func_decl = NULL;
        func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                                               SgName(func_initdouble), 
                                               voidType, //SgType *return_type, 
                                               paramList,  //SgFunctionParameterList *parlist, 
                                               globalScope,    //SgScopeStatement *scope, 
                                               exprListExp);//,   //SgExprListExp *decoratorList, 

        func_decl->set_parent(globalScope);
        voidType->set_parent(func_decl);
        paramList->set_parent(func_decl);
        exprListExp->set_parent(func_decl);

        //insert declaration
        // SgFunctionDeclaration * firstFuncDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
        // SageInterface::insertStatementBefore (firstFuncDecl, func_decl, false);

        //Check if the symbol was well store
        f_symboldouble  = gst->find_function(SgName(func_initdouble));
        ROSE_ASSERT(f_symboldouble);

        SageInterface::setExtern(func_decl);
      }

      // Now we're sure the symbol exist, we will create the call in the function.
      SgFunctionCallExp * func_calldouble  = SageBuilder::buildFunctionCallExp (f_symboldouble); 
      func_calldouble->set_parent(function->get_body());

      // Push the right value for the right call
      ref = substr_counter_in_file("double", "maqaoCptTmpVarSpe.cpt");
      SgVarRefExp * doubleVar = SageBuilder::buildVarRefExp (SgName(vars[i]), function->get_body());
      SgIntVal * refVal = SageBuilder::buildIntVal(ref);

      func_calldouble->append_arg(refVal);
      if (SageInterface::is_C_language () || SageInterface::is_Cxx_language() || SageInterface::is_C99_language ()) {
        func_calldouble->append_arg(SageBuilder::buildAddressOfOp(doubleVar));
      }
      else {
        func_calldouble->append_arg(doubleVar);
      }

      refVal->set_parent(func_calldouble);
      doubleVar->set_parent(func_calldouble);

      if (isSgFunctionDefinition(stmt)) {
        if (SgStatement* stmtToInsert = SageInterface::findLastDeclarationStatement(isSgScopeStatement(function->get_body()))) {
          // Insert calls stmts after all declarations
          SageInterface::insertStatementAfter ( stmtToInsert, SageBuilder::buildExprStatement(func_calldouble),  false);
        } else {
          function->get_body()->prepend_statement (SageBuilder::buildExprStatement(func_calldouble));
        }
      } else {
          SageInterface::insertStatementBefore (stmt, SageBuilder::buildExprStatement(func_calldouble),  false);
      }
      // Write in the file
      myfile << "{ kind=\"double\""<< ","; //type
      myfile << " " << "dim=0"<< ","; //nb dim

    }
    
    myfile << " id=" << ref << ","; // ID
    myfile << " var_name=\"" << vars[i]<< "\"," ; // var name
    if (uniqueName == "-1") {
      myfile << " where=\"" << function->get_declaration()->get_name().getString()<< "\","; // function name
      // myfile << " " << function->get_file_info()->get_line(); //localisation line of the function
    } else {
      myfile << " where=\"" << uniqueName<< "\","; // represent the localisation where we need the var
    }
    myfile << " file=\"" << getEnclosingsourceFile_RosePtr(function)->get_sourceFileNameWithPath() << "\" " ; // path of the file
    myfile <<"},\n"; //ending
    myfile.close();  
  }
}

// varNames contains all var names separate by ','
void add_lib_call_if (SgIfStmt* ifstmt, std::string uniqueName) {
  if(DEBUG) DBG_MAQAO

  SgFunctionDefinition* function = get_function_RosePtr(ifstmt);
  SgGlobal * globalScope = SageInterface::getGlobalScope(ifstmt);
  SgSymbolTable * gst = globalScope->get_symbol_table();

  std::string func_ifthen = "vproffct_store_ifthen";
  std::string func_ifelse = "vproffct_store_ifelse";

  SgFunctionSymbol * f_symbolifthen  = gst->find_function(SgName(func_ifthen));
  SgFunctionSymbol * f_symbolifelse  = gst->find_function(SgName(func_ifelse));

  //Have we to add the include ?!
  if (!isHeaderAlreadyExist(ifstmt, std::string("#include \"libvprof.fct.h\" ")) && !SageInterface::is_Fortran_language())
  {
    SageInterface::insertHeader ("libvprof.fct.h" ,PreprocessingInfo::after, false/*bool isSystemHeader*/, globalScope /*scope*/);
  }

  std::ofstream myfile;
  myfile.open ("maqaoCptTmpVarSpe.cpt", std::ios::app);
  int ref =-1;

  if (! f_symbolifthen) {
    //BUILD SYMBOL
    //BUILD DECLARATION as externat the top of all 
    SgType* voidType = SageBuilder::buildVoidType ();
    SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();

    paramList->append_arg(SageBuilder::buildInitializedName("val",SageBuilder::buildIntType()));
    paramList->append_arg(SageBuilder::buildInitializedName("varName",SageBuilder::buildIntType()));

    SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();
    SgFunctionDeclaration * func_decl = NULL;
    func_decl = SageBuilder::buildNondefiningFunctionDeclaration (
                        SgName(func_ifthen), 
                        voidType, //SgType *return_type, 
                        paramList,  //SgFunctionParameterList *parlist, 
                        globalScope,    //SgScopeStatement *scope, 
                        exprListExp);//,   //SgExprListExp *decoratorList, 
    func_decl->set_parent(globalScope);
    voidType->set_parent(func_decl);
    paramList->set_parent(func_decl);
    exprListExp->set_parent(func_decl);
      
    //Check if the symbol was well store
    f_symbolifthen  = gst->find_function(SgName(func_ifthen));
    ROSE_ASSERT(f_symbolifthen);

    SageInterface::setExtern(func_decl);
  }

  // Now we're sure the symbol exist, we will create the call in the function.
  SgFunctionCallExp * func_callifthen  = SageBuilder::buildFunctionCallExp (f_symbolifthen); 
  func_callifthen->set_parent(function->get_body());

  // Push the right value for the right call
  ref = substr_counter_in_file("ifthen", "maqaoCptTmpVarSpe.cpt");

  SgIntVal * refVal = SageBuilder::buildIntVal(ref);

  func_callifthen->append_arg(refVal);
  refVal->set_parent(func_callifthen);
  if (SgBasicBlock* thenBody = isSgBasicBlock(ifstmt->get_true_body())) {
    thenBody->prepend_statement(SageBuilder::buildExprStatement(func_callifthen));
  } else {
    SgBasicBlock* newThenBody = SageBuilder::buildBasicBlock (SageBuilder::buildExprStatement(func_callifthen), ifstmt->get_true_body());
    ifstmt->set_true_body(newThenBody);
    newThenBody->set_parent(ifstmt);
  }


  // Write in the file
  myfile << "{ kind=\"ifthen\"" << ",";
  myfile << " id=" << ref << ","; // ID
  myfile << " where=\"" << uniqueName << "\","; //uniqueName
  myfile << " file=\"" << getEnclosingsourceFile_RosePtr(ifstmt)->get_sourceFileNameWithPath() << "\" " ; // path of the file
  myfile << " },\n"; //ending

  // Do the same things to the false part
  if (ifstmt->get_false_body()) {
    if (! f_symbolifelse) {
      //BUILD SYMBOL
      //BUILD DECLARATION as externat the top of all 
      SgType* voidType = SageBuilder::buildVoidType ();
      SgFunctionParameterList* paramList = SageBuilder::buildFunctionParameterList();

      paramList->append_arg(SageBuilder::buildInitializedName("val",SageBuilder::buildIntType()));
      paramList->append_arg(SageBuilder::buildInitializedName("varName",SageBuilder::buildIntType()));

      SgExprListExp* exprListExp = SageBuilder::buildExprListExp ();
      SgFunctionDeclaration * func_decl = NULL;
      SageBuilder::buildNondefiningFunctionDeclaration (
                          SgName(func_ifelse), 
                          voidType, //SgType *return_type, 
                          paramList,  //SgFunctionParameterList *parlist, 
                          globalScope,    //SgScopeStatement *scope, 
                          exprListExp);//,   //SgExprListExp *decoratorList, 

      func_decl->set_parent(globalScope);
      voidType->set_parent(func_decl);
      paramList->set_parent(func_decl);
      exprListExp->set_parent(func_decl);
        
      //Check if the symbol was well store
      f_symbolifelse  = gst->find_function(SgName(func_ifelse));
      ROSE_ASSERT(f_symbolifelse);

      SageInterface::setExtern(func_decl);
    }

    // Now we're sure the symbol exist, we will create the call in the function.
    SgFunctionCallExp * func_callifelse  = SageBuilder::buildFunctionCallExp (f_symbolifelse); 
    func_callifelse->set_parent(function->get_body());

    // Push the right value for the right call
    ref = substr_counter_in_file("ifelse", "maqaoCptTmpVarSpe.cpt");

    SgIntVal * refVal = SageBuilder::buildIntVal(ref);
    func_callifelse->append_arg(refVal);
    refVal->set_parent(func_callifelse);

    if (SgBasicBlock* elseBody = isSgBasicBlock(ifstmt->get_false_body())) {
      elseBody->prepend_statement(SageBuilder::buildExprStatement(func_callifelse));
    } else {
      SgBasicBlock* newElseBody = SageBuilder::buildBasicBlock (SageBuilder::buildExprStatement(func_callifelse), ifstmt->get_false_body());
      
      ifstmt->set_true_body(newElseBody);
      newElseBody->set_parent(ifstmt);
    }

    // Write in the file
    myfile << "{ kind=\"ifelse\"" << ",";
    myfile << " id=" << ref << ","; // ID
    myfile << " where=\"" << uniqueName << "\","; //uniqueName   
    myfile << " file=\"" << getEnclosingsourceFile_RosePtr(ifstmt)->get_sourceFileNameWithPath() << "\" " ; // path of the file
    myfile << " },\n"; //ending
  }
  myfile.close(); 
}

int remove_lib_call(SgStatement* stmt) {
  //All functions name of the vprof lib
  std::string func_store32     = "vproffct_store_int32";
  std::string func_store64     = "vproffct_store_int64";
  std::string func_storefloat  = "vproffct_store_float";
  std::string func_storedouble = "vproffct_store_double";
  std::string func_storeifthen = "vproffct_store_ifthen";
  std::string func_storeifelse = "vproffct_store_ifelse";
  std::string func_storeptr    = "vproffct_store_ptr";

  std::string func_init32      = "vproffct_init_int32";
  std::string func_init64      = "vproffct_init_int64";
  std::string func_initfloat   = "vproffct_init_float";
  std::string func_initdouble  = "vproffct_init_double";
  std::string func_initifthen  = "vproffct_init_ifthen";
  std::string func_initifelse  = "vproffct_init_ifelse";
  std::string func_initptr     = "vproffct_init_ptr";
  std::string func_init        = "vproffct_init";

  std::string func_dump        = "vproffct_dump";
  std::string func_free        = "vproffct_free";
  
  int total_removed = 0;

  // Remove header
  if (SgGlobal* globalScope = SageInterface::getGlobalScope(stmt)) {
    bool find = false;
    SgDeclarationStatementPtrList & glbList = globalScope->get_declarations ();
    for (int i=0; i < glbList.size(); i++) {
      //must have this judgement, otherwise wrong file will be modified!
      //It could also be the transformation generated statements with #include attached
      if ((glbList[i]->get_file_info ())->isSameFile(globalScope->get_file_info ())
       || (glbList[i]->get_file_info ())->isTransformation()) 
      {
        //print preproc info
        AttachedPreprocessingInfoType *comments = glbList[i]->getAttachedPreprocessingInfo ();
        AttachedPreprocessingInfoType::iterator i;
        for (i = comments->begin (); i != comments->end (); i++)
        {
          if ((*i)->getString ().substr(0, (*i)->getString ().size()-1) == std::string("#include \"libvprof.fct.h\" ")) {
            // std::cout << "directive to remove : " << (*i)->getString ().c_str () << std::endl;
            i = comments->erase(i);
            break;
          }
        }

        //Stop because we just want the first declaration to had include stmt
        break;
      }
    }
  }

  //
  if (SgExprStatement* exprStmt = isSgExprStatement(stmt)) {
    if (SgCallExpression* call = isSgCallExpression(exprStmt->get_expression())) {
      std::string func_name = isSgFunctionRefExp(call->get_function())->get_symbol_i()->get_name().getString();
      if (func_name == func_init32     || func_name == func_init64
        || func_name == func_initfloat || func_name == func_initdouble
        || func_name == func_initifthen|| func_name == func_initifelse
        || func_name == func_initptr   || func_name == func_init
        || func_name == func_dump      || func_name == func_free
        || func_name == func_store32   || func_name == func_store64
        || func_name == func_storefloat|| func_name == func_storedouble
        || func_name == func_storeifthen|| func_name == func_storeifelse
        || func_name == func_storeptr) {
          SageInterface::removeStatement(stmt);
        total_removed++;
        }
      }
  } else if (SgFunctionDefinition* func = isSgFunctionDefinition(stmt)){
    total_removed += remove_lib_call(func->get_body());
  } else if (SgBasicBlock* body = isSgBasicBlock(stmt)) {
    SgStatementPtrList& stmtList = body->get_statements();
    for (int i=0; i < stmtList.size(); ++i) {
      int nb_removed = remove_lib_call(stmtList[i]);
      i -= nb_removed;
      total_removed += nb_removed;
    }
  } else if (SgForStatement* loop = isSgForStatement(stmt)) {
    total_removed += remove_lib_call(loop->get_loop_body());
  } else if (SgFortranDo* loop = isSgFortranDo(stmt)) {
    total_removed += remove_lib_call(loop->get_body());
  } else if (SgIfStmt* ifstmt = isSgIfStmt(stmt)) {
    total_removed += remove_lib_call(ifstmt->get_true_body());
    if (ifstmt->get_false_body()) {
      total_removed += remove_lib_call(ifstmt->get_false_body());
    }
  } else if (SgDoWhileStmt* loop = isSgDoWhileStmt(stmt)) {
    total_removed += remove_lib_call(loop->get_body());
  } else if (SgWhileStmt* loop = isSgWhileStmt(stmt)) {
    total_removed += remove_lib_call(loop->get_body());    
  } else if (SgSwitchStatement * switchstmt = isSgSwitchStatement (stmt)) {
    total_removed += remove_lib_call(switchstmt->get_body());    
  } else if (SgCaseOptionStmt * optionstmt = isSgCaseOptionStmt (stmt)) {
    total_removed += remove_lib_call(optionstmt->get_body()); 
  } else if (SgDefaultOptionStmt * defaultstmt = isSgDefaultOptionStmt (stmt)) {
    total_removed += remove_lib_call(defaultstmt->get_body()); 
  } else if (SgBasicBlock* bb = isSgBasicBlock(stmt)) {
    SgStatementPtrList & stmtList =  bb->get_statements ();
    for (int i=0; i < stmtList.size(); i++) {
      total_removed += remove_lib_call(stmtList[i]); 
    }
  } else if (SgScopeStatement* scope = isSgScopeStatement(stmt)){
    DBG_MAQAO
    std::cout << "Looking for vprof lib call to remove and we found " << scope->class_name() << " which is not handle" << std::endl;
  }
  return total_removed;
}

void use_lib_results(SgScopeStatement* scope, int nb_variable_spe, variable_spe* var_spe) {
  if (DEBUG) DBG_MAQAO
  SgGlobal * root = isSgGlobal(scope);
  if (!root) root = SageInterface::getGlobalScope(scope);

  std::vector<SgStatement*> stmtMarkedList = s2s_api::lookingForMarkedStmt(root);

  if (DEBUG){
    for (int itvarspe=0; itvarspe < nb_variable_spe; itvarspe++) {
      std::cout << "--------------------------------------\n";
      std::cout << "VarSpe : " << std::endl;
      std::cout << "var name : " << std::string(var_spe[itvarspe].var_name) << std::endl;
      std::cout << "Value : "    << var_spe[itvarspe].value    << std::endl;
      std::cout << "label : "    << std::string(var_spe[itvarspe].label)    << std::endl;
      std::cout << "--------------------------------------\n";
    }
  }

  std::vector<variable_spe*> varList;
  for (int i=0; i < nb_variable_spe; i++) {
    varList.push_back(&var_spe[i]);
  }

  for (int i=0; i < stmtMarkedList.size(); i++) {
    //For each marked statement adapt the solution
    remove_MAQAO_directives(stmtMarkedList[i]);
    if (SgForStatement* forloop = isSgForStatement(stmtMarkedList[i])) {
      Loop* loop = new Loop(forloop, -1);
      loop->specialize(varList);
    }
  }  
}

std::vector<SgStatement*> lookingForMarkedStmt(SgGlobal* root) {
  if(DEBUG) DBG_MAQAO
  SgDeclarationStatementPtrList& declList = root->get_declarations ();
  std::vector<SgStatement*> stmtMarkedList;
  //Travel throught declarations Statements
  for (SgDeclarationStatementPtrList::iterator p = declList.begin(); p != declList.end(); ++p) {
    if (SgModuleStatement* modstmt = isSgModuleStatement(*p)) {
      //printAllSrcLoopsFromModules(project);
      SgClassDefinition * modDef =  modstmt->get_definition ();
      SgDeclarationStatementPtrList & funcList =  modDef->getDeclarationList ();
      for (SgDeclarationStatementPtrList::iterator it = funcList.begin(); it != funcList.end(); ++it) {
        SgFunctionDeclaration *func = isSgFunctionDeclaration(*it);
        if (func == NULL)
          continue;
        SgFunctionDefinition *defn = func->get_definition();
        if (defn == NULL)
          continue;

        SgInitializedNamePtrList paramList = func->get_parameterList()->get_args ();
        SgSymbolTable * st = defn->get_symbol_table();

        for(int i=0; i < paramList.size(); i++) {
          SgSymbol* vsym = st->find_variable(paramList[i]->get_name().getString());
        }

        SgBasicBlock *body = defn->get_body();
        lookingForMarkedStmt(body, stmtMarkedList);
      }
    } else 
    {
      SgFunctionDeclaration *func = isSgFunctionDeclaration(*p);
      if (func == NULL)
        continue;
      SgFunctionDefinition *defn = func->get_definition();
      if (defn == NULL)
        continue;

      SgInitializedNamePtrList paramList = func->get_parameterList()->get_args ();
      SgSymbolTable * st = defn->get_symbol_table();

      for(int i=0; i < paramList.size(); i++) {
        SgSymbol* vsym = st->find_variable(paramList[i]->get_name().getString());
      }

      SgBasicBlock *body = defn->get_body();
      lookingForMarkedStmt(body, stmtMarkedList);

    }
  }
  return stmtMarkedList;
}

void lookingForMarkedStmt(SgBasicBlock *body, std::vector<SgStatement*> &stmtMarkedList) {
  if(DEBUG) DBG_MAQAO
  
  SgStatementPtrList & stmtsList = body->get_statements();

  for (SgStatementPtrList::iterator p = stmtsList.begin(); p != stmtsList.end(); ++p) 
  {      
    if (SgFunctionDeclaration * func = isSgFunctionDeclaration(*p)) {
      //TODO Look before if we found MAQAO ANALYZE
      if (SgFunctionDefinition *defn = func->get_definition()) {
        SgBasicBlock *body = defn->get_body();
        lookingForMarkedStmt(body, stmtMarkedList);
      }
    } else if (SgForStatement * loop = isSgForStatement(*p)) {
      // Look before if we found MAQAO ANALYZE
      std::vector<std::string> directives = get_MAQAO_directives(loop);
      for (int i=0; i < directives.size(); i++) {
        //For all maqao directives looking for MAQAO ANALYZE
        if (directives[i].find ("MAQAO ANALYZE") != std::string::npos) {
          stmtMarkedList.push_back(loop);
        }
      }
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body());
      if (subbody)
        lookingForMarkedStmt(subbody, stmtMarkedList);
    } else if (SgFortranDo * loop = isSgFortranDo(*p)) {
      //TODO Look before if we found MAQAO ANALYZE
      std::vector<std::string> directives = get_MAQAO_directives(loop);
      for (int i=0; i < directives.size(); i++) {
        //For all maqao directives looking for MAQAO ANALYZE
        if (directives[i].find ("MAQAO ANALYZE") != std::string::npos) {
          stmtMarkedList.push_back(loop);
        }
      }

      lookingForMarkedStmt(loop->get_body(), stmtMarkedList);
    } else if (SgDoWhileStmt * loop = isSgDoWhileStmt(*p)) { 
      //TODO Look before if we found MAQAO ANALYZE
      std::vector<std::string> directives = get_MAQAO_directives(loop);
      for (int i=0; i < directives.size(); i++) {
        //For all maqao directives looking for MAQAO ANALYZE
        if (directives[i].find ("MAQAO ANALYZE") != std::string::npos) {
          stmtMarkedList.push_back(loop);
        }
      }
      SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
      if(subbody)
        lookingForMarkedStmt(subbody, stmtMarkedList);
    } else if (SgWhileStmt * loop = isSgWhileStmt(*p)) {
      //TODO Look before if we found MAQAO ANALYZE
      std::vector<std::string> directives = get_MAQAO_directives(loop);
      for (int i=0; i < directives.size(); i++) {
        //For all maqao directives looking for MAQAO ANALYZE
        if (directives[i].find ("MAQAO ANALYZE") != std::string::npos) {
          stmtMarkedList.push_back(loop);
        }
      }

      SgBasicBlock * subbody = isSgBasicBlock(loop->get_body());
      if(subbody)
        lookingForMarkedStmt(subbody, stmtMarkedList);
    } else if (SgIfStmt * ifstmt = isSgIfStmt(*p)) {
      //TODO Look before if we found MAQAO ANALYZE
      std::vector<std::string> directives = get_MAQAO_directives(ifstmt);
      for (int i=0; i < directives.size(); i++) {
        //For all maqao directives looking for MAQAO ANALYZE
        if (directives[i].find ("MAQAO ANALYZE") != std::string::npos) {
          stmtMarkedList.push_back(ifstmt);
        }
      }

      SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ());
      SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ());
      if(bodytrue)
        lookingForMarkedStmt(bodytrue, stmtMarkedList);
      if(bodyfalse)
        lookingForMarkedStmt(bodyfalse, stmtMarkedList);
    } else if (SgSwitchStatement* switchStmt = isSgSwitchStatement(*p)) {
      //TODO Look before if we found MAQAO ANALYZE
      std::vector<std::string> directives = get_MAQAO_directives(loop);
      for (int i=0; i < directives.size(); i++) {
        //For all maqao directives looking for MAQAO ANALYZE
        if (directives[i].find ("MAQAO ANALYZE") != std::string::npos) {
          stmtMarkedList.push_back(loop);
        }
      }

        if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body()))
          lookingForMarkedStmt(subbody, stmtMarkedList);
    } else if (SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body()))
          lookingForMarkedStmt(subbody, stmtMarkedList);
    } else if (SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(*p)) {
      if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body()))
          lookingForMarkedStmt(subbody, stmtMarkedList);
    } else if (SgBasicBlock * bb = isSgBasicBlock(*p)) {
      lookingForMarkedStmt(bb, stmtMarkedList);
    } else if (SgScopeStatement * scp = isSgScopeStatement(*p)) {
        if (VERBOSE || DEBUG)
          std::cout << "----------THIS SCOPE WAS NOT HANDLED YET: " << scp->class_name() << std::endl;
    } else if (SgExprStatement * exprStmt = isSgExprStatement(*p)) {
      //std::cout << "[lookingForMarkedStmt] exprStmt : " << exprStmt->get_expression()->class_name() << std::endl;
    }
  }
}

///////////////////
// LOG functions //
///////////////////
static int log_number = 0;
void log (std::string what_change, SgLocatedNode* node, bool writeInFile/*=true*/) {
  if (DEBUG) DBG_MAQAO
  SgSourceFile * srcfile = getEnclosingsourceFile_RosePtr(node);
  SgFunctionDefinition * funcD = get_function_RosePtr(node);

  std::string filename = "log_"+srcfile->get_sourceFileNameWithoutPath ()+".log";
  std::ofstream file;

  if(log_number == 0) {
    file.open(filename.c_str(), std::ios::out);
  } else {
    file.open(filename.c_str(), std::ios::out | std::ofstream::app);
  }
  
  if(file && writeInFile) { // write in a log file
    std::ostringstream line;
    line << node->get_file_info()->get_line();
    file << "--------------------- " << "LOG " << log_number++ << "--------------------- " << std::endl;
    file << "In " + funcD->get_declaration()->get_name().getString () << std::endl;
    file << "l."+line.str() << " : " ;
    file << what_change << std::endl;
    file << node->unparseToString() << std::endl;
    file << "----------------------------------------------- " << std::endl;
    file.close();
  } else { // write in std out
    std::ostringstream line;
    line << node->get_file_info()->get_line();
    std::cout << "--------------------- " << "LOG " << log_number++ 
              << " " << srcfile->get_sourceFileNameWithoutPath ()
              <<"--------------------- " << std::endl;
    std::cout << "In " + funcD->get_declaration()->get_name().getString () << std::endl;
    std::cout << "l."+line.str() << " : " ;
    std::cout << what_change << std::endl;
    std::cout << node->unparseToString() << std::endl;
    std::cout << "----------------------------------------------- " << std::endl;
  }
}

void log (std::string what_change, std::string filename/*=""*/) {
  if(DEBUG) DBG_MAQAO

  if (filename != "") {
    std::ofstream file;
    if(log_number == 0) {
      file.open(filename.c_str(), std::ios::out);
    } else {
      file.open(filename.c_str(), std::ios::out | std::ofstream::app);
    }
    if(file) { // write in a log file
      file << "--------------------- " << "LOG " << log_number++ << " --------------------- " << std::endl;
      file << what_change << std::endl;
      file << "----------------------------------------------- " << std::endl;
      file.close();
    } else { // write in std out
      std::cout << "--------------------- " << "LOG " << log_number++ << "--------------------- " << std::endl;
      std::cout << what_change << std::endl;
      std::cout << "----------------------------------------------- " << std::endl;
    }
  } else { // write in std out
    std::cout << "--------------------- " << "LOG " << log_number++ << "--------------------- " << std::endl;
    std::cout << what_change << std::endl;
    std::cout << "----------------------------------------------- " << std::endl;
  }
}

// =====================================================================
//////////////////////////////
// Function about functions //
//////////////////////////////
SgFunctionDeclaration* build_functionDecl(SgFunctionDeclaration* func_to_cpy, 
                                          std::vector<variable_spe*> var,
                                          SgBasicBlock* otherBody) 
{
  if(DEBUG) DBG_MAQAO
  SgGlobal * globalScope = SageInterface::getGlobalScope(func_to_cpy);
  SgFunctionDeclaration* funcD = NULL;
  SgSymbolTable * st = func_to_cpy->get_definition()->get_symbol_table();
  SgSymbolTable * gst = globalScope->get_symbol_table();
  std::vector<SgSymbol*> symbol_to_ignor;
  std::vector<int>       value_of_symbol;

  for(int i=0; i < var.size(); i++) {
    if (var[i]->specialization == variable_spe::EQUALS) { 
      SgSymbol* vsym = st->find_variable(std::string(var[i]->var_name));if (!vsym) {
        SgBasicBlock * bb = isSgBasicBlock(func_to_cpy->get_definition()->get_body());
        SgSymbolTable * stbb = bb->get_symbol_table();
        vsym = stbb->find_variable(std::string(var[i]->var_name));
        
        if (!vsym) {
          if(LOG) {
            std::cout << "The function "<< func_to_cpy->get_name()
                      << " will not be specialize, because " << var[i]
                      << " is not a parameter of the function or declare in the body of the function"
                      << std::endl;
          }
          return NULL;
        }
      }
      symbol_to_ignor.push_back(vsym);
      value_of_symbol.push_back(var[i]->value);
    }
  }

  if (globalScope != NULL) {

    // Name of the new function
    std::ostringstream convert;
    std::string func_new_name = func_to_cpy->get_name().getString()+"_ASSIST";
    for(int i=0; i < var.size(); ++i) {
      std::string opstring = "";
      if (var[i]->specialization == variable_spe::EQUALS) {
        convert << var[i]->value;
        opstring="e";
        func_new_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
      }
      else if (var[i]->specialization == variable_spe::INF) {
        convert << var[i]->sup_bound;
        opstring="i";
        func_new_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
      }
      else if (var[i]->specialization == variable_spe::SUP) {
        convert << var[i]->inf_bound;
        opstring="s";
        func_new_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
      }
      else if (var[i]->specialization == variable_spe::INTERVAL) {
        convert << (var[i]->inf_bound+1);
        opstring="b";
        func_new_name += "_"+std::string(var[i]->var_name)+opstring+convert.str();
        convert.clear();
        convert.str("");
        convert << (var[i]->sup_bound-1);
        func_new_name += "_"+convert.str();
      }
      //The character '-' isn't valide for a function name
      std::replace( func_new_name.begin(), func_new_name.end(), '-', 'm');

      convert.clear();
      convert.str("");
    }

    std::replace( func_new_name.begin(), func_new_name.end(), '-', 'm');
    if(DEBUG>2) std::cout << "Build " << func_new_name << std::endl;
    bool add_symbol_to_arg;
    SgFunctionParameterList* newParamList = SageBuilder::buildFunctionParameterList(); 
    SgInitializedNamePtrList& argList =  isSgFunctionParameterList(func_to_cpy->get_parameterList())->get_args ();
    for(SgInitializedNamePtrList::iterator it_arg = argList.begin(); it_arg != argList.end(); ++it_arg) {
      add_symbol_to_arg= true;
      for(int i=0; i < var.size(); ++i) {
        if ( (*it_arg)->get_symbol_from_symbol_table()->get_name() == std::string(var[i]->var_name)
          && var[i]->specialization == variable_spe::EQUALS) {
          add_symbol_to_arg = false;
        }
      }
      if(add_symbol_to_arg) {
        newParamList->append_arg(SageBuilder::buildInitializedName((*it_arg)->get_symbol_from_symbol_table()->get_name(),(*it_arg)->get_symbol_from_symbol_table()->get_type() ));
      }
    }

    //Generation of the new function
    funcD = generateFunction(
      newParamList,
      func_new_name, 
      globalScope);
    newParamList->set_parent(funcD);

    //Set scopes and placement in source code
    globalScope->append_declaration (funcD);
    SageInterface::updateDefiningNondefiningLinks(funcD, globalScope);
    funcD->set_scope (globalScope);
    funcD->set_parent (globalScope);
    if (otherBody) {
      SgBasicBlock * newBody = s2s_api::copyBasicBlock(otherBody,symbol_to_ignor, value_of_symbol);
      funcD->get_definition()->set_body(newBody);
      newBody->set_parent(funcD->get_definition());

      newBody->set_endOfConstruct(TRANSFORMATION_FILE_INFO);                        
      newBody->get_endOfConstruct()->set_parent(newBody);
    } else {      
      SgBasicBlock * newBody = isSgBasicBlock(SageInterface::copyStatement(func_to_cpy->get_definition()->get_body()));
      funcD->get_definition()->set_body(newBody);
      newBody->set_parent(funcD->get_definition());

      newBody->set_endOfConstruct(TRANSFORMATION_FILE_INFO);                        
      newBody->get_endOfConstruct()->set_parent(newBody);
    }

  } else {if (DEBUG > 1) DBG_MAQAO}
  return funcD;
}

//! Creates a non-member function.
SgFunctionDeclaration *
createFuncSkeleton (const std::string& name, SgType* ret_type,
                    SgFunctionParameterList* params, SgScopeStatement* scope)
{
  if(DEBUG) DBG_MAQAO
  ROSE_ASSERT(scope != NULL);
  ROSE_ASSERT(isSgGlobal(scope)!=NULL);
  SgFunctionDeclaration* func;
  SgProcedureHeaderStatement* fortranRoutine;
  // generate SgProcedureHeaderStatement for Fortran code
  if (SageInterface::is_Fortran_language()) {
    fortranRoutine = SageBuilder::buildProcedureHeaderStatement(name.c_str(),ret_type, params, SgProcedureHeaderStatement::e_subroutine_subprogram_kind,scope);
    func = isSgFunctionDeclaration(fortranRoutine);  
  } else {
    func = SageBuilder::buildDefiningFunctionDeclaration(name,ret_type,params,scope);
  }

  ROSE_ASSERT (func != NULL);

  SgFunctionSymbol* func_symbol = scope->lookup_function_symbol(func->get_name());
  ROSE_ASSERT(func_symbol != NULL);
  if (Outliner::enable_debug) {
    printf("Found function symbol in %p for function:%s\n",scope,func->get_name().getString().c_str());
  }

  return func;
}

SgFunctionDeclaration * generateFunction (SgFunctionParameterList * paramList,/* SgBasicBlock* s,*/
                            const std::string& func_name_str,
                            SgScopeStatement* scope)
{
  if(DEBUG) DBG_MAQAO
  ROSE_ASSERT (&scope);
  ROSE_ASSERT(isSgGlobal(scope));
  // step 1: perform necessary liveness and side effect analysis, if requested.

  // step 2. Create function skeleton, 'func'.
  // -----------------------------------------
  SgName func_name (func_name_str);
  SgFunctionParameterList * parameterList = SageBuilder::buildFunctionParameterList();
  SgInitializedNamePtrList::iterator i;
  
  for (i=paramList->get_args().begin();i!=paramList->get_args().end();i++) {
    SgInitializedName* arg = SageBuilder::buildInitializedName((*i)->get_name(), (*i)->get_type());
    SageInterface::appendArg(parameterList,arg);
  }

  SgFunctionDeclaration* func = createFuncSkeleton (func_name,SgTypeVoid::createType (),parameterList, scope);
  ROSE_ASSERT (func);

  //step 3. Create the function body
  // -----------------------------------------
  // Generate the function body by deep-copying 's'.
  // SgBasicBlock* func_body = func->get_definition()->get_body();
  // ROSE_ASSERT (func_body != NULL);

  // This does a copy of the statements in "s" to the function body of the outlined function.
  // ROSE_ASSERT(func_body->get_statements().empty() == true);

  //func_body = isSgBasicBlock( SageInterface::deepCopyNode(s));

  func->set_type(SageBuilder::buildFunctionType(func->get_type()->get_return_type(), SageBuilder::buildFunctionParameterTypeList(func->get_parameterList())));

  // Retest this...
  //ROSE_ASSERT(func->get_definition()->get_body()->get_parent() == func->get_definition());
  ROSE_ASSERT(scope->lookup_function_symbol(func->get_name()));

  return func;
}

//////////////////////////////
//      Static analyse      //
//////////////////////////////

// Not finised
// Currently only print the weight of variable
// the calculation of the weight is maybe too much simple.
void analyze_variables(SgGlobal * globalScope) {
  if(DEBUG) DBG_MAQAO
  SgFunctionDeclaration* funcDecl = SageInterface::findFirstDefiningFunctionDecl(globalScope);
  std::vector<var_metric*> var_m_list;

  while (funcDecl) {
    if (!funcDecl->get_file_info()->isTransformation() && !SageInterface::isMain (funcDecl)) {
      std::string argsString = "";
      SgInitializedNamePtrList & argsList = funcDecl->get_args ();
      SgSymbolTable * st = funcDecl->get_definition()->get_symbol_table();

      for (int i=0; i < argsList.size(); i++) {
        SgType * argType = argsList[i]->get_type ();
      
        if (isSgTypeInt(argType->dereference ()) 
          || isSgTypeLong(argType->dereference ()) 
          || isSgTypeFloat(argType->dereference ()) 
          || isSgTypeDouble(argType->dereference ())
          || isSgArrayType(argType->dereference ())
          || isSgPointerType(argType->dereference ())) {

          if (SgArrayType* arrayType = isSgArrayType(argType)) {
            continue; //To avoid to handle Arrays
          }
          if (SgPointerType* arrayType = isSgPointerType(argType)) {
            continue; //To avoid to handle Arrays
          }

          //argsString += argsList[i]->get_name().getString() + ",";
          SgSymbol* vsym = st->find_variable(argsList[i]->get_name().getString());
          if (!vsym) {
            SgBasicBlock * bb = isSgBasicBlock(funcDecl->get_definition()->get_body());
            SgSymbolTable * stbb = bb->get_symbol_table();
            vsym = stbb->find_variable(argsList[i]->get_name().getString());
            // If the variable doesn't exist there is a problem  
            if (!vsym) {
              if(LOG) {
                std::cout << "The function "<< funcDecl->get_name()
                          << " will not be handle, because " << argsList[i]->get_name().getString()
                          << " is not a parameter of the function or declare in the body of the function"
                          << std::endl;
              }
              return;
            }
          }
          var_metric* vm = new var_metric(isSgVariableSymbol(vsym));
          var_m_list.push_back(vm);
        }

        //Add calls to the vprof lib to the function.
        //add_lib_call(funcDecl->get_definition(), argsString);
      }
      //check all variables for the function 
      weightingVariables(funcDecl->get_definition()->get_body(), var_m_list);

      //Print variables
      std::cout << "For the function \"" << funcDecl->get_name().getString() << "\" we obtained : " << std::endl;
      for (int p=0; p < var_m_list.size(); p++) {
        // std::cout << var_m_list[p]->varSym->get_name().getString() << " ";
        var_m_list[p]->printweight();
      }
    }

    SgStatement* nextStmt = SageInterface::getNextStatement (funcDecl);

    while (isSgPragmaDeclaration(nextStmt) && nextStmt) {
      nextStmt = SageInterface::getNextStatement (nextStmt);
    }

    funcDecl = isSgFunctionDeclaration(nextStmt);    
    if (DEBUG && nextStmt) std::cout << nextStmt->class_name() << std::endl;
  }
}

bool isTheOnlyVar(SgExpression* expr, SgVariableSymbol* varSym) {
  if(DEBUG) DBG_MAQAO
  if(!expr || !varSym) return false;

  if (SgBinaryOp * bo = isSgBinaryOp(expr)) {
    if (!isTheOnlyVar(bo->get_rhs_operand(), varSym)) return false;
    if (!isTheOnlyVar(bo->get_lhs_operand(), varSym)) return false;
  }
  else if (SgFunctionCallExp * fcallexp = isSgFunctionCallExp(expr)) {
    SgExpressionPtrList argList = fcallexp->get_args ()->get_expressions ();
    for (SgExpressionPtrList::iterator itArgList = argList.begin(); itArgList != argList.end(); ++itArgList) {
      if(!isTheOnlyVar ((*itArgList), varSym)) return false;
    }
  }
  else if (isSgExprListExp(expr)) {
    SgExpressionPtrList expList = isSgExprListExp(expr)->get_expressions();
    for (SgExpressionPtrList::iterator it = expList.begin(); it != expList.end(); it++) {
      if (!isTheOnlyVar ((*it), varSym)) 
        return false;
    }
  }
  else if(SgVarRefExp* varref = isSgVarRefExp(expr)) {
    if(varref->get_symbol()->get_name().getString() != varSym->get_name().getString()) 
      return false;
  } 
  else if (isSgValueExp(expr) || isSgNullExpression(expr)) {
    return true;
  } 
  else {
    if (DEBUG) DBG_MAQAO
    std::cout << "The class "  << expr->class_name() << " is not handle yet " << std::endl;
  }
  return true;
}

void weightingVariables(SgBasicBlock* body, std::vector<var_metric*> & var_m_list) {
  if(DEBUG) DBG_MAQAO
  SgNode* node = node = SageInterface::getFirstStatement(body);

    while (node) {
      switch((node)->variantT ()) {
        case V_SgForStatement: {
          SgForStatement  * loop = isSgForStatement(node);
          for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
            //Check init
            SgStatementPtrList & init_loop = loop->get_init_stmt ();
            for (int it = 0; it < init_loop.size() ; it++) {
              if ( isinExpr(isSgBinaryOp(isSgExprStatement(init_loop[it])->get_expression())->get_rhs_operand(), var_m_list[it_var]->varSym) )
                var_m_list[it_var]->numberInLoopCond;
            }
            //Check condition
            SgExprStatement * cond = isSgExprStatement(loop->get_test ());
            if ( isinExpr(cond->get_expression(), var_m_list[it_var]->varSym) ) {
              var_m_list[it_var]->numberInLoopCond++;
            }
            
            //check Incr
            if ( isinExpr(loop->get_increment (), var_m_list[it_var]->varSym) ){
              var_m_list[it_var]->numberInLoopCond++;
            }
          }
          //Check the body
          if ( SgBasicBlock * subbody = isSgBasicBlock(loop->get_loop_body()) ) 
            weightingVariables(subbody, var_m_list);
          //End checking this node
          break;
        }
        case V_SgFortranDo: {
          SgFortranDo * loop = isSgFortranDo(node);
          for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
            //Check init
            if (isinExpr(loop->get_initialization(), var_m_list[it_var]->varSym)) {
              var_m_list[it_var]->numberInLoopCond++;
            } 

            //Check cond
            if (isinExpr(loop->get_bound(), var_m_list[it_var]->varSym)) {
              var_m_list[it_var]->numberInLoopCond++;
            }

            //Check Incr
            if (isinExpr(loop->get_increment(), var_m_list[it_var]->varSym)) {
              var_m_list[it_var]->numberInLoopCond++;
            }
          }
          //Check Body
          if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
            weightingVariables(subbody, var_m_list);

          break;
        }
        case V_SgDoWhileStmt: {
          SgDoWhileStmt * loop = isSgDoWhileStmt(node);
          for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
            //Check condition
            SgExprStatement * cond = isSgExprStatement(loop->get_condition ());
            if ( isinExpr(cond->get_expression(), var_m_list[it_var]->varSym) ) {
              var_m_list[it_var]->numberInLoopCond++;
            }
          }
          //Check Body
          if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
            weightingVariables(subbody, var_m_list);

          break;
        }
        case V_SgWhileStmt: {
          SgWhileStmt  * loop = isSgWhileStmt(node);
          for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
            //Check condition
            SgExprStatement * cond = isSgExprStatement(loop->get_condition ());
            if ( isinExpr(cond->get_expression(), var_m_list[it_var]->varSym) ) {
              var_m_list[it_var]->numberInLoopCond++;
            }
            
            //Check Body
            if(SgBasicBlock * subbody = isSgBasicBlock(loop->get_body()))
              weightingVariables(subbody, var_m_list);
          }
          break;
        }
        case V_SgIfStmt: {
          SgIfStmt * ifstmt = isSgIfStmt(node);
          //Check condition
          SgExprStatement * cond = isSgExprStatement(ifstmt->get_conditional());
          for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
            if ( isinExpr(cond->get_expression(), var_m_list[it_var]->varSym) ) {
              var_m_list[it_var]->numberInIfCond++;
            }
          }
          //Check True Body
          if(SgBasicBlock * bodytrue = isSgBasicBlock(ifstmt->get_true_body ())) {
            weightingVariables(bodytrue, var_m_list);
          }
          //Check False Body
          if(SgBasicBlock * bodyfalse = isSgBasicBlock(ifstmt->get_false_body ())) {
            weightingVariables(bodyfalse, var_m_list);
          }
          break;
        }
        case V_SgSwitchStatement: {
          SgSwitchStatement* switchStmt = isSgSwitchStatement(node);

          //Check item selector
          SgExprStatement * cond = isSgExprStatement(switchStmt->get_item_selector ());
          for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
            if ( isinExpr(cond->get_expression(), var_m_list[it_var]->varSym) ) {
              var_m_list[it_var]->numberInIfCond++;
            }
          }
          //Check cases
          if(SgBasicBlock * subbody = isSgBasicBlock(switchStmt->get_body())) {
            weightingVariables(subbody, var_m_list);
          }
          break;
        }
        case V_SgCaseOptionStmt: {
          SgCaseOptionStmt* caseOption = isSgCaseOptionStmt(node);
          if(SgBasicBlock * subbody = isSgBasicBlock(caseOption->get_body())) {
            weightingVariables(subbody, var_m_list);
          }
          break;
        }
        case V_SgDefaultOptionStmt: {
          SgDefaultOptionStmt* defaultCaseOption = isSgDefaultOptionStmt(node);
          if(SgBasicBlock * subbody = isSgBasicBlock(defaultCaseOption->get_body())) {
            weightingVariables(subbody, var_m_list);
          }
          break;
        }
        case V_SgBasicBlock: {
          SgBasicBlock* subbody = isSgBasicBlock(node);
          weightingVariables(subbody, var_m_list);
          break;
        }
        case V_SgScopeStatement: {
          SgScopeStatement * scp = isSgScopeStatement(node);
          if(DEBUG) {
            DBG_MAQAO std::cout << "-----THIS SCOPE WAS NOT HANDLED YET: " << scp->class_name() << std::endl;
          }
          // /!\
          // This function do its job, 
          // but if we launch an other analyze just after it crash because it doesn't delete something in the memory pool
          // so, don't use it plz !
          //createAST(SageBuilder::buildBasicBlock_nfi(scp->getStatementList()));
          break;
        }
        case V_SgExprStatement : {
          SgExprStatement* exprstmt = isSgExprStatement(node);
          for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
            //Check in any arithmetic expression
            if ( isinExpr(exprstmt->get_expression(), var_m_list[it_var]->varSym) ) {
              //Check if it's an assignation and if the variable was known did we deduce lhs 
              if (SgAssignOp* bo = isSgAssignOp(exprstmt->get_expression())) {
                if ( isinExpr(bo->get_rhs_operand(), var_m_list[it_var]->varSym)) {
                  var_m_list[it_var]->numberInArithmStmt++;

                  if (isTheOnlyVar(bo->get_rhs_operand(), var_m_list[it_var]->varSym)) {
                    var_m_list[it_var]->numberVariablesImplies++;

                    // Check if we should add the variable
                    if (SgVarRefExp* varRef = isSgVarRefExp(bo->get_lhs_operand())) {
                      if (!isSgArrayType(varRef->get_type()) && !isSgPointerType(varRef->get_type())) {
                        var_metric * new_var = new var_metric(varRef->get_symbol());
                        var_m_list.push_back(new_var);
                      }
                    }
                  }
                }
              } else {
                var_m_list[it_var]->numberInArithmStmt++;
              }
            }
          }
          break;
        }
        case V_SgVariableDeclaration : {
          SgVariableDeclaration* declvar = isSgVariableDeclaration(node);
          if(isSgArrayType(declvar->get_definition()->get_type())) {
            SgExprListExp * listExpr = isSgArrayType(declvar->get_definition()->get_type())->get_dim_info ();
            SgExpressionPtrList & expreList =  listExpr->get_expressions ();
            for (int it_var = 0 ; it_var < var_m_list.size(); it_var++) {
              for(SgExpressionPtrList::iterator it = expreList.begin(); it != expreList.end(); ++it) {
                if (isinExpr((*it), var_m_list[it_var]->varSym)) {
                  var_m_list[it_var]->nummberMisc++;
                }
              }
            }
          }
          break;
        }
        default: 
          printdebug("Reach default : "+node->class_name());
      } // end switch

      if (isSgStatement(node))
        node = SageInterface::getNextStatement(isSgStatement(node));
      else {
        DBG_MAQAO
        std::cout << "/!\\ WARNING: Something which is not a statement appeared !! \n" << node->class_name() << std::endl;
        return;
      }
    } // end while (node)
}

// Return the place where we find a continue keyword
// 0 represent the first statement
// Maybe later we will add other stop keyword as return or break
int thereIsAStopStmt(SgStatement* stmt) {
  if (DEBUG) DBG_MAQAO

  // return 0 if it's only continue or break inside
  if (isSgContinueStmt(stmt)) {
     return 0;
  }
  if(SgBasicBlock* body = isSgBasicBlock(stmt)) {
    SgStatementPtrList & listStmt = body->get_statements ();
    for (int i = 0; i < listStmt.size(); ++i) {
      if (isSgContinueStmt(listStmt[i])) {
        return i;
      }
    }
  }
  return -1;
}

void whatSpecialize (SgStatement* sc, std::vector<SgVariableSymbol*> &varList) {
  if(DEBUG) DBG_MAQAO

  if (SgIfStmt* ifstmt = isSgIfStmt(sc)) {
    // Check the true body
    whatSpecialize(ifstmt->get_true_body(), varList);

    //Check the false body if exist
    if (ifstmt->get_false_body()) {
      whatSpecialize(ifstmt->get_true_body(), varList);
    }

    //Check the condition
    SgExpression * cond = isSgExprStatement(ifstmt->get_conditional())->get_expression();
    int varListSize_before = varList.size();
    whatSpecialize(cond, varList);
    
    //Record var names
    int varListSize_after = varList.size();
    std::string new_vars= "";
    if (varListSize_after > varListSize_before) {
      for (int i = varListSize_before; i < varListSize_after; ++i) {
        new_vars += varList[i]->get_name().getString();
        if (i+1 < varListSize_after) {
          new_vars += ",";
        }
      }

      std::string uniqueName = /*Rose::*/StringUtility::numberToString(cond);
      if (!ifstmt->get_false_body()) {
        s2s_api::add_directive(ifstmt, std::string("MAQAO ANALYZE IF-\""+uniqueName+"\" --var:"+new_vars));
      } else {
        s2s_api::add_directive(ifstmt, std::string("MAQAO ANALYZE IFELSE-\""+uniqueName+"\" --var:"+new_vars));
      }
      add_lib_call_if(ifstmt, uniqueName);
    }
  } else if (SgExprStatement* exprStmt = isSgExprStatement(sc)) {
    whatSpecialize(exprStmt->get_expression(), varList);
  } else if (SgBasicBlock* body = isSgBasicBlock(sc)) {
    SgStatementPtrList & stmtsList = body->get_statements (); 
    for (int i=stmtsList.size()-1; i >= 0; --i) {
      s2s_api::whatSpecialize(stmtsList[i], varList); 
    }
  } else if (isSgForStatement(sc) || isSgFortranDo(sc)) {
    //Do not handl these cases, they're already handled in a higher level.
    //Not very clean but it works for what I need... sorry
  } else {
    // DBG_MAQAO
    //std::cout << "In \"whatSpecialize\" the class " << sc->class_name() << " is not handle yet." << std::endl;
  }
}

void whatSpecialize (SgExpression* expr, std::vector<SgVariableSymbol*> &varList) {
  if(DEBUG) DBG_MAQAO

  if (isSgAssignOp(expr) || isSgCompoundAssignOp(expr)) { // if it's an assignation operation; we have to check in the list if the lhs is not in the list otherway we have to remove it
    SgBinaryOp* assOp = isSgBinaryOp(expr);
    if (SgVarRefExp* varRef = isSgVarRefExp(assOp->get_lhs_operand_i())) {
      SgVariableSymbol* varSymLHS = varRef->get_symbol();

      for (int i=0; i < varList.size(); ++i) {
        // If we found it we can reove it and exit, because there only one iteration of the variable in the list
        if (varSymLHS->get_name().getString() == varList[i]->get_name().getString()) {
          // std::cout << "ERASE : " << varList[i]->get_name().getString() << std::endl;
          varList.erase(varList.begin()+i);
          // And add all other variable which involved the one we just erased
          whatSpecialize(assOp->get_rhs_operand_i(), varList);
          break;
        }
      }
    }
  } else if (SgVarRefExp* varRef = isSgVarRefExp(expr)) { // We found a variable, we have to add it in the list if not already in it
    bool isLareadyInIt = false;
    SgVariableSymbol* varSym = varRef->get_symbol();
    for (int i=0; i < varList.size(); ++i) {
      if (varSym->get_name().getString() == varList[i]->get_name().getString()) {
        isLareadyInIt = true;
        break;
      }
    }
    if (!isLareadyInIt) {
      // std::cout << "PUSH BACK : " << varRef->get_symbol()->get_name().getString() << std::endl;
      varList.push_back(varRef->get_symbol());
    }
  } else if (SgUnaryOp* uop = isSgUnaryOp(expr)) { // Check unary Operation
    whatSpecialize(uop->get_operand_i(), varList);
  } else if (SgBinaryOp* binop = isSgBinaryOp(expr)) { // Check deeper in the binary operation
    whatSpecialize(binop->get_lhs_operand_i (), varList);
    whatSpecialize(binop->get_rhs_operand_i (), varList);
  }
}

} //end namespace s2s_api