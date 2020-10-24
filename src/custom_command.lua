local precomfile = string.format("%scommands/%s.lua", vararg['datadir'], vararg['custommode'])

local f = io.open(precomfile, "rb")
if f ~= nil then
	dofile(precomfile)
else
	io.stderr:write(string.format("Command not found: <%s>\n", vararg['custommode']))
end
