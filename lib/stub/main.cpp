#include "s2sLoop.hpp"
#include "api.hpp"

using namespace s2s_api;

SgProject* openProject (char* input, char* outputprefix,  int numberOfOptions, char** options);
int addTags(std::vector<ASTLoops*> astloops, int* cpt);
int removeTags(std::vector<ASTLoops*> astloops, std::string directive);
void applyOptions(int numberOfOptions, char** options, SgProject* project, SgGlobal* root, SgSourceFile* file, int nb_variable_spe, variable_spe* var_spe);
bool vprof_lct (profilerLoopInfo* pi, Loop* loop);

// Old default main
extern "C" int intermediaire_cpp (char* input, char* outputprefix , int numberOfLoops, int binLines[], int numberOfOptions, char** options) {
  std::vector<std::string> argv;
  std::string outputName;
  bool sourceFile = false;
  // bool clean_version = true;
  // LOAD prefix 
  if(outputprefix == " ") {
    outputName = "MAQAO_";
  } else {
    outputName = std::string(outputprefix);
    outputName="maqao_";
  }

  //----------------------------------------------
  //           Start operating on AST
  //----------------------------------------------
  SgProject* project = openProject (input, outputprefix, numberOfOptions, options);
  SgSourceFile* file = NULL;
  
  ROSE_ASSERT (project != NULL);
  int cpt = 0;
  for(int itfile=0; itfile < project->numberOfFiles () /*&& !sourceFile*/ ; ++itfile) {
    file = isSgSourceFile(project->get_fileList()[itfile]);
    
    if (s2s_api::isFortranFile(file->getFileName ())) {
      currentFileType = "fortran";
    } else if  (s2s_api::isCCXXFile(file->getFileName ())) {
      currentFileType = "cpp";
    }

    if (currentFileType == "fortran" || currentFileType == "c" || currentFileType == "cpp") {
      //std::cout << "File to analyze : " << file->getFileName () << std::endl;;
      currentFile = file->getFileName ();
      if (outputprefix != " ") {
        outputName = outputName + file->get_sourceFileNameWithoutPath ();
        file->set_unparse_output_filename(outputName);
      }
      if (VERBOSE || DEBUG) std::cout <<" Current file : " << currentFile << std::endl;
      sourceFile = true;

      if (!sourceFile) {
        if(VERBOSE || DEBUG) std::cout << "No fortran or C/C++ file was founded. Program will stop." << std::endl;
        return 1;
      }

      SgGlobal *root = file->get_globalScope();

      // CHECK all options
      applyOptions(numberOfOptions, options, project, root, file, 0, NULL);
    }
  }

  //The end of the 
  int returnID = backend(project);
  if(returnID && VERBOSE) {
    std::cout << "Error ID Return is not 0 : " << returnID << std::endl;    
  }

  return returnID;
}

