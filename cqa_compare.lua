module ("s2s.cqa_compare", package.seeall)

require ("s2s.fct_from_maqao")

--- Structure of the cqa compare table ---
--	compare.<name_src>[tag].v_<prev>.loop_id : number of the file in the ov_xp directory sources/cqa 
--								   .l_start : line start of the loop
--							   	   .l_stop : line stop of the loop
--							   	   .metrics : cqa metrics indicated in csv files in ov_xp/cqa directory
--								   .metrics_ORIG : cqa metrics indicated in csv files in ov_xp/cqa directory
--					  		.v_<next>
-- tag : represent the label of the analysed loop


local function find_src_path(ov_dir, loop_id, src_dir)
	local path = ov_dir.."src_binary_"..loop_id..".txt"
	if fs.exists(path) == false then
		return ""
	end
	local file = io.open(path, "r")
	local  name = file:read("*l")
	name = string.gsub(name, ": %d* ", "")
	name = string.gsub(name, "- %d*", "")
	name = string.gsub(name, src_dir, "")
	file:close()
	return name
end


-- Load lprof csv file containing infos on loops in table loop_info
-- Sort loop_info in function of file name and start line number of each loop
local function set_loop_info(ov_dir, auto_input, version, max_loops, is_semi_auto)
	local ov_dir_lprof = nil
	if fs.exists(ov_dir.."/shared/lprof_npsu.csv_lprof_loops_summary_xp_lprof_npsu.csv") == true then
		ov_dir_lprof = ov_dir.."/shared/lprof_npsu.csv_lprof_loops_summary_xp_lprof_npsu.csv"
	elseif fs.exists(ov_dir.."/shared/lprof.csv_lprof_loops_summary_xp_lprof.csv") == true then
		ov_dir_lprof = ov_dir.."/shared/lprof.csv_lprof_loops_summary_xp_lprof.csv"
	else
		Message:error("[set_loop_info] les fichiers"..ov_dir.."/shared/lprof.csv_lprof_loops_summary_xp_lprof.csv ou "..ov_dir.."/shared/lprof_npsu.csv_lprof_loops_summary_xp_lprof_npsu.csv n'existent pas")
		return false
	end

	local loop_info = s2s:load_CSV(ov_dir_lprof, ';')
	for i, content in pairs (loop_info) do
		if version == "vprev" and max_loops <= 0 then
			while loop_info[i] ~= nil do
				table.remove (loop_info, i)
			end
		else
			local name = string.gsub(content["Source Info"], ":%d*-%d*$", "")
			local start = string.gsub(content["Source Info"], "^.*:", "")
			local lend = string.gsub(start, "^.*-", "")
			start = string.gsub(start, "-.*$", "")
			loop_info[i]["Source Info"] = {}
			if is_semi_auto == true then
				if fs.exists(auto_input.src.."/"..name) == false then
					loop_info[i]["Source Info"].name = find_src_path(ov_dir.."/sources/", loop_info[i]["Loop ID"], auto_input.src)
				else
					loop_info[i]["Source Info"].name = name
				end
			else
				loop_info[i]["Source Info"].src_path = find_src_path(ov_dir.."/sources/", loop_info[i]["Loop ID"], auto_input.src)
				loop_info[i]["Source Info"].name = name
				loop_info[i]["Source Info"].src_path = string.gsub(loop_info[i]["Source Info"].src_path, loop_info[i]["Source Info"].name, "")
			end
			loop_info[i]["Source Info"].l_start = tonumber(start)
			loop_info[i]["Source Info"].l_end = tonumber(lend)
			
			if version == "vprev"
				and fs.exists(ov_dir.."/cqa/DL1_binary_"..loop_info[i]["Loop ID"].."_cqa.csv") == false
				and fs.exists(ov_dir.."/cqa/_binary_"..loop_info[i]["Loop ID"].."_cqa.csv") == false then
				loop_info[i]["Source Info"].name = ""
			end

			max_loops = max_loops - 1
		end
	end
	for i = #loop_info, 1, -1 do
		if loop_info[i]["Source Info"].name == "" or loop_info[i]["Source Info"].src_path == "" then
			table.remove(loop_info, i)
		end
	end
	table.sort(loop_info, function(a,b) if a["Source Info"].name == b["Source Info"].name then return a["Source Info"].l_start < b["Source Info"].l_start end return a["Source Info"].name>b["Source Info"].name end)

	return loop_info
