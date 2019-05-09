#ifndef S2SPROJECT
#define S2SPROJECT

#include "rose.h"
#include "s2sDefines.hpp"
#include <sstream>
#include <string>
#include <iostream>
#include <vector>

class Project;
/**
 * Currently not used
 */
class Project {
  protected:
    SgProject * Project
    std::vector<File*> files;

  public:
    Project(SgProject* p);
    int numberOfFiles();
    std::vector<File*> get_fileList();
    
  protected:

};

#endif