// (main) Called when the config file (script) is used
extern "C" int main_config_file (char* input, char* outputprefix , int numberOfLoops, configLoop loops[], int numberOfFuncs, configFunc funcs[], int numberOfOptions, char** options) {
  std::string outputName;
  bool sourceFile = false;

  // LOAD prefix 
  if(outputprefix == " ") {
    outputName = "MAQAO_";
  } else {
    outputName = std::string(outputprefix);
  }

  SgProject* project = openProject (input, outputprefix, numberOfOptions, options);
  SgSourceFile* file = NULL;
  if (DEBUG) DBG_MAQAO
  
  //std::cout << "Start operating on AST\n";
  ROSE_ASSERT (project != NULL);
  int cpt = 0;

  for(int itfile=0; itfile < project->numberOfFiles () /*&& !sourceFile*/ ; ++itfile) {
    file = isSgSourceFile(project->get_fileList()[itfile]);

    currentFileType = "";
    if (s2s_api::isFortranFile(file->getFileName ())) {
      currentFileType = "fortran";
    } else if  (s2s_api::isCCXXFile(file->getFileName ())) {
      currentFileType = "cpp";
    }

    if (currentFileType == "fortran" || currentFileType == "c" || currentFileType == "cpp") {
      //std::cout << "File to analyze : " << file->getFileName () << std::endl;;
      currentFile = file->getFileName ();
      if (outputprefix != " ") {
        outputName = outputName + file->get_sourceFileNameWithoutPath ();
        file->set_unparse_output_filename(outputName);
      }

      if (VERBOSE) std::cout << " Current file is : " << currentFile << std::endl;
      sourceFile = true;

      if (!sourceFile) {
        if (VERBOSE) std::cout << "No fortran or C/C++ file was founded. Program will stop." << std::endl;
        return 1;
      }

      SgGlobal *root = file->get_globalScope();
      
      bool found = false;

      std::vector<configLoop> loop_remaining;
      std::vector<configFunc> func_remaining;

      size_t ldm_it = 0;

      std::string s = "";

      /***************
       **   FUNCS   **
       ***************/
      ASTFunction astF(root);
      if(DEBUG) std::cout << "ANALYZING FUNCTIONS" << std::endl;
      // Add directives on functions
      for(int nbf=0; nbf < numberOfFuncs; nbf++) {
        Function * srcFunc = astF.get_function_from_line(funcs[nbf].line);
        // Si on a trouvé la boucle, on la garde
        if (srcFunc) {
          srcFunc->set_matching_with_bin_loop(true);
          srcFunc->set_id(funcs[nbf].id);

          for (int itdir = 0 ; itdir < funcs[nbf].nbtransfo; itdir++) {
            s2s_api::add_directive(srcFunc->get_function()->get_declaration(), std::string("MAQAO ")+std::string(funcs[nbf].transfo[itdir]));
          }

        } else  { // sinon on recherchera à +/- delta après avec les boucles restantes
          func_remaining.push_back(funcs[nbf]);
        }
      }

      //If lines are not exactly at the place indicate we're looking 3 lines around
      for(int nbfr=0; nbfr < func_remaining.size(); nbfr++) {
        Function * srcFunc = get_function_from_line(&astF, func_remaining[nbfr].line, S2S_DELTA);

        if(srcFunc) {
          srcFunc->set_matching_with_bin_loop(true);
          srcFunc->set_id(funcs[nbfr].id);

          for (int itdir = 0 ; itdir < funcs[nbfr].nbtransfo; itdir++) {
            s2s_api::add_directive(srcFunc->get_function()->get_declaration(), std::string("MAQAO ")+std::string(funcs[nbfr].transfo[itdir]));
          }

        } else { // if the loop/func doesn't exist, save it to print later
          std::ostringstream stm;
          stm  << func_remaining[nbfr].line;
          s += "func starting at line " + stm.str() + " was not founded.\n";
        }
      }

      /***************
       **   LOOPS   **
       ***************/
      ASTRoot     astL(root);
      if(DEBUG) std::cout << "ANALYZING LOOPS" << std::endl;

      // Add directives on loops
      for(int nbl=0; nbl < numberOfLoops; nbl++) {
        Loop * srcLoop = NULL;

        if (loops[nbl].line == -1) {
          srcLoop = astL.get_loop_from_label(loops[nbl].label);
        } else {
          srcLoop = astL.get_loop_from_line(loops[nbl].line);
        }

        // Si on a trouvé la boucle, on la garde
        if (srcLoop) {
          srcLoop->set_matching_with_bin_loop(true);
          srcLoop->set_id(loops[nbl].id);

          for (int itdir = 0 ; itdir < loops[nbl].nbtransfo; itdir++) {
            if (DEBUG) {
              std::cout << "add the directive :" << std::string("MAQAO ")+std::string(loops[nbl].transfo[itdir]) << std::endl;
              if (loops[nbl].line != -1)
                std::cout << "above the loop line " << loops[nbl].line << std::endl;
              else 
                std::cout << "above the loop labeled " << loops[nbl].label << std::endl;
            }
            s2s_api::remove_MAQAO_directives(srcLoop->get_loop_RosePtr(), loops[nbl].label);
            srcLoop->add_directive(std::string("MAQAO ")+std::string(loops[nbl].transfo[itdir]));
          }

        } else  { // sinon on recherchera à +/- delta après avec les boucles restantes
          if (loops[nbl].line != -1) {
            loop_remaining.push_back(loops[nbl]);
          }
        }
      }

      //If lines are not exactly at the place indicate we're looking 3 lines around
      for(int nblr=0; nblr < loop_remaining.size(); nblr++) {
        Loop * srcLoop = get_loop_from_line(&astL, loop_remaining[nblr].line, S2S_DELTA);

        if(srcLoop) {
          srcLoop->set_matching_with_bin_loop(true);
          srcLoop->set_id(loops[nblr].id);

          for (int itdir = 0 ; itdir < loops[nblr].nbtransfo; itdir++) {
            srcLoop->add_directive(std::string("MAQAO ")+std::string(loops[nblr].transfo[itdir]));
          }

        } else { // if the loop doesn't exist, save it to print later
          std::ostringstream stm;
          stm  << loop_remaining[nblr].line;
          s += "loop starting at " + stm.str() + " was not founded.\n";
        }
      }
      
      if (DEBUG) std::cout << "--- APPLY DIRECTIVE ON FUNCTIONS & LOOPS ---"<<std::endl;
      //s2s_api::browseFile_and_apply_directives(root);
      astF.apply_directive();
      astL.apply_directive();

      if (s != "" && VERBOSE) {
        std::cout << std::endl;
        std::cout << " --------------------------------"<< std::endl;
        std::cout << "loops & functions found in the transformation script but not in source are : " << std::endl;
        std::cout << s << std::endl;
      }
    } // end if 
  }

  int returnID = backend(project);
  if(returnID && DEBUG)
    std::cout << "Error ID Return is not 0 : " << returnID << std::endl;  

  if (VERBOSE) std::cout << "--------------------------------------------" << std::endl;

  return returnID;
}

