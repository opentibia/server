-- initialize Lua random numbers
math.randomseed(os.time())

require("otstd/otstd")

require("spells/spells")
require("weapons/weapons")

require_directory("examples")
require_directory("npcs")
