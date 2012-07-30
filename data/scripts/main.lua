-- initialize Lua random numbers
math.randomseed(os.time())

require("otstd/include")
include("otstd/otstd")

include("spells/spells")
include("weapons/weapons")


include_directory("examples")
include_directory("npcs")
