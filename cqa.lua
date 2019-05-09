module ("s2s.cqa", package.seeall)

require ("s2s.api")

-- Must be depracated since it has been wrote.
-- Exactract usefull information from CQA CSV results
function s2s:use_cqa_results (args, aproject, options, results)

	local options2 = options

	for i=1, #results do
		local input = results[i].path..results[i].file
		local outputprefix = "maqao_"

    	table.insert(loopsLines,results[i].lineStart)
    	table.insert(loopsLines,results[i].lineEnd  )
    	table.insert(loopsLines,results[i].loop_id  )

    	table.insert(options2,results[i].transformation)

		s2s.start_cqa(input, outputprefix, -1, loopsLines, options2)
		--Clear all arrays
		options2 = options
		loopsLines = {}
	end
end