end


-- Load the assist.lua file created by oneview to get back informations from lprof and decan
-- @param ov_dir the path to oneview experiment path
-- @return loops the table containing the useful iformations or false if an error occured
local function set_info_from_assist_file(oneview_cleaning_report)
	local loops = {}


	for _, content in pairs (oneview_cleaning_report) do
		loops[content.loop_id] = {
			l_start = content.lineStart,
			l_end = content.lineStop,
			ite_min = content.ite_min,
			ite_max = content.ite_max,
			ite_avg = content.ite_avg,
			r_l1_min = content.r_l1_min,
			r_l1_max = content.r_l1_max,
			r_l1_med = content.r_l1_med,
		}
	end
	return loops
end

-- Test the extension of sources files
-- @param file_name the file's name
-- @return com the right syntax for language's comments or empty string if it's not an accepted extension 
local function test_extension(file_name, is_semi_auto)
	local com = ""
	if string.match(file_name, ".c$") or string.match(file_name, ".cc$") 
		or string.match(file_name, ".cpp$") or string.match(file_name, ".C$")
		or string.match(file_name, ".CC$") or string.match(file_name, ".cxx$") then
		if is_semi_auto == true then
			com = "#pragma MAQAO TAG LOOP "
		else
			com = "#pragma MAQAO LABEL LOOP "
		end
	elseif string.match(file_name, ".f$") or string.match(file_name, ".f90$")
		or string.match(file_name, ".F") or string.match(file_name, ".F90$") then
		if is_semi_auto == true then
			com = "!DIR$ MAQAO TAG LOOP "
		else
			com = "!DIR$ MAQAO LABEL LOOP "
		end
	end
	return com
end



-- Add tags before and after each loops analysed by CQA.
-- @param auto_input : table with informations about the program launch
-- @param compare : the table containing all informations for cqa comparator
-- @return false if error. 0 if success
function s2s:add_tags(auto_input, compare)
	-- Add tags and save origin source files : it copy the file in a new file with "prev_" as prefix.
	for name, tag_table in pairs (compare) do
		if fs.exists(auto_input.src.."/prev_"..name) == false then
			os.execute("cp "..auto_input.src.."/"..name.." "..auto_input.src.."/prev_"..name)
		end
		local f_tag = io.open(auto_input.src.."/"..name, "w")
		if f_tag == nil then
			Message:error("[add_tags] impossible to open file : "..auto_input.src..name)
      		return false
   		end
		local f_orig = io.open(auto_input.src.."/prev_"..name, "r")
		if f_orig == nil then
			Message:error("[add_tags] impossible to open file : "..auto_input.src.."/prev_"..name)
      		return false
   		end

		local cpt, line = 0
		local tag_com = test_extension(name)
		for tag, content in pairs (tag_table) do
			while cpt < (content.vprev.l_start - 1) do
				line = f_orig:read("*L")
				f_tag:write(line)
				cpt = cpt + 1
			end	
			f_tag:write("\n"..tag_com..tag.."\n")
		end

		line = f_orig:read("*a")
		f_tag:write(line)
		
		f_tag:close()
		f_orig:close()
		
	end

	return 0
end


