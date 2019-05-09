module ("s2s.api", package.seeall)

--Define INFINITY macro
local INFINITY = math.huge

--[[ Check if an element is in a table ]]--
function Table.contains (table, element)
	if table == nil then return true end
  for _, value in pairs(table) do
    if value == element then
      return true
    end
  end
  return false
end

--[[ Check if an element have a start and a end line ]]--
function Table.findEndLineFromStartLine (table, element)
  for i=1, table.getn(table) do
  	--print("i:"..i)
    if table[i].lineStart == element then
      return table[i].lineEnd
    end
  end
  return -1
end

--[[ Round a number ]]--
function s2s:round(num)
	if ((num - math.floor(num)) < 0.5) then 
  		return math.floor(num)
  	else 
  		return math.floor(num + 0.5)
  	end
end

-- Delete the last charactere of a string
function s2s:deleteLastCharacter (str, char)
  return(str:gsub(char.."$", ""))
end

-- Return file name with its extension from the name without its extension
function s2s:getfile_from_basename (basename)
  local possible_extension = {".f77",".f90",".f03",".f",".F",".F77",".F90",".F03",".cpp",".hpp",".cc",".hh",".C",".H",".CPP",".HPP",".cxx",".hxx",".c++",".h++",".c",".h",".hh",".H",".HPP",".hxx",".h++",".h"}
  for i=0, #possible_extension do
    if fs.exists (basename..possible_extension[i]) == true then 
      return basename..possible_extension[i]
    end
  end
  return nil
end

-- Adds include directories with all sub directories into the include directories table
function s2s:includeDirectory (options, directory)
  local fileType = tostring(lfs.attributes(directory,"mode"))

  if fileType == "directory" then 
    -- Si c'est un repertoire dans ce cas on le rajoute dans les option -I=/path...
    table.insert(options,"-I="..directory)

    -- et on rappel récursivement tous les sous répertoire en appellant la fnction récursivement.
    local content = fs.readdir (directory)
    for _, file in pairs(content) do
      local fileType = tostring(lfs.attributes(directory.."/"..file.name,"mode"))
      if fileType == "directory" and file.name~= "." and file.name~=".." then 
        s2s:includeDirectory(options,directory.."/"..file.name)
      end
    end
  end
end

-- Get all kind of files/folders from a directory
function s2s:getAllfiles (input, directory)
  local fileType = tostring(lfs.attributes(directory,"mode"))

  if fileType == "directory" then 
    -- et on rappel récursivement tous les sous répertoire en appellant la fonction récursivement.
    local content = fs.readdir (directory)

    for _, file in pairs(content) do
      local fileType2 = tostring(lfs.attributes(directory.."/"..file.name,"mode"))
      if fileType2 == "directory" and file.name~= "." and file.name~=".." then 
        s2s:getAllfiles(input,directory.."/"..file.name)

      else -- Si c'est un fichier on l'ajoute à la liste des fichiers à analyser
        local tmp = file.name:match("^.+(%..+)$")

        if (tmp == ".f90" or tmp == ".F90" or tmp == ".f" or 
            tmp == ".C" or tmp == ".cpp" or tmp == ".cxx" or tmp == ".CC" 
            or tmp == ".c") then 
          --print("Insert "..file.name)
          table.insert(input, directory.."/"..file.name)

        end -- if 
      end -- else
    end -- for
  end -- if 
end

-- Check if a file exist from its name (or path/filename)
function s2s:file_exists(name)
   local f=io.open(name,"r")
   if f~=nil then io.close(f) return true else return false end
end

-- Sort Oneview report by file name
function s2s:sort_by_file(oneview_cleaning_report)
  table.sort(oneview_cleaning_report, function(a,b) return a.file>b.file end)
end

-- Extract the path of a file
function s2s:getPath(str)
  --message("DEBUG - getPath")
  sep='/'
  return str:match("(.*"..sep..")")
end

