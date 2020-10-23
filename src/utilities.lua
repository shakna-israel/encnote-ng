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
