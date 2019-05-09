module ("s2s.rmod_api", package.seeall)

require ("s2s.api")

INFINITY = 99999999999

--[[ Comment all statement bewteen pragmas "#ifdef FORTRAN2008" and #else ... #endif 
		Used for Yales2 which have some Fortran2008 features, 
		Rose don't take care of them and crash
]]--
function s2s:no_fort2008(inputfile)
	--message ("DEBUG - no_fort2008("..inputfile..")")
	-- body
	local path = s2s:getPath(inputfile)
	local filename = s2s:getFileFromPath(inputfile)

    if (path == nil ) then path = "./" end
	local file = io.open(inputfile, "r");
	if file ~= nil then 
		local in_if_def   = false
		local in_else_def = false
		local ftow

		ftow = io.open(path.."tmp_maqao_"..filename, "w");
    	if(ftow == nil) then print("Warning: tmp_maqao_"..filename.." not founded.") end
		for line in file:lines() do

			--If we found the directive to indicate it's fortran 2008
			if line:find("#ifdef FORTRAN2008") ~= nil then  
				in_if_def = true
				ftow:write("!"..line.."\n")
			-- For all statement defined in the directive
			elseif in_if_def == true then  
				ftow:write("!"..line.."\n")
				if string.lower(line):find("#else") ~= nil then
					 in_else_def = true
					 in_if_def = false
				end			
			-- Comment the endif statement (last statement to comment)
			elseif in_else_def == true and string.lower(line):find("#endif") ~= nil then  
				ftow:write("!"..line.."\n")
				in_else_def = false;
			-- If it's a classic statement
			else--if in_else_def == false and in_if_def == false then   
				ftow:write(line.."\n")
			end
		end

		file:close()
		ftow:close()

		print("mv "..path.."tmp_maqao_"..filename.." "..path..filename)
		os.execute("mv "..path.."tmp_maqao_"..filename.." "..path..filename)
		--oneview:cp("tmp_maqao_"..inputfile, inputfile)
		--print(path.."tmp_maqao_"..filename)
		--os.remove(path.."tmp_maqao_"..filename)
	else 
		print("File "..inputfile.. " not found !")
	end
end

--[[ Uncomment all statement bewteen pragmas "#ifdef FORTRAN2008" and #else ... #endif 
		Used for Yales2 which have some Fortran2008 features, 
		Rose don't take care of them and crash
]]--
function s2s:undo_no_fort2008(inputfile)
	--message ("DEBUG - no_fort2008("..inputfile..")")
	-- body

	local path = s2s:getPath(inputfile)
	local filename = s2s:getFileFromPath(inputfile)
    if (path == nil ) then path = "./" end

	file = io.open(path..filename, "r");
	if file ~= nil then 
		local in_if_def   = false
		local in_else_def = false
		local ftow
		ftow = io.open(path.."tmp_maqao_"..filename, "w");

		for line in file:lines() do

			--If we found the directive to indicate it's fortran 2008
			if line:find("!#ifdef FORTRAN2008") ~= nil then  
				in_if_def = true
				ftow:write(string.sub(line,2).."\n")
			-- end if find("#ifdef FORTRAN2008")

			-- For all statement defined in the directive
			elseif in_if_def == true then  
				ftow:write(string.sub(line,2).."\n")
				if string.lower(line):find("!#else") ~= nil then
					 in_else_def = true
					 in_if_def = false
				end
			-- end if in_if_def == true
			
			-- Comment the endif statement (last statement to comment)
			elseif in_else_def == true and string.lower(line):find("!#endif") ~= nil then  
				ftow:write(string.sub(line,2).."\n")
				in_else_def = false;
			-- If it's a classic statement
			else--if in_else_def == false and in_if_def == false then   
				ftow:write(line.."\n")
			end
		end
		file:close()
		ftow:close()

		--print("mv "..path.."tmp_maqao_"..filename.." "..path..filename)
		os.execute("mv "..path.."tmp_maqao_"..filename.." "..path..filename)
	else 
		print("File "..path..filename.. " not found !")
	end
end

