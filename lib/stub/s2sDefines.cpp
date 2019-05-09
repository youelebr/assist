#include "s2sDefines.hpp"

SgFunctionDefinition* vprofLibCallformain = NULL;
int DEBUG   = 0;
int LOG     = 1;
int VERBOSE = 1;
bool clean_version = false;
bool apply_directives = true;
std::string currentFile = "";
std::string currentFileType= "";

void pretty_print(cqa_struct cs) {
	std::cout << "CQA : " << std::endl;
	std::cout << "[cpp] loop_id "	<< cs.id 			<< std::endl;;
	std::cout << "[cpp] file " 		    << cs.file 		<< std::endl;
	// std::cout << "[cpp] path " 		    << cs.path 		<< std::endl;
	std::cout << "[cpp] lineStart "	  << cs.lineStart << std::endl;
	std::cout << "[cpp] lineEnd "			<< cs.lineEnd 	<< std::endl;
	// std::cout << "[cpp] func_name "<< cs.func_name << std::endl;
	std::cout << "[cpp] vect_ratio "	<< cs.vect_ratio<< std::endl;
	// std::cout << "[cpp] transfo "	<< cs.transfo 	<< std::endl << std::endl;
}

void pretty_print(vprof_struct vs) {
	std::cout << "VPROF : "                    << std::endl;
	std::cout << "[cpp] id " 	      << vs.id   << std::endl;
	std::cout << "[cpp] file "      << vs.file << std::endl;
	std::cout << "[cpp] line "      << vs.lineStart 
	          << " to "             << vs.lineEnd   << std::endl;
	std::cout << "[cpp] directive " << vs.directive << std::endl;
}

void pretty_print(decan_struct ds) {
	std::cout << "DECAN : "         << std::endl;
	std::cout << "loop_id "	<< ds.id 	<< std::endl;;
	std::cout << "[cpp] file " 		  << ds.file 		<< std::endl;
	// std::cout << "[cpp] path " 		  << ds.path 		<< std::endl;
	std::cout << "[cpp] lineStart "	<< ds.lineStart 	<< std::endl;
	std::cout << "[cpp] lineEnd "	  << ds.lineEnd 	<< std::endl;
	// std::cout << "[cpp] func_name "	<< ds.func_name 	<< std::endl;
	std::cout << "[cpp] dl1_ratio "	<< ds.dl1_ratio	<< std::endl;
	// std::cout << "[cpp] transfo "	<< ds.transfo 	<< std::endl << std::endl;
}