// (main) Called when we use MAQAO analyzes
// default main
extern "C" int main_oneview (char* input, char* outputprefix, int nb_pi, profilerLoopInfo* pi, int numberOfOptions, char** options) {
  std::vector<std::string> argv;
  std::string outputName;
  bool sourceFile = false;
  // bool clean_version = true;

  // LOAD prefix 
  if(outputprefix != "maqao_") {
    outputName = std::string(outputprefix);
  }

  if (DEBUG) {
    printf("[DEBUG INFO] START\n");
    printf("Profiler loop infos : \n");
    printf("===  :\n");
    for(int i=0; i < nb_pi; ++i ) {
      printf("=======================\n");
      printf("[C++] id : %d\n",pi[i].id);
      printf("[C++] file : %s\n",pi[i].file);
      // printf("[C++] path : %s\n",pi[i].path);
      printf("[C++] lineStart : %d\n",pi[i].lineStart);
      printf("[C++] lineEnd : %d\n",pi[i].lineEnd);
      printf("[C++] nb_ite_min : %d\n",pi[i].nb_ite_min);
      printf("[C++] nb_ite_max : %d\n",pi[i].nb_ite_max);
      printf("[C++] nb_ite_avg : %d\n",pi[i].nb_ite_avg);
      printf("[C++] dl1_ratio_min : %f\n",pi[i].dl1_ratio_min);
      printf("[C++] dl1_ratio_max : %f\n",pi[i].dl1_ratio_max);
      printf("[C++] dl1_ratio_mean : %f\n",pi[i].dl1_ratio_mean);
      printf("[C++] vec_ratio : %f\n",pi[i].vec_ratio);
      printf("=======================\n");
    }

    printf("\n=== OPTIONS : \n ");
    for(int i=0; i < numberOfOptions; ++i ) {
      printf("[C++] Options[%d]= %s\n",i,options[i] );
    }
    printf("[DEBUG INFO] END\n");
    if (DEBUG) printf("===================== End debug info | Start ASSIST =====================\n");
  }

  //----------------------------------------------
  //           Start operating on AST
  //----------------------------------------------
  SgProject* project = openProject (input, outputprefix, numberOfOptions, options);
  SgSourceFile* file = NULL;
  if (DEBUG) DBG_MAQAO
  
  ROSE_ASSERT (project != NULL);
  int cpt = 0;

  for(int itfile=0; itfile < project->numberOfFiles (); ++itfile) {
    file = isSgSourceFile(project->get_fileList()[itfile]);  
    currentFileType = "";
    if (s2s_api::isFortranFile(file->getFileName ())) {
      currentFileType = "fortran";
    } else if  (s2s_api::isCCXXFile(file->getFileName ())) {
      currentFileType = "cpp";
    }

    // Only operate on these type of files
    if (currentFileType == "fortran" || currentFileType == "c" || currentFileType == "cpp") {
      sourceFile = true;
      //Get the file name
      currentFile = file->getFileName ();
      //Modify the prefix of the output file
      outputName = outputName + file->get_sourceFileNameWithoutPath ();
      file->set_unparse_output_filename(outputName);

      //DBG_MAQAO msg
      if (DEBUG) std::cout << "--- Current file : " << currentFile << " ---" << std::endl;
      if (DEBUG) std::cout << "Number of profilers information = "<< nb_pi << std::endl; 
      
      SgGlobal * root = file->get_globalScope();
      ASTRoot ast(root);
      
      for (int i=0; i < nb_pi; i++) {
        //For each loops we will check what to do with all information we have
        Loop * loop = get_loop_from_line(&ast, pi[i].lineStart,pi[i].lineEnd, S2S_DELTA);  
        bool trans = false;
        if (!loop) {
          if (DEBUG) std::cout << "Loop not found at " << pi[i].lineStart << ":" << pi[i].lineEnd << " in " << file->getFileName () << " no transformation will be applied" << std::endl;
          continue;
        }

        if(DEBUG) {
          std::cout << "-----" << std::endl;
          std::cout << "Loop "<< pi[i].id << " line " << pi[i].lineStart << ":" << pi[i].lineEnd << std::endl;
          std::cout << "pi[i].vec_ratio=" << pi[i].vec_ratio << std::endl;
          std::cout << "pi[i].nb_ite_min=" << pi[i].nb_ite_min << std::endl;
          std::cout << "pi[i].nb_ite_max=" << pi[i].nb_ite_max << std::endl;
          std::cout << "pi[i].nb_ite_avg=" << pi[i].nb_ite_avg << std::endl;
          std::cout << "pi[i].dl1_ratio_min=" << pi[i].dl1_ratio_min << std::endl;
          std::cout << "pi[i].dl1_ratio_max=" << pi[i].dl1_ratio_max << std::endl;
          std::cout << "pi[i].dl1_ratio_mean=" << pi[i].dl1_ratio_mean << std::endl;
          std::cout << "-----" << std::endl;
        }

        // CQA
        // TODO: FIND WHEN APPLY THE SVT
        if (pi[i].vec_ratio < VEC_RATIO_MIN_TO_BV && 
          pi[i].vec_ratio != -1 &&
          (pi[i].nb_ite_min == pi[i].nb_ite_max) && (pi[i].nb_ite_max < NB_ITE_MAX_TO_BV)) {
          if(DEBUG) std::cout << "Conditions are filed to appli the SVT" << std::endl;
          
          if (!loop->hasFixBounds()) {
            if ((pi[i].nb_ite_min == pi[i].nb_ite_max) && (pi[i].nb_ite_max < NB_ITE_MAX_TO_BV)) {
              if (VERBOSE > 1 || DEBUG) std::cout << "pi[i].nb_ite_max = " << pi[i].nb_ite_max << std::endl;

              // TODO automatic specialization + short vecto 
              // Currently only work on "classical"/canonical loop : "do i=1,n"
              if (SgVarRefExp* vr = isSgVarRefExp(loop->get_bound())) {
                std::vector<variable_spe*> vec_vs_tmp;
                variable_spe* vs_tmp = new variable_spe();

                vs_tmp->specialization = variable_spe::EQUALS;
                vs_tmp->value = pi[i].nb_ite_max;
                vs_tmp->var_name = strdup(vr->get_symbol()->get_name().getString().c_str());

                std::string s_tmp = "MAQAO IF_SPE_"
                                    + std::string(vs_tmp->var_name)
                                    + "e" + s2s_api::itoa(vs_tmp->value)
                                    + "_SHORTVECT=" + s2s_api::itoa(pi[i].nb_ite_max);

                if(DEBUG) std::cout << "DIRECTIVE BEFORE SPE : " << s_tmp << std::endl;

                loop->add_directive(s_tmp);
                vec_vs_tmp.push_back(vs_tmp);

                // The specialization will check directive above the loop and apply them if corresponding, that why we create and added the directive previously 
                loop->specialize(vec_vs_tmp);
                trans = true;
              } else { // Don't know when exactly apply this
                // Currently, 4 is the only available value for the generic short vecto
                loop->shortVectoGen(4); 
                //Should be replace by
                // loop->blockVectoGen(pi[i].nb_ite_max);     
                trans = true;               
              }
            }
          } else {
            //TODO: check if bounds are smalls
            loop->shortVecto();
            trans = true;
          }
        }

        //DECAN  
        if (pi[i].dl1_ratio_mean > DL1_RATIO_MIN_TO_TILE && !trans) {
          if (VERBOSE > 1 || DEBUG) std::cout << "Average ORIG/DL1 ratio is equals to "<< pi[i].dl1_ratio_mean << std::endl;
          
          std::string directive = "MAQAO TILE";
          int tileSize = 8; //by default BUT we have to change

          if (directive.find ('=') != std::string::npos) {
            tileSize = atoi(directive.substr(directive.find ('=')+1,directive.find (' ')-1).c_str());
          }
            
          if (pi[i].nb_ite_max < tileSize && pi[i].nb_ite_max != -1) { 
            if(LOG) s2s_api::log("["+s2s_api::getEnclosingsourceFile_RosePtr(loop->get_loop_RosePtr())->get_sourceFileNameWithPath ()+" l:"+s2s_api::itoa(loop->get_line_start())+"-"+s2s_api::itoa(loop->get_line_end())+"] tile - failed - The loop will not be tile because we detected less iterations than the size of the tile.", "log_"+s2s_api::getEnclosingsourceFile_RosePtr(loop->get_loop_RosePtr())->get_sourceFileNameWithoutPath ()+".log");
            continue;
          }
          if (VERBOSE > 1 || DEBUG) std::cout << "[decan] Loop founded at " << pi[i].lineStart << ":" << pi[i].lineEnd << " in " << pi[i].file << " apply tiling" << std::endl;
          if (!loop->alreadyTransformed()) {
            if (!loop->tile(tileSize)) {
              vprof_lct (&pi[i], loop);
            }
          }
        } 

        // VPROF
        if(!trans) {
          vprof_lct (&pi[i], loop);
        }
      }

      applyOptions(numberOfOptions, options, project, root, file, pi->nb_variable_spe, pi->var_spe);

      // ASTRoot ast(root);    
      if (apply_directives)   
        s2s_api::browseFile_and_apply_directives(root);
    }
  }

  if (!sourceFile) {
    if(VERBOSE || DEBUG) std::cout << "No fortran or C/C++ file was founded. Program will stop." << std::endl;
    return 1;
  } else {
    int returnID = backend(project);

    if(returnID && VERBOSE)
    std::cout << "Error during the backend, the return status is : " << returnID << std::endl;  
    //if (VERBOSE) std::cout << "--------------------------------------------" << std::endl;

    return returnID;
  }
  return 0;
}

