local precomfile = string.format("%shooks/pre.lua", vararg['datadir'])

local f = io.open(precomfile, "rb")
if f ~= nil then
	dofile(precomfile)
end