-- Read the source file to check if a tag is present at loop line x
-- If a tag is present, it returns the tag number
-- @param loop_info : table with informations about a loop from the table compare
-- @param auto_input : table with informations filled by the user
-- @return false if error. true and the tag's value if success.
local function is_tag_present(loop_info, src_file, is_semi_auto)
	local file = io.open(src_file, "r")
	if file == nil then
		Message:error("[is_tag_present] Impossible to open file : "..src_file)
		return false
	end
	local nb_lines, i = 1, 0
	local lines_to_check = 3
	while nb_lines < (loop_info["Source Info"].l_start - lines_to_check) do
		file:read("*l")
		nb_lines = nb_lines + 1
	end

	local tag_com = test_extension(loop_info["Source Info"].name)
	local line
	
	while i < lines_to_check do
		line = file:read("*l")
		if string.match(line, tag_com..".*") then
			tag = string.match(line, ".*")
			tag = string.gsub(tag, tag_com, "")
			file:close()
			if is_semi_auto == true then
				return true, tonumber(tag)
			else
				return true, tag
			end
		end
		i = i + 1
	end
	file:close()
	return true, -1
end

function s2s:remove_tags(compare, src_path)
	for name, tag_table in pairs (compare) do
		os.execute("cp "..src_path.."/"..name.." "..src_path.."/tmp_"..name)
		local f_tag = io.open(src_path.."/tmp_"..name, "r")
		local f_no_tag = io.open(src_path.."/"..name, "w")
		local line, tag_com = ""
		while line ~= nil do
			tag_com = test_extension(name)
			if string.match(line, tag_com) == nil then
				f_no_tag:write(line.."\n")
			end
			line = f_tag:read("*l")
		end

		f_tag:close()
		f_no_tag:close()
		os.execute("rm "..src_path.."/tmp_"..name)
		
	end
end

--[[ Fill the table compare with informations about source's code before transformation
	 A loop is selected if the time average of execution is over 1.5%
	 @param ov_dir : The path to oneview experiment path
	 @param compare : the table compare (see comments at the beginning og the file for more informations about this table)
	 @param src_dir : the path to sources root
	 @param max_loops : the maximum of loops that need to be compared
	 @return false if erro. 0 if success
	]]
function s2s:fill_table_compare_vprev(ov_dir, compare, auto_input, max_loops, oneview_cleaning_report, is_semi_auto)
	local loop_info = set_loop_info(ov_dir, auto_input, "vprev", max_loops, is_semi_auto)
	local tag = 1
	local vprof_decan = set_info_from_assist_file(oneview_cleaning_report)
	if loop_info == false or vprof_decan == false then
		return false
	end
	for _, content in pairs (loop_info) do
		if content["Time Average (%)"] > auto_input.loop_time_min or max_loops <= 0 then
			if compare[content["Source Info"].name] == nil then
				tag = 1
				compare[content["Source Info"].name] = {}
			end
			if is_semi_auto == false then
				_, tag = is_tag_present(content, content["Source Info"].src_path.."/"..content["Source Info"].name, is_semi_auto)
			end
			if compare[content["Source Info"].name][tag] == nil then
				compare[content["Source Info"].name][tag] = {}
				compare[content["Source Info"].name][tag].counter = 0
			end
		    if vprof_decan[content["Loop ID"]] ~= nil then
				compare[content["Source Info"].name][tag].vprev = {
					loop_id = content["Loop ID"],
					l_start = content["Source Info"].l_start,
					l_end = content["Source Info"].l_end,
					vprof_decan = {
						ite_min = vprof_decan[content["Loop ID"]].ite_min,
						ite_max = vprof_decan[content["Loop ID"]].ite_max,
						ite_avg = vprof_decan[content["Loop ID"]].ite_avg,
						r_l1_min = vprof_decan[content["Loop ID"]].r_l1_min,
						r_l1_max = vprof_decan[content["Loop ID"]].r_l1_max,
						r_l1_med = vprof_decan[content["Loop ID"]].r_l1_med,
					},
				}
		    else
			    compare[content["Source Info"].name][tag].vprev = {
		    	    loop_id = content["Loop ID"],
		        	l_start = content["Source Info"].l_start,
		        	l_end = content["Source Info"].l_end,
		        }
		    end
		    if is_semi_auto == false then
				compare[content["Source Info"].name][tag].vprev.src_path = content["Source Info"].src_path
			end

			local csv_path = ov_dir.."/cqa/DL1_binary_"..content["Loop ID"].."_cqa.csv"
			if fs.exists(csv_path) == false then
				csv_path = ov_dir.."/_binary_"..content["Loop ID"].."_cqa.csv"
			end	
			compare[content["Source Info"].name][tag].vprev.metrics, ret = s2s:load_CSV(csv_path, ';')
			if ret == -2 then
				Message:error("[fill_table_compare_vprev] impossible to open file : "..csv_path)
				return false
			elseif ret == -3 then
				Message:error("[fill_table_compare_vprev] Second parameter of load_CSV is not of type string")
				return false
			end
		
			tag = tag + 1
		end
	end
	return true