bool vprof_lct (profilerLoopInfo* pi, Loop* loop) {
  if (DEBUG) std::cout << "Loop count transformation " << std::endl;
  //If we didn't have any transformation to perform, so just insert LOOP COUNT transfo
  std::string directive ="";
  if (pi->nb_ite_min == -1 || pi->nb_ite_max == -1 || pi->nb_ite_avg == -1) return false;

  if (currentFileType == "cpp") {
    directive += "loop_count min="+s2s_api::itoa(pi->nb_ite_min)+" max="+s2s_api::itoa(pi->nb_ite_max)+" avg="+s2s_api::itoa(pi->nb_ite_avg);
  } else {
    directive += "LOOP COUNT MIN="+s2s_api::itoa(pi->nb_ite_min)+" MAX="+s2s_api::itoa(pi->nb_ite_max)+" AVG="+s2s_api::itoa(pi->nb_ite_avg);
  }
  if (pi->lineStart == pi->lineEnd && currentFileType == "fortran") {
    if (VERBOSE || DEBUG) std::cout << "Beware of the loop line "<< pi->lineStart << " is not a casual loop and the loop count directive doesn't work on this kind of loop" << std::endl;
    return false;
  }
  if (DEBUG) std::cout << "Loop founded at " << pi->lineStart << ":" << pi->lineEnd << " in " << currentFile << std::endl;

  if (clean_version == true && pi->nb_ite_min == pi->nb_ite_max && pi->nb_ite_max < 10) {
    // Do something
    loop->clean(pi->nb_ite_max);
  } else /*if(clean_version == false)*/ {
    loop->add_directive(directive);
  }

  return true;
}