--[[ Restore all statements in macro which were not handle during the transformations ]]--
function s2s:post_traitement_macro(inputfile, transinputfile)
  --message("DEBUG - post_traitement_macro")
	-- print ("post_traitement_macro("..inputfile..","..transinputfile..")")

	trans_file 	= io.open(    transinputfile, "r");
	orig_file 	= io.open(         inputfile, "r");
	outputfile 	= io.open("tmpASSISTtmp_"..fs.basename(inputfile), "w+");
	orig_line = orig_file:read()

	for trans_line in trans_file:lines() do
		-- print("TransLine: "..trans_line)
		
		if (trans_line:find("#ifdef") ~= nil or trans_line:find("#ifndef") or trans_line:find("#if "))
		and string.sub(trans_line, string.find(trans_line,"^.")) == "#" then
			-- Search to corresponding line in tu input trans_file
			while orig_line ~= trans_line do 
				if orig_line == nil then print("orig_line == nil"); break end
				orig_line = orig_file:read()
			end
			
			outputfile:write(trans_line.."\n") -- write the #if

			line_to_write = ""
			next_trans_line = trans_file:read() --look into the next line in the maqao_<input> file
			-- move forward while is a directive
			while string.find(next_trans_line,"#define") ~= nil
				or string.find(next_trans_line,"#include") ~= nil
				or string.find(next_trans_line,"#ifdef") ~= nil
				or string.find(next_trans_line,"#ifndef") ~= nil
				or string.find(next_trans_line,"^/")
				or string.find(next_trans_line,"/$") ~= nil do 
				line_to_write = line_to_write..next_trans_line.."\n"
				next_trans_line = trans_file:read()
			end

			if string.find(next_trans_line,"#endif") -- if it IS empty - Case 1
			  or string.find(next_trans_line,"#else") 
			  or string.find(next_trans_line,"#elif") then 
			  -- print("Case 1 - IS empty")			
				current_pos_file = orig_file:seek() 

			  next_orig_line = orig_file:read()
			  line_to_write = "" --next_orig_line.."\n"
				--write line from the orig_file
				while string.find(next_orig_line,"#endif") == nil 
				  and string.find(next_orig_line,"#else") == nil 
				  and string.find(next_orig_line,"#elif") == nil do
					line_to_write = line_to_write..next_orig_line.."\n"
					next_orig_line = orig_file:read()
				end -- end while; end after write en #else of #endif
				
				-- print("case 1 - "..line_to_write)
				line_to_write = line_to_write..next_orig_line.."\n"
				outputfile:write(line_to_write)
				current_pos2 = orig_file:seek("set",current_pos_file) 
				orig_line = orig_file:read()

			else -- if it IS NOT empty - Case 2
			  print("Case 2 - IS NOT empty")
				--write line from the trans_file
				while string.find(next_trans_line,"#endif") == nil 
				  and string.find(next_trans_line,"#else") == nil 
				  and string.find(next_trans_line,"#elif") == nil do
					line_to_write = line_to_write..next_trans_line.."\n"
					next_trans_line = trans_file:read()
				end -- end while; end after write en #else of #endif

				-- print("case 2 - "..line_to_write)
				line_to_write = line_to_write..next_trans_line.."\n"
				outputfile:write(line_to_write)
			end
						
			--if the #if contain a #else
			if (string.find(next_trans_line,"#else") or string.find(next_trans_line,"#elif") )
			and string.sub(next_trans_line, string.find(trans_line,"^.")) == "#" then 

				-- move forward to the #else part
				while orig_line ~= next_trans_line do 
					orig_line = orig_file:read()
				end

				--outputfile:write(next_trans_line.."\n") -- write the #else
				next_trans_line = trans_file:read()

				line_to_write = ""
				-- move forward while is a directive
				while string.find(next_trans_line,"#define") ~= nil
					or string.find(next_trans_line,"#include") ~= nil
					or string.find(next_trans_line,"#ifdef") ~= nil
					or string.find(next_trans_line,"#ifndef") ~= nil
					or string.find(next_trans_line,"^/")
					or string.find(next_trans_line,"/$") ~= nil do 
					line_to_write = line_to_write..next_trans_line.."\n"
					next_trans_line = trans_file:read()
				end

				if string.find(next_trans_line,"#endif") -- if it IS empty - Case 3
				 or string.find(next_trans_line,"#else") 
				 or string.find(next_trans_line,"#elif") then 
					-- print("Case 3 - IS empty")
				  	next_orig_line = orig_file:read()
				  	line_to_write = "" 
					--write line from the orig_file
					while string.find(next_orig_line,"#endif") == nil 
					  and string.find(next_orig_line,"#else") == nil 
					  and string.find(next_orig_line,"#elif") == nil do
						line_to_write = line_to_write..next_orig_line.."\n"
						next_orig_line = orig_file:read()
					end -- end while; end after write en #else of #endif
					
					line_to_write = line_to_write..next_orig_line.."\n"
					-- print("Case 3 - "..line_to_write)
					outputfile:write(line_to_write)

				else -- if it IS NOT empty - Case 4
					-- print("Case 4 - IS NOT empty")
					--write line from the trans_file
					while string.find(next_trans_line,"#endif") == nil 
					  and string.find(next_trans_line,"#else") == nil 
					  and string.find(next_trans_line,"#elif") == nil do
						line_to_write = line_to_write..next_trans_line.."\n"
						next_trans_line = trans_file:read()
					end -- end while; end after write en #else of #endif

					-- print("Case 4 - "..line_to_write)
					line_to_write = line_to_write..next_trans_line.."\n"
					outputfile:write(line_to_write.."\n") -- write the #else
				end

			end

		else -- write the current line
			outputfile:write(trans_line.."\n")
		end --end if found "#ifdef" 
	end --end for line in trans_file:lines()
	trans_file:close()
	orig_file:close()
	outputfile:close()
	os.rename("tmpASSISTtmp_"..fs.basename(inputfile), transinputfile)
