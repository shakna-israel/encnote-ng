local postcomfile = string.format("%shooks/post.lua", vararg['datadir'])

local f = io.open(postcomfile, "rb")
if f ~= nil then
	dofile(postcomfile)
end