end


--[[ Fill the table compare with informations about source's code after transformation
	 It select loops that were selected for the sources before transformation (if still present after transformations)
	 @param ov_dir : The path to oneview experiment path
	 @param compare : the table compare (see comments at the beginning og the file for more informations about this table)
	 @param auto_input : table with informations filled by the user
	 return false if error. true if success
	]]
function s2s:fill_table_compare_vnext (ov_dir, compare, auto_input, oneview_cleaning_report, is_semi_auto)
	local loop_info = set_loop_info(ov_dir, auto_input, "vnext", math.huge, is_semi_auto)
	local vprof_decan = set_info_from_assist_file(oneview_cleaning_report)
	if loop_info == false or vprof_decan == false then
		return false
	end

	local tag, ret = 1, 0
	local file
	for _, content in pairs (loop_info) do
		if compare[content["Source Info"].name] ~= nil then
			if is_semi_auto == true then
				ret, tag = is_tag_present(content, auto_input.src.."/"..content["Source Info"].name, is_semi_auto)
			else
				ret, tag = is_tag_present(content, content["Source Info"].src_path.."/"..content["Source Info"].name, is_semi_auto)
			end
			if ret == true and compare[content["Source Info"].name][tag] ~= nil then
				if compare[content["Source Info"].name][tag].counter == 1 then
					if compare[content["Source Info"].name][tag].vprev ~= nil then
						local tmp = tag
						tag = #compare[content["Source Info"].name] + 1
						compare[content["Source Info"].name][tag] = {
							counter = 1,
							vprev = compare[content["Source Info"].name][tmp].vprev,
						}
					end					
				else
					compare[content["Source Info"].name][tag].counter = 1
				end
	      		if vprof_decan[content["Loop ID"]] ~= nil
	      			and content["Source Info"].l_start == vprof_decan[content["Loop ID"]].l_start
	      			and content["Source Info"].l_end == vprof_decan[content["Loop ID"]].l_end then
					compare[content["Source Info"].name][tag].vnext = {
						loop_id = content["Loop ID"],
						l_start = content["Source Info"].l_start,
						l_end = content["Source Info"].l_end,
						vprof_decan = {
							ite_min = vprof_decan[content["Loop ID"]].ite_min,
							ite_max = vprof_decan[content["Loop ID"]].ite_max,
							ite_avg = vprof_decan[content["Loop ID"]].ite_avg,
							r_l1_min = vprof_decan[content["Loop ID"]].r_l1_min,
							r_l1_max = vprof_decan[content["Loop ID"]].r_l1_max,
							r_l1_med = vprof_decan[content["Loop ID"]].r_l1_med,
						},
					}
		      	else -- create a function that verify lines codes of start loop and end loop
			        compare[content["Source Info"].name][tag].vnext = {
			        	loop_id = content["Loop ID"],
			        	l_start = content["Source Info"].l_start,
			        	l_end = content["Source Info"].l_end,
			        }
		    	end
		    	if is_semi_auto == false then
					compare[content["Source Info"].name][tag].vnext.src_path = content["Source Info"].src_path
				end


				local csv_path = ov_dir.."/cqa/DL1_binary_"..content["Loop ID"].."_cqa.csv"
				if fs.exists(csv_path) == false then
					csv_path = ov_dir.."/_binary_"..content["Loop ID"].."_cqa.csv"
				end
				if fs.exists(csv_path) == true then
					compare[content["Source Info"].name][tag].vnext.metrics, ret = s2s:load_CSV(csv_path, ';')
					if ret == -2 then
						Message:error("[fill_table_compare_vnext] impossible to open file : "..csv_path)
						return false
					elseif ret == -3 then
						Message:error("[fill_table_compare_vnext] Second parameter of load_CSV is not of type string")
						return false
					end
				else
					table.remove(compare[content["Source Info"].name][tag], vnext)
				end
			elseif ret == false then
				return false
			end
		end
	end
	return true
