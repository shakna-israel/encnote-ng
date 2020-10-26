utilities = {}

utilities.in_sequence = function(query, tbl)
	for i=1, #tbl do
		if tbl[i] == query then
			return true
		end
	end
	return false
end

utilities.utf8_substring = function(s,i,j)
	i=utf8.offset(s,i)
	j=utf8.offset(s,j+1)-1
	return string.sub(s,i,j)
end

utilities.split_string = function(str, pattern)
	-- No splits found, return group of 1.
	if string.find(str, pattern) == nil then
		return { str }
	end

	-- Iterate and find matches...
	local t = {}
	local fpat = "(.-)" .. pattern
	local last_end = 1
	local s, e, cap = string.find(str, fpat, 1)
	while s do
		-- Ignore blank...
		if s ~= 1 or cap ~= "" then
			t[#t + 1] = cap
		end
		last_end = e+1
		s, e, cap = string.find(str, fpat, last_end)
	end

	if last_end <= #str then
		cap = string.sub(str, last_end)
		t[#t + 1] = cap
	end

	return t
end

utilities.ordered_pairs = function(tbl)
	local keys = {}
	for k, _ in pairs(tbl) do
		keys[#keys + 1] = k
	end
	table.sort(keys)

	local index = 0
	local count = #keys
	local get_next = function(i)
		return i + 1
	end
	local check = function(i, c)
		return i <= c
	end

	return function()
		index = get_next(index)
		if check(index, count) then
			return keys[index], tbl[keys[index]]
		end
	end
end

utilities.run = function(statement, ...)
	-- Bundle arguments into statement...
	local args = {...}
	for idx, arg in ipairs(args) do
		local x = string.format("%q", tostring(arg))
		x = "'" .. x:sub(2, #x - 1):gsub("'", "\\'") .. "'"
		statement = statement .. ' ' .. x
	end

	-- Hack to get the status code...
	local f = io.popen(statement .. '; echo "-retcode:$?"', 'r')

	local s = f:read("*all")
	f:close()

	local i1, i2, ret = s:find('%-retcode:(%d+)\n$')
	s = s:sub(1, i1-1)

	ret = tonumber(ret) or ret

	return ret, s
end