end -- end function

--[[ 
	Extract all modules from a file. 
	It created as files as modules in the file to anallyze 
	It is preferable to use s2s:extract_mod_headers() instead.
	We only need headers of functions etc, and some function body can cause Rose errors
]]--
function s2s:extract_mod(inputfile)
  --print("DEBUG - extract_mod")
	file = io.open(inputfile, "r");
	if file ~= nil then 
		writeMod = false
		local ftow
		local newmod
		for line in file:lines() do
			if writeMod == false then 
				if string.lower(line):find("module") ~= nil then  
					modStmt = string.lower(line):find("module")

					commentStmt = string.lower(line):find("!")
					if commentStmt == nil then commentStmt = string.lower(line):find("c") end
					if commentStmt == nil then commentStmt = INFINITY end
					stringStmt = string.lower(line):find("\"")
					if stringStmt == nil then stringStmt = INFINITY end

					if modStmt ~= nil and modStmt < commentStmt and modStmt < stringStmt then 
						--print("line : "..string.lower(line))
						writeMod = true
						subline = string.sub(line,modStmt+7)
						newmod = string.sub(subline,1,1)
						local i = 2
						while string.sub(subline,i,i) ~= " " and i <= subline:len() do
							--print(string.sub(subline,i,i))
							newmod = newmod..string.sub(subline,i,i)
							i = i + 1
						end
						path = inputfile:match("(.*/)")
						if (path == nil) then 
							path="" 
						end
						ftow = io.open(path..newmod..".rmod", "w+");
						print("(EM) creation of ".. newmod..".rmod")
						ftow:write("\n")
						ftow:write("! ===================================================================================\n")
						ftow:write("! <<Automatically generated for Rose Fortran Separate Compilation, DO NOT MODIFY IT>>\n")
						ftow:write("! ===================================================================================\n")
						ftow:write("\n")
						ftow:write(line)
						ftow:write("\n")
					end 
				end
			else 
				if string.lower(line):find("end module") ~= nil then  
					ftow:write(line)
					ftow:write("\n")
					ftow:close()
					-- s2s:no_fort2008(newmod..".rmod")
					writeMod = false
				else
					if (string.lower(line):find("c") ~= nil) then
						if (string.lower(line):find("c") ~= 1) then
							ftow:write(line)
							ftow:write("\n")
						end
					else 
						ftow:write(line)
						ftow:write("\n")
					end
				end
			end
		end
	else
		print("(EM) file "..file.." not found ")
	end
end