// Initialze the project and every variables which have to be initialized
SgProject* openProject ( char* input, char* outputprefix, int numberOfOptions, char** options) {
  if (DEBUG) DBG_MAQAO
  std::vector<std::string> argv;
  std::string outputName;

  //----------------------------------------------
  //           TODO : add options
  //----------------------------------------------
  if(options != NULL) {
    if(options[0] == "verbose") {
      argv.push_back("-rose:verbose");
      argv.push_back("1");
    }
  }

  //Handle options which are not for transformations.
  for(int itopt=0; itopt < numberOfOptions; itopt++) {
    std::string opt = std::string(options[itopt]);
    if (opt.find("debug") != std::string::npos) {
      DEBUG = 1;
      VERBOSE = 1;
      //DEBUG = s2s_api::atoi(opt[opt.find("debug")+5]);
    } else if (opt.find("log") != std::string::npos) {
      LOG = 1;
    } else if (opt.find("verbose") != std::string::npos) {
      VERBOSE = 1;
    } else if (opt.find("nolog") != std::string::npos) {
      LOG = 0;
    } else if (opt.find("clean") != std::string::npos) {
      clean_version = true;
    }
  }

  std::string str(input);    // Input files
  std::string buf;           // Have a buffer string
  std::stringstream ss(str); // Insert the string into a stream

  while (ss >> buf)
    argv.push_back(buf);

  // LOAD prefix 
  if(outputprefix != " ") {
    std::string out = "-outputprefix="+std::string(outputprefix);
    argv.push_back(out);
  }

  // LOAD all include directories
  for(int itopt=0; itopt < numberOfOptions; itopt++) {
    std::string opt = std::string(options[itopt]);
    if (opt.find ("-I=") != std::string::npos) {
      std::string include = "-I";
      for(int j = 3; j < opt.size(); ++j ) {
        
        if(opt[j] == ',') {
          argv.push_back(include);
          include = "-I";
        }
        include += opt[j];
      }
      argv.push_back(include);
    }
  }

  // LOAD all MACRO
  for(int itm=0; itm < numberOfOptions; itm++) {
    std::string opt = std::string(options[itm]);
    if (opt.find ("-D=") != std::string::npos) {
      // std::cout <<"option["<<itm<<"]="<< std::string(options[itm]) << std::endl;
      std::string macro = "-D";
      for(int j = 3; j < opt.size(); ++j ) {
        
        if(opt[j] == ',') {
          argv.push_back(macro);
          macro = "-D";
        }
        macro += opt[j];
      }
      argv.push_back(macro);
      // std::cout << macro << std::endl;
    }
  }

  //----------------------------------------------
  //           Start operating on AST
  //----------------------------------------------
  return load_project(argv);
}