--Extract the file name from a full path/filename
function s2s:getFileFromPath(str)
  --message("DEBUG - getFileFromPath")
  if (str:match("^.+/(.+)$") ~= nil) then
  return str:match("^.+/(.+)$")
  else
    return str
  end
end

-- Gather information collected during phase 2 of "autotuning"
function s2s:autothunep2 (compteur, vrofRes)
  -- Do the match between both 
  dofile(compteur)
  dofile(vrofRes)
  -- print ("Elements")
  -- Table:tostring(elements)
  -- print ("Int32")
  -- Table:tostring(Int32)
  -- print ("ifthen")
  -- Table:tostring(Ifthen)

  local final_struct = {};
  local i=1;
 --[[
  struct final_struct {
    string file
    string kind
    string label
    int    id
    string var_name;
    int    value

  };
 --]]
  -- print("What we are looking at the begining : ")
  for _,elem in pairs(elements) do
    final_struct[i] = {
      file = elem.file,
      kind = elem.kind,
      label = elem.where,
      id = elem.id,
      var_name = elem.var_name,
      value = -1
    }
    if final_struct[i].var_name == nil then 
      final_struct[i].var_name = elem.kind
    end
    i = i +1
  end

  if (Int32 ~= nil) then 
    for ite,elem in pairs(Int32) do
      for _,finalstruct_elem in ipairs (final_struct) do

        if (finalstruct_elem.id == elem.var_id and finalstruct_elem.kind == "int32") then 
          -- print(finalstruct_elem.var_name.." for the id "..elem.var_id)
          finalstruct_elem.value = s2s:get_max(elem.values)
          break
        end
      end
    end
  end
  if (Int64 ~= nil) then 
    for ite,elem in pairs(Int64) do
      for _,finalstruct_elem in ipairs (final_struct) do

        if (finalstruct_elem.id == elem.var_id and finalstruct_elem.kind == "int64") then 
          -- print(finalstruct_elem.var_name.." for the id "..elem.var_id)
          finalstruct_elem.value = s2s:get_max(elem.values)
          break
        end
      end
    end
  end
  if (Float ~= nil) then 
    for ite,elem in pairs(Float) do
      for _,finalstruct_elem in ipairs (final_struct) do

        if (finalstruct_elem.id == elem.var_id and finalstruct_elem.kind == "float") then 
          -- print(finalstruct_elem.var_name.." for the id "..elem.var_id)
          finalstruct_elem.value = s2s:get_max(elem.values)
          break
        end
      end
    end
  end
  if (Double ~= nil) then 
    for ite,elem in pairs(Double) do
      for _,finalstruct_elem in ipairs (final_struct) do

        if (finalstruct_elem.id == elem.var_id and finalstruct_elem.kind == "double") then 
          -- print(finalstruct_elem.var_name.." for the id "..elem.var_id)
          finalstruct_elem.value = s2s:get_max(elem.values)
          break
        end
      end
    end
  end
  if (Ifthen ~= nil) then 
    for ite,elem in pairs(Ifthen) do    
      for _,finalstruct_elem in ipairs (final_struct) do
        if (finalstruct_elem.id == elem.id and finalstruct_elem.kind == "ifthen") then 
          finalstruct_elem.value = elem.nb_time
          break
        end
      end
    end
  end
  if (Ifelse ~= nil) then 
    for ite,elem in pairs(Ifelse) do
      for _,finalstruct_elem in ipairs (final_struct) do
        if (finalstruct_elem.id == elem.id and finalstruct_elem.kind == "ifelse") then 
          finalstruct_elem.value = elem.nb_time
          break
        end
      end
    end
  end

  return final_struct
end

-- Get the maximum number of times that a variable is called with a specific value from a list of variables/values
function s2s:get_max(values)
  local max = 0;
  local val
  for _, elem in pairs(values) do
    if (elem.nb_times > max) then 
      val = elem.val
      max = elem.nb_times
    end
  end

  return val
end