--[[ 
	Extract all modules from a file into a rmod file.
	This function extract only headers functions and not their bodies.
]]--
function s2s:extract_mod_headers(inputfile)
  --print("DEBUG - extract_mod_headers")
	file = io.open(inputfile, "r");
	if file ~= nil then 
		writeMod = false
		contains = false
		continueline = false
		isInIfdef = false
		local ftow
		local newmod
		local endType = ""
		for line in file:lines() do
			if writeMod == false then 
				if string.lower(line):find("module") ~= nil then  
					modStmt = string.lower(line):find("module")

					commentStmt = string.lower(line):find("!")
					if commentStmt == nil then commentStmt = string.lower(line):find("c") end
					if commentStmt == nil then commentStmt = INFINITY end
					stringStmt = string.lower(line):find("\"")
					if stringStmt == nil then stringStmt = INFINITY end

					if modStmt ~= nil and modStmt < commentStmt and modStmt < stringStmt then 
						--print("line : "..string.lower(line))
						writeMod = true
						subline = string.sub(line,modStmt+7)
						newmod = string.sub(subline,1,1)
						local i = 2
						while string.sub(subline,i,i) ~= " " and i <= subline:len() do
							--print(string.sub(subline,i,i))
							newmod = newmod..string.sub(subline,i,i)
							i = i + 1
						end
						path = inputfile:match("(.*/)")
						if (path == nil) then 
							path="" 
						end
						ftow = io.open(path..newmod..".rmod", "w+");
						print("(EM) creation of ".. newmod..".rmod")
						ftow:write("\n")
						ftow:write("! ===================================================================================\n")
						ftow:write("! <<Automatically generated for Rose Fortran Separate Compilation, DO NOT MODIFY IT>>\n")
						ftow:write("! ===================================================================================\n")
						ftow:write("\n")
						ftow:write(line)
						ftow:write("\n")
					end 
				end
			else 
				if string.lower(line):find("end module") ~= nil then 
					ftow:write(line)
					ftow:write("\n")
					ftow:close()
					-- s2s:no_fort2008(newmod..".rmod")
					writeMod = false
				else
					if (contains == true) then --On a passé le contains, ne garder que les en-tête de fonctions à partir de maintenant
						if (string.match(string.lower(line), "^[%s]*subroutine")) then 
							ftow:write(line)
							ftow:write("\n")
							if (string.match(line, "&")) then
								continueline = true
								endType = "subroutine"
							else
								ftow:write("end subroutine")
								ftow:write("\n")
							end

						elseif (string.match(string.lower(line), "^[%s]*function")) then
							ftow:write(line)
							ftow:write("\n")
							if (string.match(line, "&")) then
								continueline = true
								endType = "function"
							else
								ftow:write("end function")
								ftow:write("\n")
							end

						elseif (continueline == true) then
							if (string.match(line, "&")) then
								continueline = true
								ftow:write(line)
								ftow:write("\n")
							else
								continueline = false
								ftow:write(line)
								ftow:write("\n")
								ftow:write("end "..endType)
								ftow:write("\n")
							end
						elseif (string.match(string.lower(line), "^[%s]*#")) then
							if (string.match(string.lower(line), "^[%s]*#ifdef")) then 
								isInIfdef = true
							end
							if (string.match(string.lower(line), "^[%s]*#endif")) then 
								isInIfdef = false
							end
							ftow:write(line)
							ftow:write("\n")
						end

					elseif (contains == false) then -- if contains = false
						if (string.match(string.lower(line), "^[%s]*contains")) then
							ftow:write(line)
							ftow:write("\n")
							contains = true
						elseif (string.match(string.lower(line), "^[%s]*#")) then
							if (string.match(string.lower(line), "^[%s]*#ifdef") 
								or string.match(string.lower(line), "^[%s]*#if def") 
								or string.match(string.lower(line), "^[%s]*#if 0")) then 
								isInIfdef = true
							end
							if (string.match(string.lower(line), "^[%s]*#endif")) then 
								isInIfdef = false
							end
							ftow:write(line)
							ftow:write("\n")
						elseif (string.lower(line):find("^c ")) then
							--do nothing it's an old fortran comment

						else
							if (isInIfdef == false) then
								ftow:write(line)
								ftow:write("\n")
							end
						end
					end
				end
			end
		end
	else
		print("(EM) file "..file.." not found ")
	end
end

fileTreated ={}

--[[ Created a rmod file from a file .f90  ]]--
function s2s:create_rmod_file (filetoanalyze, includeDir)
  --message("DEBUG - create_rmod_file")
	--print("filetoanalyze : "..filetoanalyze..".rmod")
	rmod_file = io.open(filetoanalyze..".rmod", "r");
	local i=1
	local location = "./"
	while rmod_file == nil and i <= #includeDir do
		rmod_file = io.open(includeDir[i]..filetoanalyze..".rmod", "r");
		i = i + 1
	end
	if rmod_file ~= nil then 
		rmod_file:close()
		--print("rmod file is already existing !") 
		--return true 
	end

	i = 1
	src_file = io.open(filetoanalyze..".f90", "r");
	while src_file == nil and i <= #includeDir do
		--print(includeDir[i])
		src_file = io.open(includeDir[i]..filetoanalyze..".f90", "r");
		location = includeDir[i]
		i = i + 1
	end

	if src_file ~= nil then 
		if Table.contains(fileTreated, location..filetoanalyze..".rmod") == false then
			ftow = io.open(location..string.lower(filetoanalyze)..".rmod", "w");
			ftow:write("\n")
			ftow:write("! ===================================================================================\n")
			ftow:write("! <<Automatically generated for Rose Fortran Separate Compilation, DO NOT MODIFY IT>>\n")
			ftow:write("! ===================================================================================\n")
			ftow:write("\n")

			print("Creation of "..location..filetoanalyze..".rmod")
			local t = src_file:read("*all") -- capture file in a string
			t = t:gsub('!(.-)\n','\n') -- delete all comments
			--t = t:gsub("^%s*\r?", '') -- delete all blank lines
			ftow:write(t)
			src_file:close()
			ftow:close()
			--print("RMOD file is created ! \n\n")
			table.insert(fileTreated, location..filetoanalyze..".rmod")
			s2s:rmod_file(filetoanalyze..".rmod",includeDir)
		end
	else 
		print("(CRF) src_file "..filetoanalyze.." is NOT available")

	end