void applyOptions(int numberOfOptions, char** options, SgProject* project, SgGlobal* root, SgSourceFile* file, int nb_variable_spe, variable_spe* var_spe) {
  if(DEBUG) DBG_MAQAO
  if(DEBUG) std::cout << "number of options = " << numberOfOptions << std::endl;

  // CHECK all options
  for(int itopt=0; itopt < numberOfOptions; itopt++) {

    std::string opt = std::string(options[itopt]);
    if (DEBUG) std::cout << "opt["<<itopt<<"]=" <<opt << std::endl;
    // Just dump some info
    if (opt.find ("dump-loop-info") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      ASTRoot ast(root);
      if (VERBOSE) std::cout << "-------        DUMP LOOPS INFO        -------"<<std::endl;
      if (!ast.empty())
        ast.print();
      else
        if (VERBOSE || DEBUG) std::cout << "EMPTY" << std::endl;

      apply_directives = false;
      break;
    }
    //To add calls to the vprof library and have values of variable to specialization
    else if(opt.find ("vprofcalltrans") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      if (VERBOSE) std::cout << "--- LOOKING FOR ALL FUNCTIONS DECLARATIONS ---"<<std::endl;
            
      //First we will analyze variables then check all functions then addvprofcall to all functions and main if exist          
      SgFunctionDeclaration * mainDecl = SageInterface::findMain (root);
      if(mainDecl) {
        s2s_api::add_init_call (mainDecl->get_definition(), 1/*nb threads*/, 0/*model number*/);
      } else {
        s2s_api::analyze_variables(root);
        s2s_api::addvprofCall(root);  
      }
      break;
    }
    // Generate the .dot file representing the graph of the project
    else if(opt.find ("generateDOT") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      generateDOT (* project);
      apply_directives = false;
      if (VERBOSE) std::cout << ".dot file was generated, please use dot -Tps <filename>.dot -o <newfile>.<extension> " << std::endl;
    }
    // Generate a pdf with all information of each node
    else if(opt.find ("generatePDF") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      generatePDF (* project);
      apply_directives = false;
    }
    //Print all class_name of each stmt, can be use to put a name of a unknown statement
    else if(opt.find ("printStmts") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      s2s_api::printAllStmt(root);
      apply_directives = false;
    }
    // First step of specialization by searching what to specialize
    else if(opt.find ("autothune_p1") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      //Generate the AST of loops
      ASTRoot ast(root);
      std::string varListstr = "";
      std::vector<SgVariableSymbol*> varList;
      std::vector<ASTLoops*> astloops = ast.get_internalLoopsList();
      int level_depth = 2; // from the innermost and 1 = innermost

      //First we will analyze variables then check all functions then addvprofcall to all functions and main if exist          
      SgFunctionDeclaration * mainDecl = SageInterface::findMain (root);
      if(mainDecl) {
        s2s_api::add_init_call (mainDecl->get_definition(), 1/*nb threads*/, 2/*model number*/);
      } else {
        for (int i=0; i < astloops.size(); i++) {
          // This function mark handled "if" statement at the same time that it analyze what to specialize
          astloops[i]->whatSpecialize(varList, level_depth);
          //Create the list of variables to analyze
          for (int j=0; j < varList.size(); ++j) {
            varListstr += varList[j]->get_name().getString();
            if (j < varList.size()-1)
              varListstr += ",";
          }
                
          //Create a unique name for the mark
          std::string uniqueName = /*Rose::*/StringUtility::numberToString(astloops[i]->get_loop()->get_bound());
          // Ajout des appels pour compter
          s2s_api::add_lib_call(astloops[i]->get_loop()->get_loop_RosePtr(),varListstr, uniqueName);
        }
      }

      // unparse a version with the static analysis (call to the vprof lib)
      std::string  old_outputname = file->get_unparse_output_filename();
      file->set_unparse_output_filename (std::string("maqao_analyze_"+old_outputname.substr(6)));
      backend(project);

      // reset name and remove call to the vprof lib
      file->set_unparse_output_filename (old_outputname);

      for (int i=0; i < astloops.size(); i++) {
        // Remove vprof lib calls
        s2s_api::remove_lib_call(astloops[i]->get_loop()->get_loop_RosePtr());
        s2s_api::remove_lib_call(s2s_api::get_function_RosePtr(astloops[i]->get_loop()->get_loop_RosePtr()));
        // Marking handle loops with a unique name 
        std::string uniqueName = /*Rose::*/StringUtility::numberToString(astloops[i]->get_loop()->get_bound());
        std::string loopmarking = "MAQAO ANALYZE LOOP-" + s2s_api::itoa(i) + " \"" + uniqueName + "\" --var:" + varListstr;
        s2s_api::add_directive(astloops[i]->get_loop()->get_loop_RosePtr(), loopmarking, false);
      }
    }
    // Use information from results of vprof lib to specialize
    else if(opt.find ("autothune_p3") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      if (nb_variable_spe <= 0) {
        s2s_api::printdebug("autothunep3 cannot specialize because no results were found");
        break;
      }
      s2s_api::use_lib_results(root, nb_variable_spe, var_spe);
    // Where we test some stuff before to put it in production
    }
    //  Add tags to identify loops
    else if (opt.find("add_tags") != std::string::npos){
      if (DEBUG) DBG_MAQAO
      //Generate the AST of loops
      ASTRoot ast(root);
      std::vector<ASTLoops*> astloops = ast.get_internalLoopsList();
      int cpt = false;
      addTags(astloops, &cpt);
    }
    // Remove previously added tags
    else if (opt.find("remove_tags") != std::string::npos){
      if (DEBUG) DBG_MAQAO
      ASTRoot ast(root);
      std::vector<ASTLoops*> astloops = ast.get_internalLoopsList();
      std::string directive = "MAQAO LABEL LOOP ";
      removeTags(astloops, directive);
      
      apply_directives = false;
    }
    //Print all class_name of each stmt, can be use to put a name of a unknown statement
    else if(opt.find ("apply-directives") != std::string::npos) {
      if (DEBUG) DBG_MAQAO
      apply_directives = true;
    } 
    else if (opt.find ("test") != std::string::npos) {
      if (DEBUG) {std::cout << "We are in TEST  : "; DBG_MAQAO}
    }
  }
}

