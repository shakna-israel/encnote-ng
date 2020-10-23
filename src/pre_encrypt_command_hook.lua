local precryptcomfile = string.format("%shooks/preencrypt.lua", vararg['datadir'])

local f = io.open(precryptcomfile, "rb")
if f ~= nil then
	dofile(precryptcomfile)
end
