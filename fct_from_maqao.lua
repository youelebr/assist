module ("s2s.fct_from_maqao", package.seeall)

local function get_indent(depth)
	local i;
	local indent = "";

	for i=1,depth,1
	do 
		indent = indent.."\t";
	end
	return indent;
end	


local function __table_tostring(name,object,indent,processed)

	if(type(object) == "nil") then
		return;
	end

	if(type(object) == "table" and processed[object]) then
		print(get_indent(indent).."[\""..name.."\"] = Ref: "..tostring(object)..";");
		return;
	--Special case to avoid cycling on the special _M entry : metatable entry points on the associated object  
	elseif(type(object) == "table" and name == "_M") then
		print(get_indent(indent).."[\"Ref:_M\"];");
		return;
	--Special case to avoid cycling on the special _G entry : global Lua registry object  		
	elseif(type(object) == "table" and name == "_G") then
		print(get_indent(indent).."[\"Ref:_G\"];");
		return;		
	else
		processed[object] = true;
	end
	if(type(object) == "number") then
		if(type(name) == "number") then
			print(get_indent(indent).."["..name.."] = "..object..";");
		else
			print(get_indent(indent).."[\""..tostring(name).."\"] = "..object..";");
		end
	elseif(name == nil) then
			print(get_indent(indent).."[\"nil\"] = nil;");
	elseif(type(object) == "boolean") then
		if(type(name) == "number") then
			print(get_indent(indent).."["..name.."] = "..tostring(object)..";");
		elseif(type(name) == "userdata") then
			print(get_indent(indent).."[\""..tostring(name).."\"] = "..tostring(object)..";");
                else
			print(get_indent(indent).."[\""..name.."\"] = "..tostring(object)..";");
		end		
	elseif(type(object) == "userdata" or type(object) == "function") then
		if(type(name) == "number") then
			print(get_indent(indent).."["..name.."] = "..tostring(object)..";");
		elseif(type(name) == "userdata") then
			print(get_indent(indent).."[\""..tostring(name).."\"] = "..tostring(object)..";");
                else
			print(get_indent(indent).."[\""..name.."\"] = "..tostring(object)..";");
		end
	elseif(type(object) == "string") then
		if(type(name) == "number") then
			print(get_indent(indent).."["..name,"] = "..string.format("%q",object)..";");
		else
			print(get_indent(indent).."[\""..name.."\"] = "..string.format("%q",object)..";");
		end
	elseif(type(object) == "table") then
		if(type(name) == "number") then
			print(get_indent(indent).."["..name.."] = ".."\n"..get_indent(indent).."{");
		elseif(type(name) == "userdata" or type(name) == "function" or type(name) == "table") then
			print(get_indent(indent).."[\""..tostring(name).."\"] = ".."\n"..get_indent(indent).."{");
		else
			print(get_indent(indent).."[\""..name.."\"] = ".."\n"..get_indent(indent).."{");
		end

		indent = indent + 1;
		for k,v in pairs(object)
		do
			__table_tostring(k,v,indent,processed);
		end
		indent = indent - 1;
		print(get_indent(indent).."};\n");	
	end
end


-- When intergrated in MAQAO, needs to be erased and replaced by Utils:load_CSV
function s2s:load_CSV (csv_name, delim)
   local csv    = nil
   local header = {}
   local file   = nil
   local i_line = 1
   
   -- Check the file exists
   if fs.exists (csv_name) == false then
      return csv, -1
   end
   
   -- Check the delimiter
   if type(delim) ~= "string" then
      return csv, -3
   end
   
   -- Open the file
   file = io.open (csv_name, "r")
   if file == nil then
      return csv, -2
   end
   
   -- Parse the file
   csv = {}
   for line in file:lines() do
      
      -- Get the header
      if i_line == 1 then
         local sub_line = String:split (line, delim)
         for _, elem in ipairs (sub_line) do
            table.insert (header, elem)
         end
      -- Use the header to parse lines
      else
         local sub_line = String:split (line, delim)
         local csv_line = {}
         for i, elem in ipairs (sub_line) do
            if tonumber (elem) ~= nil then
               csv_line[header[i]] = tonumber (elem)
            else
               csv_line[header[i]] = elem
            end
         end
         table.insert (csv, csv_line)
      end
      i_line = i_line + 1
   end
   
   -- Close the file
   file:close ()
   
   return csv, 0
end

-- When intergrated in MAQAO, needs to be erased and replaced by Table:tostring
function s2s:table_tostring(table)
	local table_toprint = nil;
	local name = "Table";
	local processed = {};
	
	if(table ~= nil) then
		table_toprint = table;
	else
		table_toprint = self;
	end

	if(type(table_toprint) ~= "table") then
		print("WARNING : Given object to print is not a table: instead got "..type(table_toprint));
		return nil;
	end

	print(name.." = ".."\n{");
	for k,v in pairs(table_toprint)
	do
		__table_tostring(k,v,1,processed);
	end
	print("};");		

	processed = nil;
	
	return "";--Return a string (empty) to be strict 
end