// Add labels to all inner loops
int addTags(std::vector<ASTLoops*> astloops, int* cpt){
  if (DEBUG) DBG_MAQAO
  if (astloops.size() == 0) {
    *cpt = true;
    return true;
  }
  else {
    std::vector<ASTLoops*> astinternloops;
    for (int i=0; i < astloops.size(); i++) {
      astinternloops = astloops[i]->get_internalLoopsList();
      addTags(astinternloops, cpt);
      if (*cpt == true) {
       std::string uniqueName = /*Rose::*/StringUtility::numberToString(astloops[i]->get_loop()->get_bound());
       std::string loopmarking = "MAQAO LABEL LOOP " + uniqueName ;
       s2s_api::add_directive(astloops[i]->get_loop()->get_loop_RosePtr(), loopmarking, true);
      }
      *cpt = false;
    }
  }
  return true;
}

int removeTags(std::vector<ASTLoops*> astloops, std::string directive){
  if (DEBUG) DBG_MAQAO
  if (astloops.size() == 0) {
    return true;
  } else {
    std::vector<ASTLoops*> astinternloops;
    for (int i=0; i < astloops.size(); i++) {
      astinternloops = astloops[i]->get_internalLoopsList();
      removeTags(astinternloops, directive);
      s2s_api::remove_MAQAO_directives(astloops[i]->get_loop()->get_loop_RosePtr(), directive);
    }
  }
  return true;
}