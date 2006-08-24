function onUse(cid, item, frompos, item2, topos)
 	if item2.itemid >= 3805 and item2.itemid <= 3810 or
 		item2.itemid >= 1646 and item2.itemid <= 1661 or
 		item2.itemid >= 1666 and item2.itemid <= 1677 or
		item2.itemid >= 3813 and item2.itemid <= 3820 or
 		item2.itemid >= 1714 and item2.itemid <= 1717 or
 		item2.itemid >= 1724 and item2.itemid <= 1737 or
 		item2.itemid >= 1750 and item2.itemid <= 1753 or
 		item2.itemid >= 2080 and item2.itemid <= 2085 or
 		item2.itemid >= 2116 and item2.itemid <= 2119 or
 		item2.itemid >= 2581 and item2.itemid <= 2588 or
 		item2.itemid == 2095 or item2.itemid == 2098 or 
		item2.itemid == 2101 or item2.itemid == 2104 or 
		item2.itemid == 2105 or item2.itemid == 1775 or 
		item2.itemid == 2602 or item2.itemid == 1619 or 
		item2.itemid == 1738 or item2.itemid == 1739 or item2.itemid == 1741 or 
		item2.itemid == 2064 or item2.itemid >= 1614 and item2.itemid <= 1616 then
 		doRemoveItem(item2.uid,0)
 		doSendMagicEffect(topos,2)
 	else
 		return 0
 	end
 	return 1
 end