-- Print a debug message with a critic level
-- It was a failed prototype
-- Main idea was to have a unique function to print all debug messages only when required by the user and print only for some level of information.
-- Currently it print just debug messages with only one level of debug information...
function s2s:DBG (txt, lvl_require)
   if lvl == nil then 
      return; 
   end
   if lvl_require == nil then
    lvl_require = 1
   end
   
   if lvl_require <= assist_struct["DEBUG"] then
      -- if maqao_isatty (io.stdout) == 1 then
         -- print ("\27[35mDEBUG: "..txt.."\27[0m")
      -- else
         print ("DEBUG: "..txt)
      --end
   end
end

-- Handle options of assist
function s2s:src_options(src, input, excludeFile, outputprefix, sub_oneview_array, var_struct, loopsLines, options, replace, args, argsopt)
  local srcType = tostring(lfs.attributes(src,"mode"))
  if srcType == "directory" then
    s2s:getAllfiles(input, src)
  else
    input = String:split(src,",")
  end

  --Table:tostring(input)

  for i=1, #input do
    if args.srcDir ~= nil then 
      src = args.srcDir..input[i]
    else 
      src = input[i]
    end 
    if excludeFile[fs.basename(src)] == nil then 
      s2s:DBG("Analyze a src file ")

      Message:info("-> START TRANSFORMATION ON "..src)
      if args.f2008 ~= nil then
        s2s:DBG("Manage a f2008 src file ",2)
        s2s:no_fort2008(src)
        -- s2s.start(src, outputprefix, -1, loopsLines, options)
        s2s.start_oneview(src, 
                        outputprefix, 
                        #sub_oneview_array, 
                        sub_oneview_array, 
                        #var_struct, 
                        var_struct,
                        options)
        s2s:undo_no_fort2008(src)
        s2s:undo_no_fort2008(outputprefix..src)
      elseif args.handle_macro ~= nil or args.hm ~= nil then
        s2s:DBG("Manage a src file with macros",2)
        -- s2s.start(src, outputprefix, -1, loopsLines, options)
        s2s.start_oneview(src, 
                        outputprefix, 
                        #sub_oneview_array, 
                        sub_oneview_array, 
                        #var_struct, 
                        var_struct,
                        options) 
        s2s:post_traitement_macro(src, "maqao_"..src)
        if (s2s:file_exists("maqao_analyze_"..src)) then 
          s2s:post_traitement_macro(src, "maqao_analyze_"..src)
        end
      else              
        s2s:DBG("Manage a classical src file ",2)
        -- s2s.start(src, outputprefix, -1, loopsLines, options)
        s2s.start_oneview(src, 
                        outputprefix,  
                        #sub_oneview_array, 
                        sub_oneview_array, 
                        #var_struct, 
                        var_struct,
                        options)
      end

      
      if replace ~= nil then
        s2s:DBG("Replace the original file ")
        file = src:match( "([^/]+)$" )

        if (replace == "true") then
          path = string.gsub(src,file,"")
        else 
          path = replace.."/"
        end
        if (path == nil ) then path = "./" end
        
        os.execute("mv maqao_"..file .." "..path..file )
      end
    end
  end

  --[[ 
    At the end of the first phase of auto specialization ASSIST create maqaoCptTmpVarSpe.cpt 
    Here, we re-ordering and modify the file to have a lua format.
    It's done here because it's easier to manage.
  --]]
  if (s2s:file_exists("maqaoCptTmpVarSpe.cpt") and argsopt == "autothune_p1") then 
    s2s:DBG("Work on the maqaoCptTmpVarSpe.cpt",2)
    os.execute("echo \"elements = {\" > maqaoCptTmpVarSpe.cpt_")
    -- os.execute("head -n 1 maqaoCptTmpVarSpe.cpt > maqaoCptTmpVarSpe.cpt_")
    os.execute("sort -k 1,1 maqaoCptTmpVarSpe.cpt >> maqaoCptTmpVarSpe.cpt_")
    -- os.execute("tail -n +2 maqaoCptTmpVarSpe.cpt | sort -k 1,1 >> maqaoCptTmpVarSpe.cpt_")
    os.execute("echo \"}\" >> maqaoCptTmpVarSpe.cpt_")

    os.execute("mv maqaoCptTmpVarSpe.cpt_ maqaoCptTmpVarSpe.cpt")
  end


  Message:info ("-> ... FINISH TRANSFORMATIONS on "..src)
