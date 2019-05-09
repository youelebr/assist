module ("s2s.help", package.seeall)

function s2s:init_help() 
   	local help = Help:new();
   	help:set_name ("ASSIST");
   	help:set_usage ("maqao s2s [...]")
   	help:set_description ("ASSIST (Automatique Source-to-Source assISTant)\n".. 
   						  "Transform a file automaticaly according to directives or MAQAO profilers results, before to return a source file.\n\n"..
   						  "Please read the documentation, specially on how to use ASSIST and the limitation sections before to use ASSIST.")
    

	help:add_separator ("How to launch ASSIST")
	help:add_option ("source", "src", "<path_to_file/file>", false, 
                    "Select a source file to transform.")
  
  -- DEPRECATED
  -- This option sould not be used. It is preferable to use src or oneview
	--help:add_option ("binary", "bin", "<path_to_bin/bin>", false, 
  --                  "Select a binary to analyze; we will search files used to compile this binary.")
	
	-- help:add_option ("vprof_xp", nil, "<path to VPROF csv>", false, 
  --                  "Select a VPORF csv to launch automatic transformations associated.")
	
  -- help:add_option ("lprof_xp", nil, "<path to LPROF csv>", false, 
  --                  "Select a LPROF csv to launch automatic transformations associated.")
	
	help:add_option ("Include", "I", "<path to include folder>,<path to other include folder>,...", false, 
                    "To add all includes path needed for the file; separates them by a \",\" no space. All sub-folders will also be included.")
	
	help:add_option ("Exclude", "E", "<path to exclude file>,<path to other exclude file>,...", false, 
                    "To add all excludes files you don't want to analyze and transform; separates them by a \",\" no space.")
  
  help:add_option ("D", "", "<path to exclude file>,<path to other exclude file>,...", false, 
                    "To add all macro needed to compile the file, \n"..
                    "if a statement is in a #ifdef MACRO and a #endif and the macro was not defined, \n"..
                    "the statement will not be handle.")
  	
  -- help:add_option ("outputprefix", "op", "<prefix for the output file>", false, 
  --                   "By default the prefix is \"maqao_\", but it is possible to change for another.")

  help:add_option ( "generateRMOD=<folder>", "", nil, false,
                 "Recursive function which browses all sub-folders and generates all RMOD needed by Rose to work on Fortran files. \n")

  help:add_option ( "removeAllRMOD=<foler>", "", nil, false,
                 "Recursive function which browses all sub-folders and removes all RMOD files. \n")

	help:add_option ( "option=<opt>", "", nil, false,
                 "Available values are: \n"..
                 --"<.br>  - create-mod : extract a module from a file and create a file which represent the module : <modulename>.rmod\n"..
                 -- "<.br>  - generateRMOD : Extract all modules from a file and create a file which represent the module : \n                   <modulename>.rmod\n"..
                 -- "    It also try to extract all modules from all files needed by the source file in parameter\n"..
                 "<.br>  - generatePDF : Generate a PDF file which represent all nodes. \n"..
                 "<.br>  - generateDOT : Generate a DOT file which represent all nodes in a graph form. \n"..
                 --"<.br>  - test-all-files : Print loops information from all files of a binary (only usable with the \"-bin=\" option \n"..
                 --"<.br>  - dump-loop-info : Dump information about loops of a file.\n"..
                 --"<.br>  - Vprof-directive : To Use with the option vprof_xp to search values to apply on which lines and add the right directive at the right line.\n"..
                 "<.br>  - apply-directives : Search all MAQAO directives in files and apply transformations associated.\n"..
                 "<.br>  - vprofcalltrans : Transform which add call to the vprof library in any function to trace variable and know if it's good thing to specialize or not.\n"..
                 "<.br>  - printStmts : Debug function which print all kind of statement are into the source file.\n"
   )

	help:add_separator ("Directives")
    help:add_option ("On functions :\n", "", nil, false, 
                    "<.br>  - DEADCODE\n"..
                    "<.br>     Deletes all statements detected as never execute.\n"..
                    "\n"..
                    "<.br>  - SPECIALIZE(var<op>value [, ...])\n"..
                    "<.br>     Specializes a function by copying the function, propagates constants and deletes dead code.\n"..
					          "<.br>     A call of this new function will be added at the begining of the original function, right after declarations.\n"..
                    "<.br>     If not all conditions match, the original body will be execute.\n"..
                 	"<.br>    * var : The name of the variable\n"..                    
                 	"<.br>    * <op> : different operator can be used : \"=\", \"<\", \">\", \"{}\", \"={}\"\n"..
                 	"                \"=\" : indicates than the variable is a constant.  \n"..
                 	"                \"<\" : indicates than the variable is always lesser than a constant.  \n"..
                 	"                \">\" : indicates than the variable is always greater than a constant.  \n"..
                 	"                \"{x,y}\",\"={x,y}\"  indicates that the variable is always between two constants x and y.(two integers).\n"..
                 	"<.br>    * value : The value associated to the variable\n"..
                 	"<.br>    For example : !DIR$ MAQAO SPECIALIZE(x=4, y={4,10})\n"..
                 	"<.br>                   Creates a copy of the function, \n"..  
                 	"<.br>                   in this copy: x will value 4 and y will not be less than 4 and greater than 10.\n".. 
                 	"<.br>                   A new function will be created which takes care of these information \n"..
                 	"                   to remove any dead code that these information involve.\n"       
                    )
	help:add_option ("On loops :\n", nil, nil, false, 
					 "<.br>  - UNROLL=val :"..
					 "<.br> Unroll a loop \"val\" times. \n"..
					 "<.br>  - FULLUNROLL[=val] :"..
					 "<.br> Unroll a loop \"val\" times and replace the loop by its unrolled body \n"..
           "<.br>  - INTERCHANGE=depth :"..
           "<.br> Interchange a loop with an other in the nest. \n"..
           "<.br>  - TILE=val :"..
           "<.br> Tile a perfectly nested loop nest using \"val\" as tile size. \n"..
           "<.br>  - TILE_INNER=val :"..
           "<.br> Tile the inner most loop of a nest using \"val\" as tile size. \n"..
           "<.br>  - STRIP_MINIG=val :"..
           "<.br> Tile only the innermost loop using \"val\" as tile size. \n"..
					 "<.br>  - SHORTVEC[%<val>] :"..
					 "<.br> Apply the short vectorization transformation on the loop. \n"..
           "<.br>  - GENSHORTVEC[%<val>] :"..
           "<.br> Apply the generic short vectorization transformation on the loop. \n"..
           "<.br>                    To use when loop bounds are not fixed or too high for SVT. It creates multiple versions and adds guards with MODULO comparisons. \n"
					)


	help:add_separator ("Limits")
	help:add_option ("rmod files for Fortran:\n", nil, nil, false, 
		             "A fortran files use statements as \"use\" and \"include\",\nto handle information in modules you have to create, beforehand, a rmodfile with the command :\n"..
		             "     $maqao s2s -option=\"generateRMOD\" -src=<file use or included>\n"..
                 "  or $maqao s2s -generateAllRMOD=<path/to/folder/> \n"..
                 "It search all modules that the source file needs and create an associated .rmod file.")
  help:add_option ("Fortran2008:\n", nil, nil, false, 
                 "Fortran 2008 features are not handle yet, so please put guard #if FORTRAN2008 <body> #else <same body without f2008 features> #endif to be handle by the flag -f2008 with ASSIST")
	help:add_option ("C/C++ directives for Fortran:\n", nil, nil, false, 
		             "Rose does not preprocess files and can not be abled to handle some directive such as #define. \n"..
		             "These statements are considered as comments. \n"..
		             "That means that these statements will appear in the output file but will do not be handle.")
   	--Utils:load_common_help_options (help)
   	return help;
end
