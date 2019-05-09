module ("s2s", package.seeall)
assist_struct = {}
assist_struct.DEBUG = 0

require ("s2s.help")
require ("s2s_c")
require ("s2s.api")
require ("s2s.rmod_api")
require ("s2s.load_files")
require ("s2s.auto")
require ("s2s.cqa_compare")


function s2s:s2s_launch (args, aproject)
  local help = s2s:init_help()
  local lprof_results = {} -- Used with bin (DEPRECATED)
  local vprof_results = {} -- Used with bin (DEPRECATED)
  -- local vprof_struct = {}  
  -- local cqa_struct = {}
  -- local decan_struct = {}
  -- local cqa_results   = {}
  local myTableOfTransfo = {} -- Table containing string of directives representing what transformation need to be done.
  local options = {}          -- Table of string with all option to pass to ASSIST 
  local input = {}            -- String of the file to handle
  local loopsLines = {}       -- Bad name and bad use, Array containing [i] line start, [i+1] line end, [i+2] loop id. 
                              -- It is used to give to C++ part wher are loops to modify. 
                              -- It is used with myTableOfTransfo.
  local includeDir = {}       -- Table of path (String) of where are header and other files to include.
  local macro = {}            -- Table of macro to handle. Is like the compilers option "-D"
  local excludeFile = {}      -- Table of files (String), with their path, of which files are to exlude.
                              -- If a file  cannot be handle by Rose or ASSIST but in the list of Oneview it can be avoid while continuing to transform other files.
  local directive = " "       -- String used to create the LOOP COUNT directive from VPROF results (DEPRECATED)
  local useLPROF = false      -- Used with bin (DEPRECATED)
  local useVPROF = false      -- Used with bin (DEPRECATED)
  local useCQA   = false      -- Used with bin (DEPRECATED)
  local compileWithG = true   -- Used with bin (DEPRECATED)
  local i = 1                 -- Iterator of all loops ... Not sure it has to be define here, but meh, what ever.
  local bin = nil             -- Used to determine if we have to work from src, bin, config file or Oneview results
  local src = nil             -- Used to determine if we have to work from src, bin, config file or Oneview results
  local configFile = nil      -- Used to determine if we have to work from src, bin, config file or Oneview results
  local oneview_file = nil    -- Used to determine if we have to work from src, bin, config file or Oneview results
  local auto_config_file = nil-- For the automatic comparison (Claire's Work)
  local max_loops = math.huge
  local auto_input = {}       -- Table containing auto_config_file informations
  local compare = {}          -- Table containing informations for cqa comparator
  local ov_global_metrics = {}--
  local auto_version = 0      -- 
  local clean_auto = nil      --
  local last_version = 0      --
  local replace = nil         -- Bool to determine if the output ile has to replace the old one or to e move at the specific location

  --[[ 
      Managment of all arguments
  --]] 
  if(args.help ~= nil or args.h ~= nil) then
    help:print_help(true);
    return false
  end
  
  if  args.v == true or args.auto_version == true then
    help:print_auto_version (false)
    return false
  end
  
  if (args.s2sdebug ~= nil)   then 
    table.insert(options, "debug"); 
    assist_struct.DEBUG = tonumber(args.s2sdebug) 
  end
  if (args.log     ~= nil) then table.insert(options, "log")     end
  if (args.verbose ~= nil) then table.insert(options, "verbose") end
  if (args.clean   ~= nil) then table.insert(options, "clean")   end
  --Load eventual option put by the user
  argsopt = args.option

  -- Set the output prefix
  if args.op ~= nil then
    outputprefix = args.op
  elseif args.outputprefix ~=nil then
    outputprefix = args.outputprefix
  else 
    outputprefix = "maqao_"
  end

  -- If a binary file is used
  if args.bin ~= nil and argsopt ~= "autothune" then 
    bin = args.bin
  elseif  args.binnary ~= nil and argsopt ~= "autothune" then
    bin = args.binnary
  end

  -- If a source file is used
  if args.src ~= nil then 
    src = args.src
  elseif  args.source ~= nil then
    src = args.source
  end

  if args.config ~= nil then
    configFile = args.config
  end

  if args.oneview ~= nil then
    oneview_file = args.oneview
  end

  if args["semi-auto"] ~= nil then
  	auto_config_file = args["semi-auto"]
  	if args["max-loops"] ~= nil and tonumber(args["max-loops"]) > 0 then
  		max_loops = tonumber(args["max-loops"])
  	end
  end

  if args["ov-compare"] ~= nil then
    if args["ovdir1"] == nil or args["ovdir2"] == nil then
      Message:info("Right syntax : $maqao s2s --ov-compare --ovdir1=<OneView experiment directory of the first version of the code> --ovdir2=<OneView experiment directory of the second version of the code>")
      return false
    end
    if args["max-loops"] ~= nil and tonumber(args["max-loops"]) > 0 then
      max_loops = tonumber(args["max-loops"])
    end
    ov_dir1 = args["ovdir1"]
    ov_dir2 = args["ovdir2"]
    auto_input.src = ""
    if args["loop-time-min"] ~= nil and tonumber(args["loop-time-min"]) > 0 then
      auto_input.loop_time_min = tonumber(args["loop-time-min"])
    else  
      auto_input.loop_time_min = 1.5
    end
    
    dofile(ov_dir1.."/RESULTS/assist.lua")
    ov_global_metrics.vprev = oneview_global_metrics
    s2s:fill_table_compare_vprev(ov_dir1, compare, auto_input, max_loops, oneview_cleaning_report, false)
    dofile(ov_dir2.."/RESULTS/assist.lua")
    ov_global_metrics.vnext = oneview_global_metrics
    s2s:fill_table_compare_vnext (ov_dir2, compare, auto_input, oneview_cleaning_report, false)
    local output_file = nil
    if args.output ~= nil then
      output_file = args.output
    elseif args.o ~= nil then
      output_file = args.o
    end
    s2s:compare_cqa_metrics(compare, output_file, ov_global_metrics)
  end

  if args["clean-auto"] ~= nil then
  	clean_auto = args["clean-auto"]
  	if args["clean-after"] == true then 
  		last_version = "last"
  	elseif args["clean-after"] ~= nil and tonumber(args["clean-after"]) >= 0 then
  		last_version = tonumber(args["clean-after"])
  	end
  end

  if args["create-auto-config"] ~= nil then
  	local outname = args["create-auto-config"]
  	if outname == true or outname == "" then
  		-- Use a default name
  		outname = "auto_config.lua"
  	end
  	s2s:generate_auto_config_file(outname)
  	return
  end

  if args["generate-descriptor"] ~= nil then 
    local outname = args["generate-descriptor"]
   
    if outname == true
    or outname == "" then
      -- Use a default name
      outname = "config.txt"
    end
    s2s:generate_config_file("demo_"..outname)
    s2s:generate_empty_config_file(outname)
    return
  end
  
  if args.replace ~= nil then
    replace = tostring(args.replace)
  end

  --VPROF
  if  args.vprof_xp ~= nil then
    Message:info ("-> LOAD VPROF REPORTS...")   
    useVPROF = true

    vprof_xp_csv = String:split(args.vprof_xp,",")

    for i=1, #vprof_xp_csv do
      s2s:DBG("VPROF file load : "..vprof_xp_csv[i])
      vprof_results = s2s:load_vprof_CSV_file (vprof_xp_csv[i])
    end

    s2s:sort_by_file(vprof_results)
    
    argsopt = "VPROFdirective"
  end

  --CQA
  if  args.cqa_xp ~= nil then
    Message:info ("-> LOAD CQA REPORTS...")   
    useCQA = true

    local cqa_xp_csv = args.cqa_xp
    myTableOfTransfo = s2s:load_cqa_CSV_file (cqa_xp_csv)
    argsopt = "CQATRANS"
  end

  -- Currently not used 
  if args.srcDir ~= nil then 
    srcDir = args.srcDir
  else 
    srcDir=""
  end

  ------------------------------------------------------
  -- Personal use for quick include/exclude insertion --
  ------------------------------------------------------
  --[[ 
    if args.dauvergne ~= nil then 
      Message:info("Add DAUVERGNE folder to include")
      table.insert(includeDir,"/opt/intel/impi/4.1.1.036/intel64/include/")
      table.insert(includeDir,"/opt/intel/composer_xe_2015.2.164/mkl/include/fftw/")
      table.insert(includeDir,"/home/ylebras/maqao_s2s/src/plugins/s2s/includes/")
      table.insert(includeDir,"/home/ylebras/maqao_s2s/src/plugins/s2s/includes/hdf5/")
      table.insert(includeDir,"/home/ylebras/maqao_s2s/src/plugins/s2s/includes/petsc/")
      table.insert(includeDir,"/home/ylebras/maqao_s2s/src/plugins/s2s/includes/petsc/include/")
      table.insert(includeDir,"/home/ylebras/maqao_s2s/src/plugins/s2s/includes/petsc/include/petsc/")
      table.insert(includeDir,"/home/ylebras/maqao_s2s/src/plugins/s2s/includes/petsc/include/petsc/finclude/")
      table.insert(includeDir,"/home/ylebras/maqao_s2s/src/plugins/s2s/includes/slepc/include/slepc/")
    end
    
    -- Personal use for speed include/exclude files
    if args.avbp ~= nil then 
      Message:info("Add AVBP folder to include")
      -- /!\ Do not forget the / at the end of the line
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/COMMON/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/GENERIC/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/PARSER/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/NUMERICS/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/IO/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/DBG/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/BNDY/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/CFD/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/CFM/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/CHEM/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/CHEM/NOX")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/LES/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/TTC/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/LAGRANGE/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/MAIN/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/MAIN/SLAVE")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/MAIN/COMPUTE")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/SCRIPT_MANAGER/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/PMESH/interp_tree_search/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/PMESH/interf_avbp/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/PMESH/pmeshlib/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/PMESH/generic/")
      table.insert(includeDir,"/home/ylebras/AVBP/SOURCES/PMESH/pproc/")
      -- Ignore due to Rose error : 
      -- "Insert types and variables from ISO_FORTRAN_ENV into the local scope 
      --  Sorry: ISO_FORTRAN_ENV intrinsic module not implemented yet."
      excludeFile["load_fpi_ttc_table_hdf.f90"] = 1
      excludeFile["input_param_keywords.f90"] = 1
      excludeFile["mod_inputs.f90"] = 1
      -- Error "SOURCES/TTC/CommComb_HDF.f90 line 12:0 no viable alternative at input 'use'"
      excludeFile["CommComb_HDF.f90"] = 1
    end
    -- Personal use for speed include/exclude files
    if args.avbp_v2 ~= nil then 
      Message:info("Add AVBP v2 folder to include")
      -- /!\ Do not forget the / at the end of the line
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/COMMON/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/GENERIC/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/PARSER/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/NUMERICS/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/IO/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/DBG/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/BNDY/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/CFD/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/CFM/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/CHEM/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/CHEM/NOX")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/LES/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/TTC/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/LAGRANGE/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/MAIN/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/MAIN/SLAVE")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/MAIN/COMPUTE")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/SCRIPT_MANAGER/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/PMESH/interp_tree_search/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/PMESH/interf_avbp/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/PMESH/pmeshlib/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/PMESH/generic/")
      table.insert(includeDir,"/home/ylebras/AVBP_v2/SOURCES_D7_0_2_commit_17008/PMESH/pproc/")
      -- Ignore due to Rose error : 
      -- "Insert types and variables from ISO_FORTRAN_ENV into the local scope 
      --  Sorry: ISO_FORTRAN_ENV intrinsic module not implemented yet."
      -- excludeFile["load_fpi_ttc_table_hdf.f90"] = 1
      -- excludeFile["input_param_keywords.f90"] = 1
      -- excludeFile["mod_inputs.f90"] = 1
      excludeFile["postproc_NS.f90"] = 1
      excludeFile["mod_adj_graph.f90"] = 1
      -- Error "SOURCES/TTC/CommComb_HDF.f90 line 12:0 no viable alternative at input 'use'"
      excludeFile["CommComb_HDF.f90"] = 1
    end

    -- Personal use for speed include/exclude files
    if args.yales2 ~= nil then 
      Message:info("Add YALES2 folder to include")
      -- /!\ Do not forget the / at the end of the line
      table.insert(includeDir,"/home/ylebras/YALES2/yales2/src/rmods/")
      --table.insert(includeDir,"/home/ylebras/YALES2/yales2/src/main_decan_v2/")
      excludeFile["dynamic_mode_decomposition_m.f90"] = 1 --Petsc no viable alternative at input 'PetscMPIInt'
      excludeFile["grid_partitioning_m.f90"] = 1 -- 2 function definded #if <> subrouttine #else other subroutine #endif <routine body>
      excludeFile["petsc_m.f90"] = 1 -- Because petsc
      excludeFile["scalar_operators_m.f90"] = 1 -- It seams a variable was a problem
      excludeFile["scalar_source_terms_m.f90.new"] = 1
      excludeFile["3D_cylinder"] = 1
    end

    -- Personal use for speed include/exclude files
    if args.abinit ~= nil then 
      Message:info("Add ABINIT folder to include")
      -- /!\ Do not forget the "/" at the end of the line
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/mods/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/10_defs/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/10_dumpinfo/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/11_memory_mpi/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/12_hide_mpi/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/14_hidewrite/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/16_hideleave/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/incs/")
      table.insert(includeDir,"/home/ylebras/ABINIT/abinit-7.10.5/src/65_nonlocal/")
    end
  ]]--

  --To add include files
  if args.I ~= nil or args.Include ~= nil then 

    if args.I ~= nil then includeDir_user = String:split(args.I,",")
    else includeDir_user = String:split(args.args.Include,",")
    end

    for i=1, #includeDir_user do
      local fileType = tostring(lfs.attributes(includeDir_user[i],"mode"))
      if fileType == "directory" then
        s2s:includeDirectory( options, includeDir_user[i])
      else
        Message:warn("It's weird, "..includeDir_user[i].." seams not to be a directory ...")
        table.insert(includeDir,includeDir_user[i])
      end
    end

  end
  for i=1, #includeDir do
    table.insert(options,"-I="..includeDir[i])
  end
  
  --To exclude files (can be usefull when the binary is use)  
  if args.E ~= nil or args.Exclude ~= nil then 

    if args.E ~= nil then excludeFile_user = String:split(args.E,",")
    else excludeFile_user = String:split(args.Exclude,",")
    end

    for i=1, #excludeFile_user do
      excludeFile[excludeFile_user[i]] = 1
    end
  end

  --To add Macro to the command line
  if args.D ~= nil then 

    if args.D ~= nil then 
      macro_user = String:split(args.D,",")
    end

    for i=1, #macro_user do
      table.insert(macro,macro_user[i])
    end

    for i=1, #macro do
      table.insert(options,"-D="..macro[i])
    end
  end

  --Generation of all RMOD files of a folder (and subfolders)
  if args.generateAllRMOD ~= nil then 
    s2s:extract_all_mod(args.generateAllRMOD)
    --Did I have to stop after ?!
    return
  end

  --Remove all RMOD files of a folder (and subfolders)
  if args.removeAllRMOD ~= nil then 
    s2s:clean_all_rmod(args.removeAllRMOD)
    return
  end
  
  --[[
    "dump-loop-info"
    "apply-directives"
    "search-loop-from-bin"
    "generateDOT"
    "generatePDF"
    "printStmts"
    "vprofcalltrans"
  ]]--
  table.insert(options, argsopt)

  i = 1
  local fileList={}
  
  -- If automatic comparision is used
  -- This part 
  if auto_config_file ~= nil then -- --semi-auto=<path_to_auto_config_file> File generated by --generate-auto-config-file 
    auto_input.xp_ov_path = nil

    -- Set the name of the experiment directory where the reports created by Oneview will be located
    if args["xp-dir"] ~= nil then
      auto_input.auto_input.xp_ov_path = args["xp-dir"]
      if not fs.exists(auto_input.xp_ov_path) then
        Message:error(auto_input.xp_ov_path.." does not exists\n")
        return false
      end
    else
      auto_input.xp_ov_path = io.popen("pwd"):read("*l")
    end
    
    Message:info("LOAD CONFIGURATION FILE")
    if s2s:load_auto_config_file(auto_config_file, auto_input) == false then
      return false
    end
    auto_input.xp_ov_path = auto_input.xp_ov_path.."/maqao_s2s_ov_results_"..string.gsub(auto_input.src, ".*/", "").."/"
    auto_version = s2s:find_last_version(auto_input.src)
    if auto_version > 0 then
      os.execute("rm -rf "..auto_input.xp_ov_path.."/exp_ov_on_V"..auto_version)
    end

    Message:info("SET EXPERIMENT ENVIRONMENT")
    if s2s:set_s2s_auto_env(auto_input, auto_version) == false then
      if auto_version == 0 then
        s2s:clean_auto_env(auto_input.src)
      else
        os.execute("rm -r "..auto_input.src.."_V"..(auto_version+1))
      end

      return false
    end
    
    -- execute Oneview
    if s2s:run_oneview(auto_input, auto_version) == false then
      if auto_version == 0 then
        s2s:clean_auto_env(auto_input.src)
      else
        os.execute("rm -r "..auto_input.src.."_V"..(auto_version+1))
      end
      return false
    end
    oneview_file = auto_input.xp_ov_path.."exp_ov_on_V"..auto_version.."/RESULTS/assist.lua"
    if fs.exists(oneview_file) == false then
      Message:error("[set_loop_info] le fichier existe pas : "..oneview_file)
      return false
    end
    dofile(oneview_file)
    ov_global_metrics.vprev = oneview_global_metrics
    if s2s:fill_table_compare_vprev(auto_input.xp_ov_path.."/exp_ov_on_V"..auto_version, compare, auto_input, max_loops, oneview_cleaning_report, true) == false then
        return false
    end
    if s2s:add_tags(auto_input, compare) == false then
      return false
    end

    -- set options to execute the transformations on source files
    if argsopt == "apply-directives" then
      oneview_file = nil
      local pwd = io.popen("pwd"):read("*l").."/"
      relative_src_path = string.gsub(auto_input.src, pwd, "")
      src = string.gsub(src, pwd, "")
      src = string.gsub(src, relative_src_path, "")
      src = auto_input.src.."_V"..auto_version.."/"..src
      print(src)
    end
    replace = auto_input.src.."_V"..(auto_version+1)
  end

  --*********************--
  --    Launch Assist    --
  --  by different ways  --
  --*********************--
  -- (DEPRECATED)
  -- I wrote it(bin) when I started and it should work, 
  -- but to be honest, it's not really well wrote... not at all...
  -- Advice : Only use Oneview, config or src. I say that, it's for you ...
  if bin ~= nil then -- To launch with --vprof_xp= & --bin=
    s2s:DBG ("-> SEARCH LOOPS IN BINARY...") 
    local asmf = aproject:load (bin, aproject:get_uarch_name())
    for func in asmf:functions() do   
      -- iterate over function loops and list ids
      for loop in func:loops () do     
          -- iterate over loops return by Vprof
          if useVPROF then
            for k=1,table.getn(vprof_results) do
              if loop:get_id() == vprof_results[k].loop_id then
                --Message:info("USE VPROF")
                src_start,src_end = loop:get_src_lines()
                --local tmp = String:split (lprof_results[j].source_info, "@")
                myTableOfTransfo[i] = {
                  loop_id = loop:get_id(),
                  lineStart = src_start,
                  lineEnd = src_end,
                  file = loop:get_first_entry():get_first_insn():get_src_file_path(),
                  ite_min = vprof_results[k].ite_min,
                  ite_max = vprof_results[k].ite_max,
                  ite_avg = s2s:round(vprof_results[k].ite_avg)
                }
                i = i + 1
              end -- end if
            end -- end for
          -- iterate over loops return by LPROF
          elseif useLPROF then
            --Message:info("USE LPROF")
            for k=1,table.getn(lprof_results) do
              if loop:get_id() == lprof_results[k].loop_id then
                for ii=1, table.getn(lprof_results) do 
                  if lprof_results[ii].source_info ~= nil then 
                    filefound = String:split (lprof_results[ii].source_info, "@")
                    --Message:info("filefounded : "..filefound[2])
                  end -- end if 
                end -- end for 
              end -- end if
            end -- end for
          -- iterate over loops return by MAQAO
          elseif useCQA then
            --do nothing, all is done in the csv file
          else
            if(loop:get_first_entry():get_first_insn():get_src_file_path() ~= nil) then
              var = loop:get_first_entry():get_first_insn():get_src_file_path()

              if (Table.contains(fileList,var) == false) then 
                table.insert(fileList,loop:get_first_entry():get_first_insn():get_src_file_path())
              end
        
              src_start,src_end = loop:get_src_lines()
              astart, astop = loop:get_asm_addresses()
              if (src_start ~= -1 or src_end ~=-1) and src_start ~= src_end then
                  myTableOfTransfo[i] = {
                    loop_id = loop:get_id(),
                    lineStart = src_start,
                    lineEnd = src_end,
                    file = loop:get_first_entry():get_first_insn():get_src_file_path(),
                    --addressStart = astart,
                    --addressStop = astop
                  }
                i = i + 1
              else 
                compileWithG = false
              end
              --end
            end -- end if
          end -- end else
      end --end for loop
    end -- end for func

    if compileWithG == false and useVPROF == nil and useLPROF == nil then
      Message:warn("Found no debug data for all loops.\n With GNU or Intel compilers, please recompile with -g to analyze all loops.")
    end
     
    --Message:info ("-> LOOPS TO ANALYZE") 
    i=1
    j=1
    nbOfDirectives = 0
    listOfFiles=myTableOfTransfo[j].file
    input = myTableOfTransfo[j].file
    table.insert(loopsLines,myTableOfTransfo[j].lineStart)
    table.insert(loopsLines,myTableOfTransfo[j].lineEnd)
    table.insert(loopsLines,myTableOfTransfo[j].loop_id)
    --Message:info("Insert : start "..myTableOfTransfo[j].lineStart.." end "..myTableOfTransfo[j].lineEnd.. " id "..myTableOfTransfo[j].loop_id)

    if useVPROF then
      directive = "VPROFdirective={"..myTableOfTransfo[j].lineStart..","..myTableOfTransfo[j].lineEnd..",LOOP COUNT MAX="..myTableOfTransfo[j].ite_max..", MIN="..myTableOfTransfo[j].ite_min..", AVG="..myTableOfTransfo[j].ite_avg.."}"
      --Message:info("ADD directive for the file "..myTableOfTransfo[j].file.. " : "..myTableOfTransfo[j].lineStart..","..myTableOfTransfo[j].lineEnd.. " : "..directive)
      table.insert(options, directive)
      nbOfDirectives = nbOfDirectives + 1
    end

    -- If a file is in the Exclude list, remove it.
    for j=j, table.getn(myTableOfTransfo) do
      --if the file is the same as previously add loop to analyze
      tmp = myTableOfTransfo[j].file:match("^.+(%..+)$")
      --Message:info("file without path : " ..fs.basename(myTableOfTransfo[j].file))
      if excludeFile[fs.basename(myTableOfTransfo[j].file)] ~= nil then 
        Message:info("File unanalyze : "..myTableOfTransfo[j].file)
        if useVPROF then
          for t=0, nbOfDirectives do
            table.remove(options, table.getn(options))
          end
          nbOfDirectives = 0
        end

      end

      -- Test, we only analyze source codes (Fortran/C/C++)
      -- And the file is not in the Exclude file list
      if  (tmp == ".f90" or tmp == ".F90" or tmp == ".f" or 
           tmp == ".C" or tmp == ".cpp" or tmp == ".cxx" or tmp == ".CC" 
           or tmp == ".c") 
           and excludeFile[fs.basename(myTableOfTransfo[j].file)] == nil
      then 
        if input == myTableOfTransfo[j].file then
          if Table.contains(loopsLines,myTableOfTransfo[j].lineStart) == false
          then

            table.insert(loopsLines,myTableOfTransfo[j].lineStart)
            table.insert(loopsLines,myTableOfTransfo[j].lineEnd)
            table.insert(loopsLines,myTableOfTransfo[j].loop_id)
            if useVPROF then

              directive = "VPROFdirective={"..myTableOfTransfo[j].lineStart..","..myTableOfTransfo[j].lineEnd..",LOOP COUNT MAX="..myTableOfTransfo[j].ite_max..", MIN="..myTableOfTransfo[j].ite_min..", AVG="..myTableOfTransfo[j].ite_avg.."}"
              --Message:info("ADD Vdirective for the file "..myTableOfTransfo[j].file.. " : "..myTableOfTransfo[j].lineStart..","..myTableOfTransfo[j].lineEnd.. " : "..directive)
              table.insert(options, directive)
              nbOfDirectives = nbOfDirectives + 1
            end
          end
        else
          -- if args.createMod ~= nil then
          --   --Message:info("create-mod")
          --   Message:info("create all module file for "..input)
          --   s2s:rmod_file (input, includeDir)
          -- end
          if (excludeFile[input] == nil) then
            listOfFiles = listOfFiles.." "..myTableOfTransfo[j].file
            input = srcDir..input

            --Message:info ("-> START TRANSFORMATIONS on "..input.." ...") 
            -- Test all files at the same time -- NOT USED YET NOT WELL TESTED
            if argsopt ~= "test-all-files" then
              if args.f2008 ~= nil then
                s2s:no_fort2008(input)
                s2s.start(input, outputprefix, -1, loopsLines, options)
                s2s:undo_no_fort2008(input)
                s2s:undo_no_fort2008("maqao_"..input)
                os.execute("mv ".."maqao_"..input.." "..input)
              else 
                s2s.start(input, outputprefix, -1, loopsLines, options)
                s2s:post_traitement_macro(input, outputprefix..fs.basename(input))
              end

              if replace ~= nil then
                file = src:match( "([^/]+)$" )

                if (replace == "true") then
                  path = string.gsub(src,file,"")
                else 
                  path = replace.."/"
                end
                if (path == nil ) then path = "./" end
                  
                os.execute("mv maqao_"..file .." "..path..file )
              end
              if useVPROF then
                for t=0, nbOfDirectives do
                  table.remove(options, table.getn(options))
                end
                nbOfDirectives = 0
              end

            end

            if useVPROF then
              directive = "VPROFdirective={"..myTableOfTransfo[j].lineStart..","..myTableOfTransfo[j].lineEnd..",LOOP COUNT MAX="..myTableOfTransfo[j].ite_max..", MIN="..myTableOfTransfo[j].ite_min..", AVG="..myTableOfTransfo[j].ite_avg.."}"
              table.insert(options, directive)
              nbOfDirectives = nbOfDirectives + 1
            end

            Message:info ("-> ... FINISH TRANSFORMATIONS on "..input)
          end

          i = i + 1
          loopsLines = {myTableOfTransfo[j].lineStart,myTableOfTransfo[j].lineEnd,myTableOfTransfo[j].loop_id}
          input = myTableOfTransfo[j].file
        end
      end -- end else tmp != ".f90"
    end --end for

    if args.createMod ~= nil then
      Message:info("create all module file for "..input)
      s2s:rmod_file (input, includeDir)
    end

    -- If a bin is used, the arg "option" is not mandatory.
    -- Either the user indicate a VPROF file or "option", by default we lanch "apply-directive" option.
    if useVPROF then
      directive = "VPROFdirective={"..myTableOfTransfo[j].lineStart..","..myTableOfTransfo[j].lineEnd..",LOOP COUNT MAX="..myTableOfTransfo[j].ite_max..", MIN="..myTableOfTransfo[j].ite_min..", AVG="..myTableOfTransfo[j].ite_avg.."}\""
      Message:info("ADD Vprof-directive for the file "..myTableOfTransfo[j].file.. " : "..myTableOfTransfo[j].lineStart..","..myTableOfTransfo[j].lineEnd.. " : "..directive)
      table.insert(options, directive)
      nbOfDirectives = nbOfDirectives + 1
    elseif argsopt == nil then
      Message:info("default case : apply-directives")
      opt = "apply-directives"
      table.insert(options, opt)
    end

    input = srcDir..input
    --Message:info ("-> START TRANSFORMATIONS on the last file : "..input.."...") 
    -- Test all file is experimental and not well tested.
    if argsopt ~= "test-all-files" then
      if args.f2008 ~= nil then
        s2s:no_fort2008(input)
        s2s.start(input, outputprefix, -1, loopsLines, options)
        s2s:undo_no_fort2008(input)
        s2s:undo_no_fort2008("maqao_"..input)
      else 
        Message:info("START TRANSFORMATION ON THE LAST FILE : "..input)
        s2s.start(input, outputprefix, -1, loopsLines, options)
        s2s:post_traitement_macro(input, outputprefix..fs.basename(input))
      end

      if replace ~= nil then
        file = src:match( "([^/]+)$" )

        if (replace == "true") then
          path = string.gsub(src,file,"")
        else 
          path = replace.."/"
        end
        if (path == nil ) then path = "./" end
            
        os.execute("mv maqao_"..file .." "..path..file )
      end
      if useVPROF then
        for t=0, nbOfDirectives do
          table.remove(options, table.getn(options))
        end
        nbOfDirectives = 0
      end
    else 
      opt = "test-all-files"
      table.insert(options, opt)
      input = listOfFiles
      if args.f2008 ~= nil then
        s2s:no_fort2008(input)
        Message:info("START TRANSFORMATION ON "..input)
        s2s.start(input, outputprefix, -1, loopsLines, options)
        s2s:undo_no_fort2008(input)
        s2s:undo_no_fort2008("maqao_"..input)
      else 
        Message:info("START TRANSFORMATION ON "..input)
        s2s.start(input, outputprefix, -1, loopsLines, options)
        s2s:post_traitement_macro(input, outputprefix)
      end
      if replace ~= nil then
        Message:warn("Sorry the replace option is not available with the option \"test-all-file\"\n")
      end
    end
    Message:info ("-> ... FINISH TRANSFORMATIONS on the last file : "..input)
  
  elseif configFile ~= nil then -- --config=<File.lua> 
    s2s:DBG("file config = "..configFile)

    if args.arch ~= nil then 
      -- input, loopTrans, funcTrans = s2s:load_config_file(configFile, args.arch) -- The first auto_version (when all directives are wrote directly in a file as they have to be in the source) /!\ Becareful -- s2s_c.c arg 11 have to be a LUA_TSTRING type
      input, loopTrans, funcTrans = s2s:load_config_file_v2(configFile, args.arch)
    else
      -- input, loopTrans, funcTrans = s2s:load_config_file(configFile, "All") -- The first auto_version ( when all directives are wrote directly in a file as they have to be in the source) /!\ Becareful -- s2s_c.c arg 11 have to be a LUA_TSTRING type
      input, loopTrans, funcTrans = s2s:load_config_file_v2(configFile, "All")  
    end

    local indexL = 0
    local indexF = 0
    -- Message:info("DBG last print test-loop")
    --Count the number of elements in the Table
    for line, trans in pairs(loopTrans) do
      indexL = indexL +1
      -- Message:info ("On the loop line "..trans[1].. " next directives will be applied : ")
      -- for j=2, #trans do 
      --   Message:info(" !DIR$ MAQAO " ..trans[j])
      -- end
    end

    -- Message:info("DBG last print test-func")
    --Count the number of elements in the Table
    for line, trans in pairs(funcTrans) do
      indexF = indexF +1 
      -- Message:info ("On the loop line "..trans[1].. " next directives will be applied : ")
      -- for j=2, #trans do 
      --   Message:info(" !DIR$ MAQAO " ..trans[j])
      -- end
    end

    s2s.start_config_file (input, outputprefix, indexL, loopTrans, indexF, funcTrans, options)

    if replace ~= nil then
      file = src:match( "([^/]+)$" )

      if (replace == "true") then
        path = string.gsub(src,file,"")
      else 
        path = replace.."/"
      end
      if (path == nil ) then path = "./" end
          
      os.execute("mv maqao_"..file .." "..path..file )
    end

  elseif oneview_file ~= nil then -- --oneview=<ov_File> Fichier génbéré par OV 
    local oneview_array = {}
    local nbOfInfo = 0
    local oneview_input = nil
    local ite = 0
    local ite_subarr = 0;
    local filename = ""
    local path = ""
    local tmpArray = {}
    local sub_oneview_array = {}
    local var_struct= {}
    oneview_input = String:split(oneview_file,",")

    for i=1, #oneview_input do
      s2s:DBG("Oneview file load : "..oneview_input[i])
      tmpArray = s2s:load_oneview_file (oneview_input[i])
      table.foreach(tmpArray, function(k,v) table.insert(oneview_array, v) end)
    end

    s2s:sort_by_file(oneview_array)

    repeat 
      repeat 
        ite = ite + 1

        input = oneview_array[ite].file
        path = s2s:getPath(input)
        filename = s2s:getFileFromPath(input)
        
        --Create a subarray to only handle one file at time and to sort and group all transformation by file
        nbOfInfo = nbOfInfo + 1
        sub_oneview_array[nbOfInfo] = {}
        sub_oneview_array[nbOfInfo].id             = oneview_array[ite].loop_id
        sub_oneview_array[nbOfInfo].file           = oneview_array[ite].file
        sub_oneview_array[nbOfInfo].lineStart      = oneview_array[ite].lineStart
        sub_oneview_array[nbOfInfo].lineStop       = oneview_array[ite].lineStop
        sub_oneview_array[nbOfInfo].label          = nil
        sub_oneview_array[nbOfInfo].vec_ratio      = oneview_array[ite].vecRatio
        sub_oneview_array[nbOfInfo].dl1_ratio_min  = oneview_array[ite].r_l1_min
        sub_oneview_array[nbOfInfo].dl1_ratio_max  = oneview_array[ite].r_l1_max
        sub_oneview_array[nbOfInfo].dl1_ratio_avg  = oneview_array[ite].r_l1_med
        sub_oneview_array[nbOfInfo].nb_ite_min     = oneview_array[ite].ite_min
        sub_oneview_array[nbOfInfo].nb_ite_max     = oneview_array[ite].ite_max
        sub_oneview_array[nbOfInfo].nb_ite_avg     = s2s:round(oneview_array[ite].ite_avg)
        
        if (ite >= table.getn(oneview_array)) then break end
      until (input ~= oneview_array[ite+1].file)
            
      if ((excludeFile[fs.basename(input)] == nil) and (excludeFile[input] == nil)) then 
        -- Pre traitement
        if args.f2008 ~= nil and input:match("^.+(%..+)$") ~= ".c" and 
          input:match("^.+(%..+)$") ~= ".C" and input:match("^.+(%..+)$") ~= ".cpp" and 
          input:match("^.+(%..+)$") ~= ".cxx" and input:match("^.+(%..+)$") ~= ".CC"
        then
          s2s:DBG("Manage a f2008 src file ",2)
          s2s:no_fort2008(input)
        end
        
        Message:info ("-> START TRANSFORMATIONS on "..filename.." ...")
        s2s.start_oneview(input, 
                          outputprefix, 
                          #sub_oneview_array, 
                          sub_oneview_array, 
                          #var_struct, 
                          var_struct,
                          options) 
        
        -- Eventual post traitement
        if args.f2008 ~= nil and input:match("^.+(%..+)$") ~= ".c" and 
          input:match("^.+(%..+)$") ~= ".C" and input:match("^.+(%..+)$") ~= ".cpp" and 
          input:match("^.+(%..+)$") ~= ".cxx" and input:match("^.+(%..+)$") ~= ".CC"
        then
          s2s:undo_no_fort2008(input)
          file = input:match( "([^/]+)$")
          s2s:undo_no_fort2008(outputprefix..file)
        elseif args.handle_macro ~= nil or args.hm ~= nil then
          s2s:DBG("Manage a src file with macros",2)
          file = input:match( "([^/]+)$")
          s2s:post_traitement_macro(input, "maqao_"..file)
          if (s2s:file_exists("maqao_analyze_"..file)) then 
            s2s:post_traitement_macro(file, "maqao_analyze_"..file)
          end
        end

        -- Move file and eventually rename it
        if replace ~= nil then
          s2s:DBG("Replace the original file ")
          file = input:match( "([^/]+)$")

          if (replace == "true") then
            path = string.gsub(input,file,"")
          else 
            path = replace.."/"
          end
          if (path == nil ) then path = "./" end
          
          --The file will be renamed as originally
          os.execute("mv "..outputprefix..file .." "..path..file )
        end

        Message:info ("-> ... FINISH TRANSFORMATIONS on "..path..outputprefix..filename)
        print ("")

      else 
        print (input.." will not be handle because it is in exclude table")
      end 

      nbOfInfo=0
      sub_oneview_array = {}
    until ite >= table.getn(oneview_array)

     Message:info ("-> ... FINISH TRANSFORMATIONS")

  elseif src ~= nil then -- -src=<source file> --option="<pt>"
    s2s:DBG("Analyze a src file ")
    local var_struct= {}
    local sub_oneview_array = {}

    -- Create all file .rmod need by Rose. Representing Fortran header that will not be analyzed
    -- It takes as input a folder, then it analyzes all fortran file in that folder
    if argsopt == "generateRMOD" then
      for i=1, #input do
        Message:info("create all needed modules for "..input[i])
        s2s:extract_mod(input[i])
      end
      return
    -- First phase of "autotuning". During this pahse ASSIST will looking for which variable to specialize. Loop bound, conditon into loop nest, etc.
    -- Second phase is the execution of the new binary which gonna collect information about these variables (values each time it is required) by using the vprof lib.
    elseif argsopt == "autothune" then --../maqao s2s -src=codelet.c,driver.c -option=autothune -bin=./convf32 -makefile=./ -Ipath=/home/cbaskevitch/Bureau/Assist_doc/tests/VPROF_FCT_v2/ -Lpath=/home/cbaskevitch/Bureau/Assist_doc/tests/VPROF_FCT_v2/build/
      if args.makefile == nil or args.bin == nil or args.Lpath == nil or args.Ipath == nil then
        Message:error("Needs :\n - path to binary with -bin=<path>\n - path to Makefile with --makefile=<path>\n - path to lib VPROF_FCT to add in LDFLAGS for the compilation with -Lpath=<path>\n - path to lib VPROF_FCT to add in CFLAGS for the compilation with -Ipath=<path>")
        return
      elseif not fs.exists(args.Lpath) then
      	Message:error(args.Lpath.." does not exist.")
      elseif not fs.exists(args.Ipath) then
      	Message:error(args.Ipath.." does not exist.")
      end
      s2s:autothune(src, excludeFile, outputprefix, loopsLines, options, replace, args, argsopt, args.Lpath, args.Ipath)
      
      return
    -- Phase 3 of the "autotune" consist by analyzing results gathered during phase 2 and create specialized version of function or loops
    elseif argsopt == "autothune_p3" then 
      local cptvarspe;
      local vprofres;

      --[[ 
        maqaoCptTmpVarSpe.cpt: is the file where ASSIST write information
        about which variable he want to analyze/specialize and how to find them after 
      --]]
      if (args.cptvarspe ~= nil) then 
        if (s2s:file_exists(args.cptvarspe)) then 
          dofile(args.cptvarspe)
        else 
          print("ERROR ".. args.cptvarspe .. " does not exist")
        end
        cptvarspe = args.cptvarspe
      elseif (s2s:file_exists("maqaoCptTmpVarSpe.cpt")) then
        cptvarspe = "maqaoCptTmpVarSpe.cpt"
      else 
        print("ERROR maqaoCptTmpVarSpe.cpt not found")
      end
      --[[ 
        vproflibres.lua: During the execution of the second step with the instrumented binary
        At the end it will print a lua table with all information needed. We have to copy/paste 
        them into a vproflibres.lua.
      --]]
      if (args.vprofres ~= nil) then 
        if (s2s:file_exists(args.vprofres)) then 
          vprofres = args.vprofres
        else 
          print("ERROR ".. args.vprofres .. " does not exist")
        end
      elseif (s2s:file_exists("vproflibres.lua")) then
        vprofres = "vproflibres.lua"
      else 
        print("ERROR vproflibres.lua not found")
      end
      var_struct = s2s:autothunep2(cptvarspe, vprofres)
    -- Test if the user inform a right option.
    elseif argsopt ~= "dump-loop-info" 
      and argsopt ~= "apply-directives" 
      and argsopt ~= "generateDOT"
      and argsopt ~= "generatePDF" 
      and argsopt ~= "printStmts" 
      and argsopt ~= "vprofcalltrans" 
      and argsopt ~= "autothune_p1" 
      and argsopt ~= "autothune_p3"
      and argsopt ~= "add_tags"
      and argsopt ~= "remove_tags"
      and argsopt ~= "test"
      and debugStmt == nil
    then
      Message:warn("wrong option, do nothing")
      Message:warn("Possible options are : \n-generateRMOD\n-generatePDF\n-generateDOT\n-apply-directives\n-vprofcalltrans")
      os.exit(1)
    end

    s2s:src_options(src, input, excludeFile, outputprefix, sub_oneview_array, var_struct, loopsLines, options, replace, args, argsopt)

  else
    s2s:DBG("Warning: Bad use of ASSIST, nothing will be done, something wrong happened ")
    if ((bin == nil and src == nil)
      or (bin ~= nil and src ~= nil))
      --and argsopt ~= "generateRMOD"
      -- and args.vprof_xp == nil 
      and configFile == nil
      and auto_config_file == nil
      and clean_auto == nil
      and args["ov-compare"] == nil
    then
        Message:warn("--- Binary or source file is missing ! ---")
        Message:warn("maqao s2s -bin=binFile [-vprof_xp=<vprofCSV>] -option=<opt>")
        Message:warn("or")
        Message:warn("maqao s2s -src=srcFile -option=<opt>")
        return false
    end
  end -- end else

  --*************************--
  --    Auto compare mode    --
  --    Finallyzing part     --
  --*************************--
  if auto_config_file ~= nil then
  	if s2s:run_oneview(auto_input, auto_version + 1) == false then
		  return false
	  end 
	  local ov_dir = auto_input.xp_ov_path.."/exp_ov_on_V"
	  oneview_file = ov_dir..(auto_version+1).."/RESULTS/assist.lua"
    if fs.exists(oneview_file) == false then
		  Message:error("[set_loop_info] le fichier existe pas : "..oneview_file)
		  return false
	  end
	  oneview_cleaning_report = nil
	  oneview_global_metrics = nil
	  dofile(oneview_file)
	  ov_global_metrics.vnext = oneview_global_metrics
	
	  if s2s:fill_table_compare_vnext(ov_dir..(auto_version+1), compare, auto_input, oneview_cleaning_report, true) == false then
		  return false
	  end
	
	  local output_file = nil
	  if args.output ~= nil then
		  output_file = args.output
	  elseif args.o ~= nil then
		  output_file = args.o
	  end
	  s2s:compare_cqa_metrics(compare, output_file, ov_global_metrics)
	  if args["let-tags"] == nil then
      s2s:remove_tags(compare, auto_input.src.."_V"..auto_version)
	   s2s:remove_tags(compare, auto_input.src.."_V"..(auto_version+1))
    end
  end

  if clean_auto == true then
  	Message:warn("Cannot clean sources versions, please use this syntax :")
  	Message:warn("maqao s2s clean_auto=<src_path without suffix _Vx> [--last-version=<last version you want to keep>")
  elseif clean_auto ~= nil then
  	clean_auto = string.gsub(clean_auto, "/$", "")
	  local nb_version = s2s:find_last_version(clean_auto) + 1
	  if last_version == "last" then
		  last_version = nb_version - 2
	  end
   	s2s:clean_auto_env(clean_auto, nb_version, last_version)
   end
end -- end function