end

-- Phase 3 of "autotuning", check that the file ith all information of phase2 exust and load these information
function s2s:autothunep3 (args, var_struct)
  local cptvarSpe;
  local vprofres;

  --[[ 
    maqaoCptTmpVarSpe.cpt: is the file where ASSIST write information
    about which variable he want to analyze/specialize and how to find them after 
  --]]
  if (args.cptvarSpe ~= nil) then 
    if (s2s:file_exists(args.cptvarSpe)) then 
      dofile(args.cptvarSpe)
    else 
      print("ERROR ".. args.cptvarSpe .. " does not exist")
    end
    cptvarSpe = args.cptvarSpe
  elseif (s2s:file_exists("maqaoCptTmpVarSpe.cpt")) then
    cptvarSpe = "maqaoCptTmpVarSpe.cpt"
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

  var_struct = s2s:autothunep2(cptvarSpe, vprofres)
  return var_struct
end

-- Modify automatically Makefile to add link to our VPROF lib when compiling for the autotune feature
local function modify_makefile(make_path, Lpath, Ipath)
  local f_orig = io.open(make_path.."/Makefile.orig", "r")
  local f_av = io.open(make_path.."/Makefile.AV", "w")
  local line = ""
  local include = " -I"..Ipath
  local link = " -lvprof.fct-lib-dyn-c -Lib/x86_64-linux-gnu/ -lglib-2.0 -L"..Lpath
  local ret = 0

  while line ~= nil and ret == 0 do
  	if string.match(line, "^LDFLAGS") ~= nil then
  		f_av:write(line.." "..link.."\n")
  		ret = ret + 1
  	else
  		f_av:write(line.."\n")
  	end
  	line = f_orig:read("*l")
  end

  if ret < 1 then
  	Message:error("Please use LDFLAGS in your Makefile.")
  	os.exit(1)
  end

  while line ~= nil do
  	if string.match(line, ".*%.s:") ~= nil or string.match(line, ".*%.o:") ~= nil then
  		f_av:write(line.."\n")
  		line = f_orig:read("*l")
  		f_av:write(line..include.."\n")
  	else
  		f_av:write(line.."\n")
  	end
  	line = f_orig:read("*l")
  end


  f_orig:close()
  f_av:close()
end

-- 
local function modify_vproflibres(path_orig, path_new)
  local f_orig = io.open(path_orig, "r")
  local f_new = io.open(path_new, "w")
  local line
  
  repeat
    line = f_orig:read("*l")
    until line == nil 
          or string.match(line, "Int32 = {") 
          or string.match(line, "Int64 = {") 
          or string.match(line, "Float = {")
          or string.match(line, "Ifthen = {")

  while line ~= nil do
    if string.match(line, "^  }$") then
      f_new:write(line..",\n")
    else  
      f_new:write(line.."\n")
    end

    line = f_orig:read("*l")
  end
  f_orig:close()
  f_new:close()
end


