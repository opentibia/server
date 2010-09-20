--[[
-- default weapons
require("weapons/defaults")

-- scripted weapons
require("weapons/scripted/explosive_arrow")
require("weapons/scripted/poison_arrow")
require("weapons/scripted/viper_star")
]]


registerOnUseWeapon("all", otstd.onUseWeapon)