end

function s2s:rmod_file (filetoanalyze, includeDir)
  --message("DEBUG - rmod_file")
	--print("file to analyze : "..filetoanalyze)
	local modName = {}
	local j=1
	local i=1
	--file = io.open(filetoanalyze, "r");
	file = nil
	while file == nil and i <= #includeDir do
		file = io.open(includeDir[i]..filetoanalyze, "r");
		i = i + 1
	end

	if file ~= nil then 
		for line in file:lines() do
			useStmt = string.lower(line):find("use ")

			commentStmt = string.lower(line):find("!")
			if commentStmt == nil then commentStmt = string.lower(line):find("c") end
			if commentStmt == nil then commentStmt = INFINITY end
			stringStmt = string.lower(line):find("\"")
			if stringStmt == nil then stringStmt = INFINITY end

			if useStmt ~= nil and useStmt < commentStmt and useStmt < stringStmt then 
				--print("line : "..line)
				for i = useStmt+4, #line do
					if string.sub(line,i,i) == "," then
						break
					elseif string.sub(line,i,i) ~= " " then
						modName[j] = string.sub(line,i,i)
						j = j + 1
						--table.insert(modName,string.sub(line,i,i))
					elseif string.sub(line,i,i) ~= "!" then
						break
					else 
						print("ERROR : NIL")
					end
				end
				if next(modName) ~= nil then
					s_modName = table.concat(modName)
					--print("modName : "..s_modName)
					s2s:create_rmod_file(s_modName, includeDir)
				end
				j = 1
				modName = {}
			end
		end
		--if file ~= nil then file:close() end
	else 
		print("File "..filetoanalyze.." not founded")
	end
end

--[[
	Create all $file.rmod existring by extracting all modules from all files recursively in all directories
	lvl is juste used for debug, it's used as increment to add space with string.rep(str, lvl)
--]]
function s2s:extract_all_mod(directory, lvl)
  --message("DEBUG - extract_all_mod")
	if (lvl == nil ) then 
		lvl = 0
	end

	local content = fs.readdir (directory)
	for _, file in pairs(content) do

		local fileType = tostring(lfs.attributes(directory.."/"..file.name,"mode"))
		-- print(string.rep(" ",lvl*2).."----------------")
		-- print(string.rep(" ",lvl*2)..file.name .. " " ..fileType)

		if ((file.name == "..") or (file.name == ".")) then
			--Do nothing
	    elseif fileType == "file" then 
	    	-- print("Extract all mod from "..directory.."/"..file.name)
	    	ext = file.name:match("^.+(%..+)$")
	    	if ext ~= nil then 
	    		if ext:match("f") or ext:match("F") then
	    			s2s:extract_mod_headers(directory.."/"..file.name)
	    		end
	    	end
	    elseif fileType == "directory" then 
	        s2s:extract_all_mod(directory.."/"..file.name, lvl+1)
	    end
	end
end

--[[
	Remove $file.rmod existring recursively in all directories
	lvl is juste used for debug, it's used as increment to add space with string.rep(str, lvl)
--]]
function s2s:clean_all_rmod( directory )
  --message("DEBUG - clean_all_rmod")
	if (lvl == nil ) then 
		lvl = 0
	end

	local content = fs.readdir (directory)
	for _, file in pairs(content) do
		local fileType = tostring(lfs.attributes(directory.."/"..file.name,"mode"))
		-- print(string.rep(" ",lvl*2).."----------------")
		-- print(string.rep(" ",lvl*2)..file.name .. " " ..fileType)

		if ((file.name == "..") or (file.name == ".")) then
			--Do nothing
	    elseif fileType == "file" then 
	    	if (file.name:match("(.*rmod)")) then 
	    		--remove the file
	    		os.remove(directory.."/"..file.name)
	    	end
	    elseif fileType == "directory" then 
	        s2s:clean_all_rmod(directory.."/"..file.name, lvl+1)
	    end
	end
end