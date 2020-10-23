-- set_paths.lua
local version = _VERSION:match("%d+%.%d+")

local extra_path = string.format("%spackages", vararg['datadir'])

package.path = string.format('%s/share/lua/', extra_path) .. version .. string.format('/?.lua;%s/share/lua/', extra_path) .. version .. '/?/init.lua;' .. package.path
package.cpath = string.format('%s/lib/lua/', extra_path) .. version .. '/?.so;' .. package.cpath