-- Phase 1
function s2s:autothune(src, excludeFile, outputprefix, loopsLines, options, replace, args, argsopt, Lpath, Ipath)
  local input = {}
  local var_struct= {}
  local sub_oneview_array = {}

  -- Path on autothune_p1
  for i=1, #options do
  	if options[i] == "autothune" then
  		table.remove(options, i)
  		break
  	end
  end
  table.insert(options, "autothune_p1")
  argsopt = "autothune_p1"
  
  s2s:src_options(src, input, excludeFile, outputprefix, sub_oneview_array, var_struct, loopsLines, options, replace, args, argsopt)

  local pwd = io.popen("pwd"):read("*l")
  input = {}
  local srcType = tostring(lfs.attributes(src,"mode"))
  if srcType == "directory" then
    s2s:getAllfiles(input, src)
  else
    input = String:split(src,",")
  end
  -- rename src files with sufix _orig
  -- rename maqao_analyze_ sy supprissing prefix and moved it in the src directory
  for i=1, #input do
    local src
    if args.srcDir ~= nil then 
      src = args.srcDir..input[i]
    else 
      src = input[i]
    end
    local src_file = string.gsub(src, ".*/", "")
    local src_path =  string.match(src, ".*/")
    if src_path == nil then
      src_path = "./"
    end
    if excludeFile[fs.basename(src)] == nil and string.match(src, outputprefix) == nil then
      os.execute("mv "..src.." "..src.."_orig")
      if string.match(src_path, pwd) ~= nil then
        os.execute("mv "..pwd.."/"..outputprefix.."analyze_"..src_file.." "..src)
      else
        os.execute("mv "..pwd.."/"..outputprefix.."analyze_"..src_file.." "..pwd.."/"..src)
      end
    end
  end

  -- modifier Makefile
  Message:info ("-> START COMPILATION ... ")
  local make_path = string.match(args.makefile, ".*/")
  os.execute("mv "..make_path.."/Makefile "..make_path.."/Makefile.orig")
  modify_makefile(make_path, Lpath, Ipath)
  os.execute("ln -s "..make_path.."/Makefile.AV "..make_path.."/Makefile")
  
  -- compile and execute
  os.execute("cd "..make_path.."; make clean; make")
  Message:info ("-> ... FINISH COMPILATION")
  Message:info ("-> START EXECUTION OF "..args.bin)
  os.execute("./"..args.bin.." >> tmp_vproflibres.lua")
  modify_vproflibres("tmp_vproflibres.lua", "vproflibres.lua")
  os.execute("rm tmp_vproflibres.lua")
  Message:info ("-> ... FINISH EXECUTION OF "..args.bin)

  -- remove maqao_ prefix
  for i=1, #input do
    local src
    if args.srcDir ~= nil then 
      src = args.srcDir..input[i]
    else 
      src = input[i]
    end
    local src_file = string.gsub(src, ".*/", "")
    local src_path =  string.match(src, ".*/")
    if src_path == nil then
      src_path = "./"
    end
    if excludeFile[fs.basename(src)] == nil and string.match(src, outputprefix) == nil then
      os.execute("mv "..src.." "..src.."_analyze")
      os.execute("mv "..pwd.."/"..outputprefix..src_file.." "..src)
    end
  end

  -- execute autothune_p3
  var_struct = s2s:autothunep3(args, var_struct)
  
  for i=1, #options do
  	if options[i] == "autothune_p1" then
  		table.remove(options, i)
  		break
  	end
  end
  table.insert(options, "autothune_p3")
  argsopt = "autothune_p3"
  input = {}
  s2s:src_options(src, input, excludeFile, outputprefix, sub_oneview_array, var_struct, loopsLines, options, replace, args, argsopt)
  
  -- suppress sufix
  input = {}
  local srcType = tostring(lfs.attributes(src,"mode"))
  if srcType == "directory" then
    s2s:getAllfiles(input, src)
  else
    input = String:split(src,",")
  end
  for i=1, #input do
    local src
    if args.srcDir ~= nil then 
      src = args.srcDir..input[i]
    else 
      src = input[i]
    end
    local src_file = string.gsub(src, ".*/", "")
    local src_path =  string.match(src, ".*/")
    if src_path == nil then
      src_path = "./"
    end
    if excludeFile[fs.basename(src)] == nil and string.match(src, outputprefix) == nil then
    	os.execute("mv "..src.."_orig "..src)
    	os.execute("mv "..src.."_analyze "..src_path.."/"..outputprefix.."analyze_"..src_file)
    end
  end
end