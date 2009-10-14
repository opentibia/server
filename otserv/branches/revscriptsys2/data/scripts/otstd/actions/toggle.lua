otstd.toggle = {}

otstd.toggles = {
		[1479] = {newid = 1480},
		[1480] = {newid = 1479},
		
		[1634] = {newid = 1635},
		[1635] = {newid = 1635},

		[1636] = {newid = 1637},
		[1637] = {newid = 1636},

		[1638] = {newid = 1639},
		[1639] = {newid = 1638},

		[1640] = {newid = 1641},
		[1641] = {newid = 1640},

		[1786] = {newid = 1787},
		[1787] = {newid = 1786},

		[1788] = {newid = 1789},
		[1789] = {newid = 1788},

		[1790] = {newid = 1791},
		[1791] = {newid = 1790},

		[1792] = {newid = 1793},
		[1793] = {newid = 1792},

		[1873] = {newid = 1874},
		[1874] = {newid = 1873},

		[1875] = {newid = 1876},
		[1876] = {newid = 1875},
	
		[1945] = {newid = 1946}, 
		[1946] = {newid = 1945},
	
		[2037] = {newid = 2038},
		[2038] = {newid = 2037},
	
		[2039] = {newid = 2040},
		[2040] = {newid = 2039},
	
		[2058] = {newid = 2059},
		[2059] = {newid = 2058},

		[2060] = {newid = 2061},
		[2061] = {newid = 2060},
	
		[2064] = {newid = 2065},
		[2065] = {newid = 2064},
	
		[2066] = {newid = 2067},
		[2067] = {newid = 2066},
	
		[2068] = {newid = 2069},
		[2069] = {newid = 2068},
	
		[2162] = {newid = 2163},
		[2163] = {newid = 2162},
	
		[2578] = {newid = 2579},
		[2579] = {newid = 2578},
	
		[3697] = {newid = 3698},
		[3698] = {newid = 3697},
	
		[3699] = {newid = 3700},
		[3700] = {newid = 3699},
	
		[3709] = {newid = 3710},
		[3710] = {newid = 3709},

		[3743] = {newid = 3744},
		[3744] = {newid = 3743},
	
		[3945] = {newid = 3946},
		[3946] = {newid = 3945},
	
		[3947] = {newid = 3948},
		[3948] = {newid = 3947},
	
		[3949] = {newid = 3950},
		[3950] = {newid = 3949},
	
		[7058] = {newid = 7059},
		[7059] = {newid = 7058},

		[8684] = {newid = 8685},
		[8685] = {newid = 8684},
	
		[8686] = {newid = 8687},
		[8687] = {newid = 8686},
	
		[8688] = {newid = 8689},
		[8689] = {newid = 8688},
	
		[8690] = {newid = 8691},
		[8691] = {newid = 8690},

		[9575] = {newid = 9576},
		[9576] = {newid = 9575},
	
		[9577] = {newid = 9578},
		[9578] = {newid = 9577},
	
		[9579] = {newid = 9580},
		[9580] = {newid = 9579},

		[9581] = {newid = 9582},	
		[9582] = {newid = 9581},
	
		[9747] = {newid = 9748},
		[9748] = {newid = 9747},
	
		[9749] = {newid = 9750},
		[9750] = {newid = 9749},

		[9825] = {newid = 9826},
		[9826] = {newid = 9825},
	
		[9827] = {newid = 9828},
		[9828] = {newid = 9827},

		[9884] = {newid = 9885},
		[9885] = {newid = 9884},
	
		[9887] = {newid = 9888},
		[9888] = {newid = 9887},
	
		[9889] = {newid = 9890},
		[9890] = {newid = 9889},
	
		[9889] = {newid = 9890},
		[9890] = {newid = 9889},

		[9892] = {newid = 9893},
		[9893] = {newid = 9892},
	
		[9895] = {newid = 9896},
		[9896] = {newid = 9895},

		[9898] = {newid = 9899},
		[9899] = {newid = 9898},
	
		[9901] = {newid = 9902},
		[9902] = {newid = 9901},
	
		[9904] = {newid = 9905},
		[9905] = {newid = 9904}
	}

function otstd.toggle.handler(event)
	local item = event.item
	item:setItemID(event.toggle.newid)
end

function otstd.toggle.registerHandlers()
	for id, data in pairs(otstd.toggles) do
		if data.listener then
			stopListener(data.listener)
		end

		function lamba_callback(event)
			event.toggle = data
			otstd.toggle.handler(event)
		end
		data.listener =
			registerOnUseItem("itemid", id, lamba_callback)
	end
end

otstd.toggle.registerHandlers()