end



-- Compare metrics 
-- Print a table on standard output
-- @param compare : the table containing all informations for cqa comparator
-- @param output_file : the output file
function s2s:compare_cqa_metrics(compare, output_file, ov_global_metrics)
	local file
	if output_file == nil then
		file = io.stdout
	else 
		file = io.open(output_file, "w")
	end

	if ov_global_metrics.vprev ~= nil and ov_global_metrics.vnext ~= nil then
		file:write("\t\tGLOBAL METRICS                             BEFORE TRANSFO                  AFTER TRANSFO                  IS BETTER ?\n\n")
		file:write("\t\tTotal Time (s) :                              "..ov_global_metrics.vprev.total_time_s.."                             "..ov_global_metrics.vnext.total_time_s.."\n")
		
		file:write("\t\tFlow Complexity :                             "..ov_global_metrics.vprev.flow_complexity.. 				 "                             "..ov_global_metrics.vnext.flow_complexity..					"                            lower is better\n")
		file:write("\t\tArray Access Efficiency (%) :                 "..ov_global_metrics.vprev.array_efficieny.. 				 "                         "..	 	ov_global_metrics.vnext.array_efficieny..					"                         higher is better\n\n")
		file:write("\t\tSpeedup if clean :                            "..ov_global_metrics.vprev.speedup_if_clean..				 "                             "..ov_global_metrics.vnext.speedup_if_clean..				"                            lower is better\n")
		file:write("\t\tNb loops to get 80% if clean :                "..ov_global_metrics.vprev.nb_loop_80_if_clean..		 "                             "..ov_global_metrics.vnext.nb_loop_80_if_clean..	 		"                            lower is better\n\n")
		file:write("\t\tSpeedup if FP Vectorised :                    "..ov_global_metrics.vprev.speedup_if_fp_vect.. 		 "                          "..		ov_global_metrics.vnext.speedup_if_fp_vect..	 		"                         lower is better\n")
		file:write("\t\tNb loops to get 80% if FP vectorized :        "..ov_global_metrics.vprev.nb_loop_80_if_fp_vect..	 "                             "..ov_global_metrics.vnext.nb_loop_80_if_fp_vect..		"                            lower is better\n\n")
		file:write("\t\tSpeedup if Fully Vectorised :                 "..ov_global_metrics.vprev.speedup_if_fully_vect..	 "                          "..		ov_global_metrics.vnext.speedup_if_fully_vect..		"                         lower is better\n")
		file:write("\t\tNb loops to get 80% if Fully vectorized :     "..ov_global_metrics.vprev.nb_loop_80_if_fully_vect.."                             "..ov_global_metrics.vnext.nb_loop_80_if_fully_vect.."                            lower is better\n\n")
		file:write("\t\tSpeedup if data in L1 Cache :                 "..ov_global_metrics.vprev.speedup_if_l1..					 "                          "..		ov_global_metrics.vnext.speedup_if_l1..						"                          lower is better\n")
		file:write("\t\tNb loops to get 80% if data in L1 Cache :     "..ov_global_metrics.vprev.nb_loop_80_if_l1..				 "                             "..ov_global_metrics.vnext.nb_loop_80_if_l1..				"                            lower is better\n\n")
		file:write("\t\tCompilation Options :            "..ov_global_metrics.vprev.compilation_options.."\n\n")
		file:write("\t\t-------------------------------------------------------------------------------\n\n")
	end

	for name, tag_table in pairs (compare) do
		file:write(name.." :\n\n")
				file:write("\t\tMETRICS                                    BEFORE TRANSFO                  AFTER TRANSFO                  IS BETTER ?\n\n")
		for tag, content in pairs (tag_table) do
			if content.vnext == nil or content.vnext.metrics == nil then
				file:write("\t\tLoop lines :                              "..content.vprev.l_start.." - "..content.vprev.l_end.."\t\t\tThis loop is not present after transformations\n\n")
				file:write("\t\tLoop_id :                                 "..content.vprev.loop_id.."\n\n")

				-- Speedup of vectorization 
				local vprev = string.format("%.2f", content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT)"] / content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT) if clean"])
				file:write("\t\tSpeedup if clean :                        "..vprev..string.rep(" ", 55).."lower is better\n")
				vprev = string.format("%.2f", content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT)"] / content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT) if fully vectorized"])
				file:write("\t\tSpeedup if fully vectorized :             "..vprev..string.rep(" ", 55).."lower is better\n")

				--  CQA Metrics
				file:write("\n\t\tBottlenecks :                           "..content.vprev.metrics[1]["bottlenecks"].."\n")
				file:write("\t\tUnroll confidence level :                 "..content.vprev.metrics[1]["unroll confidence level"].."\n")
				file:write("\t\tCycles L1 if clean :                      "..content.vprev.metrics[1]["cycles L1 if clean"]..string.rep(" ", 55).."higher is better\n")
				file:write("\t\tCycles L1 if fully vec :                  "..content.vprev.metrics[1]["cycles L1 if fully vectorized"]..string.rep(" ", 55).."higher is better\n")
				file:write("\t\tVector-efficiency ratio all :             "..content.vprev.metrics[1]["vec eff ratio all"]..string.rep(" ", 55).."higher is better\n")
				file:write("\t\tVectorization ratio all :                 "..content.vprev.metrics[1]["packed ratio all"]..string.rep(" ", 55).."higher is better\n")
				file:write("\t\tFP op per cycle L1 :                      "..content.vprev.metrics[1]["FP operations per cycle L1"]..string.rep(" ", 55).."higher is better\n")

				-- DECAN and VPROF Metrics
				if content.vprev.vprof_decan ~= nil then
					file:write("\n\t\tIterations min :                    "..content.vprev.vprof_decan.ite_min.."\n")
					file:write("\t\tIterations max :                      "..content.vprev.vprof_decan.ite_max.."\n")
					file:write("\t\tIterations avg :                      "..content.vprev.vprof_decan.ite_avg.."\n")
					vprev = string.format("%.2f", content.vprev.vprof_decan.r_l1_min)
					file:write("\t\tr_l1_min :                            "..vprev.."\n")
					vprev = string.format("%.2f", content.vprev.vprof_decan.r_l1_max)
					file:write("\t\tr_l1_max :                            "..vprev.."\n")
					vprev = string.format("%.2f", content.vprev.vprof_decan.r_l1_med)
					file:write("\t\tr_l1_med :                            "..vprev.."\n")
				else
					file:write("\n\t\tNo DECAN and VPROF results available\n")
				end
				file:write("\t\t-------------------------------------------------------------------------------\n\n")
			else
				file:write("\t\tLoop lines :                              "..content.vprev.l_start.." - "..content.vprev.l_end.."                        "..content.vnext.l_start.." - "..content.vnext.l_end.."\n\n")
				file:write("\t\tLoop_id :                                 "..content.vprev.loop_id.."                              "..content.vnext.loop_id.."\n\n")

				-- Speedup of vectorization 
				local vprev = content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT)"] / content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT) if clean"]
				local vnext = content.vnext.metrics[1]["cycles L1 (best case for DIV/SQRT)"] / content.vnext.metrics[1]["cycles L1 (best case for DIV/SQRT) if clean"]
				local is_better
				if vprev > vnext then
					is_better = "\027[32m yes (lower is better)\027[00m"
				elseif vprev < vnext then
					is_better = "\027[00;31m no (lower is better) \027[00m"
				else
					is_better = "lower is better"
				end
				vprev = string.format("%.2f", vprev)
				vnext = string.format("%.2f", vnext)
				file:write("\t\tSpeedup if clean :                        "..vprev..string.rep(" ", 28)..vnext..string.rep(" ", 24)..is_better.."\n")
				
				vprev = content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT)"] / content.vprev.metrics[1]["cycles L1 (best case for DIV/SQRT) if fully vectorized"]
				vnext = content.vnext.metrics[1]["cycles L1 (best case for DIV/SQRT)"] / content.vnext.metrics[1]["cycles L1 (best case for DIV/SQRT) if fully vectorized"]
				if vprev > vnext then
					is_better = "\027[32m yes (lower is better)\027[00m"
				elseif vprev < vnext then
					is_better = "\027[00;31m no (lower is better) \027[00m"
				else
					is_better = "lower is better"
				end
				vprev = string.format("%.2f", vprev)
				vnext = string.format("%.2f", vnext)
				file:write("\t\tSpeedup if fully vectorized :             "..vprev..string.rep(" ", 28)..vnext..string.rep(" ", 24)..is_better.."\n")



				-- CQA Metrics
				local rep = 0
				if string.match(content.vprev.metrics[1]["bottlenecks"], "P%d,") == nil then
					rep = 4
				else
					rep = string.len(content.vprev.metrics[1]["bottlenecks"]) % 5
				end
				file:write("\n\t\tBottlenecks :                             "..content.vprev.metrics[1]["bottlenecks"]..string.rep(" ", rep*7)..content.vnext.metrics[1]["bottlenecks"].."\n")
				file:write("\t\tUnroll confidence level :                 "..content.vprev.metrics[1]["unroll confidence level"]..string.rep(" ", 30)..content.vnext.metrics[1]["unroll confidence level"].."\n")

				vprev = content.vprev.metrics[1]["cycles L1 if clean"]
				vnext = content.vnext.metrics[1]["cycles L1 if clean"]
				if vprev < vnext then
					is_better = "\027[32m yes (higher is better)\027[00m"
				elseif vprev > vnext then
					is_better = "\027[00;31m no (higher is better) \027[00m"
				else
					is_better = "higher is better"
				end
				vprev = string.format("%.2f", vprev)
				vnext = string.format("%.2f", vnext)
				file:write("\t\tCycles L1 if clean :                      "..vprev..string.rep(" ", 28)..vnext..string.rep(" ", 22)..is_better.."\n")

				vprev = content.vprev.metrics[1]["cycles L1 if fully vectorized"]
				vnext = content.vnext.metrics[1]["cycles L1 if fully vectorized"]
				if vprev < vnext then
					is_better = "\027[32m yes (higher is better)\027[00m"
				elseif vprev > vnext then
					is_better = "\027[00;31m no (higher is better) \027[00m"
				else
					is_better = "higher is better"
				end
				vprev = string.format("%.2f", vprev)
				vnext = string.format("%.2f", vnext)
				file:write("\t\tCycles L1 if fully vec :                  "..vprev..string.rep(" ", 28)..vnext..string.rep(" ", 22)..is_better.."\n")
				
				vprev = content.vprev.metrics[1]["vec eff ratio all"]
				vnext = content.vnext.metrics[1]["vec eff ratio all"]
				if vprev < vnext then
					is_better = "\027[32m yes (higher is better)\027[00m"
				elseif vprev > vnext then
					is_better = "\027[00;31m no (higher is better) \027[00m"
				else
					is_better = "higher is better"
				end
				vprev = string.format("%.2f", vprev)
				vnext = string.format("%.2f", vnext)
				file:write("\t\tVector-efficiency ratio all :             "..vprev..string.rep(" ", 28)..vnext..string.rep(" ", 22)..is_better.."\n")
				
				vprev = content.vprev.metrics[1]["packed ratio all"]
				vnext = content.vnext.metrics[1]["packed ratio all"]
				if vprev < vnext then
					is_better = "\027[32m yes (higher is better)\027[00m"
				elseif vprev > vnext then
					is_better = "\027[00;31m no (higher is better) \027[00m"
				else
					is_better = "higher is better"
				end
				vprev = string.format("%.2f", vprev)
				vnext = string.format("%.2f", vnext)
				file:write("\t\tVectorization ratio all :                 "..vprev..string.rep(" ", 28)..vnext..string.rep(" ", 22)..is_better.."\n")

				file:write("\t\tFP op per cycle L1 :                      "..content.vprev.metrics[1]["FP operations per cycle L1"]..string.rep(" ", 20)..content.vprev.metrics[1]["FP operations per cycle L1"]..string.rep(" ", 16).."higher is better\n")

				-- DECAN and VPROF Metrics
				if content.vprev.vprof_decan ~= nil and content.vnext.vprof_decan ~= nil then
					file:write("\n\t\tIterations min :                          "..content.vprev.vprof_decan.ite_min..string.rep(" ", 30)..content.vnext.vprof_decan.ite_min.."\n")
					file:write("\t\tIterations max :                          "..content.vprev.vprof_decan.ite_max..string.rep(" ", 30)..content.vnext.vprof_decan.ite_max.."\n")
					file:write("\t\tIterations avg :                          "..content.vprev.vprof_decan.ite_avg..string.rep(" ", 30)..content.vnext.vprof_decan.ite_avg.."\n")
					if type(content.vprev.vprof_decan.r_l1_min) == "number" then
						vprev = string.format("%.2f", content.vprev.vprof_decan.r_l1_min)
					else
						vprev = content.vprev.vprof_decan.r_l1_min
					end
					if type(content.vnext.vprof_decan.r_l1_min) == "number" then
						vnext = string.format("%.2f", content.vnext.vprof_decan.r_l1_min)
					else
						vnext = content.vnext.vprof_decan.r_l1_min
					end
					file:write("\t\tr_l1_min :                                "..vprev..string.rep(" ", 28)..vnext.."\n")
					if type(content.vprev.vprof_decan.r_l1_max) == "number" then
						vprev = string.format("%.2f", content.vprev.vprof_decan.r_l1_max)
					else
						vprev = content.vprev.vprof_decan.r_l1_max
					end
					if type(content.vnext.vprof_decan.r_l1_max) == "number" then
						vnext = string.format("%.2f", content.vnext.vprof_decan.r_l1_max)
					else
						vnext = content.vnext.vprof_decan.r_l1_max
					end
					file:write("\t\tr_l1_max :                                "..vprev..string.rep(" ", 28)..vnext.."\n")
					if type(content.vprev.vprof_decan.r_l1_med) == "number" then
						vprev = string.format("%.2f", content.vprev.vprof_decan.r_l1_med)
					else
						vprev = content.vprev.vprof_decan.r_l1_med
					end
					if type(content.vnext.vprof_decan.r_l1_med) == "number" then
						vnext = string.format("%.2f", content.vnext.vprof_decan.r_l1_med)
					else
						vnext = content.vnext.vprof_decan.r_l1_med
					end
					file:write("\t\tr_l1_med :                                "..vprev..string.rep(" ", 28)..vnext.."\n")
				else
					file:write("\n\t\tNo DECAN and VPROF results available\n")
				end
				-- Speedup after/before transformations
				vprev = string.format("%.2f", content.vnext.metrics[1]["vec eff ratio all"] / content.vprev.metrics[1]["vec eff ratio all"])
				file:write("\n\t\tSpeedup vector-efficiency ratio (after/before) : "..vprev.."\n")
				vprev = string.format("%.2f", content.vnext.metrics[1]["packed ratio all"] / content.vprev.metrics[1]["packed ratio all"])
				file:write("\t\tSpeedup vectorization ratio (after/before) :\t "..vprev.."\n")
				file:write("\t\t-------------------------------------------------------------------------------\n\n")
			end
			
		end

	end

end