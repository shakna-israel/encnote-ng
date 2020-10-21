cli_args = {}

local optionless = {
	"--help",
	"--datadir",
}

local shorts = {
	['-h'] = '--help',
	['-hh'] = '--helpinfo',
	['-k'] = '--keyfile',
	['-d'] = '--datafile',
	['-D'] = '--datadir',
	['-m'] = '--mode',
	['-f'] = '--file',
	['-l'] = '--length',
}

local in_table = function(query, tbl)
	for i=1, #tbl do
		if tbl[i] == query then
			return true
		end
	end
	return false
end

local argparse = function()
	cli_args['progname'] = arg[0]

	local current_flag = '(none)'

	-- Translate short to long
	local narg = {}
	for i=1, #arg do
		for short, long in pairs(shorts) do
			if arg[i] == short then
				narg[i] = long
			end
		end
		if narg[i] == nil then
			narg[i] = arg[i]
		end
	end
	arg = narg

	for i=1, #arg do
		-- Set main flags
		if string.sub(arg[i], 1, 2) == '--' and not in_table(arg[i], optionless) then
			current_flag = string.sub(arg[i], 3)
			
			-- Allow number parsing...
			if tonumber(arg[i + 1]) then
				cli_args[current_flag] = tonumber(arg[i + 1])
			else
				cli_args[current_flag] = arg[i + 1]	
			end

			i = i + 1
		-- Set optionless flags
		elseif string.sub(arg[i], 1, 2) == '--' then
			current_flag = string.sub(arg[i], 3)
			cli_args[current_flag] = true
		end
	end

	-- Default values
	if not arg['length'] then
		arg['length'] = 20
	end
end

argparse()
