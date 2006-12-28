function onUse(cid, item, frompos, item2, topos)
	if frompos.x == 65535 or getTileHouseInfo(frompos) == 0 then
		doSendMagicEffect(frompos, 3)
		return 1
	end
	if item.itemid == 3901 then
		doTransformItem(item.uid, 1652)
	elseif item.itemid == 3902 then
		doTransformItem(item.uid, 1658)
	elseif item.itemid == 3903 then
		doTransformItem(item.uid, 1666)
	elseif item.itemid == 3904 then
		doTransformItem(item.uid, 1670)
	elseif item.itemid == 3905 then
		doTransformItem(item.uid, 3813)
	elseif item.itemid == 3906 then
		doTransformItem(item.uid, 3817)
	elseif item.itemid == 3907 then
		doTransformItem(item.uid, 3821)
	elseif item.itemid == 3908 then
		doTransformItem(item.uid, 2602)
	elseif item.itemid == 3909 then
		doTransformItem(item.uid, 1614)
	elseif item.itemid == 3910 then
		doTransformItem(item.uid, 1615)
	elseif item.itemid == 3911 then
		doTransformItem(item.uid, 1616)
	elseif item.itemid == 3912 then
		doTransformItem(item.uid, 1619)
	elseif item.itemid == 3913 then
		doTransformItem(item.uid, 3805)
	elseif item.itemid == 3914 then
		doTransformItem(item.uid, 3807)
	elseif item.itemid == 3915 then
		doTransformItem(item.uid, 1738)
	elseif item.itemid == 3917 then
		doTransformItem(item.uid, 2084)
	elseif item.itemid == 3918 then
		doTransformItem(item.uid, 2095)
	elseif item.itemid == 3919 then
		doTransformItem(item.uid, 3809)
	elseif item.itemid == 3920 then
		doTransformItem(item.uid, 3811)
	elseif item.itemid == 3921 then
		doTransformItem(item.uid, 1716)
	elseif item.itemid == 3923 then
		doTransformItem(item.uid, 1774)
	elseif item.itemid == 3926 then
		doTransformItem(item.uid, 2080)
	elseif item.itemid == 3927 then
		doTransformItem(item.uid, 2098)
	elseif item.itemid == 3928 then
		doTransformItem(item.uid, 2104)
	elseif item.itemid == 3929 then
		doTransformItem(item.uid, 2101)
	elseif item.itemid == 3931 then
		doTransformItem(item.uid, 2105)
	elseif item.itemid == 3932 then
        doTransformItem(item.uid, 1724)
	elseif item.itemid == 3933 then
		doTransformItem(item.uid, 1728)
	elseif item.itemid == 3935 then
		doTransformItem(item.uid, 1775)
	elseif item.itemid == 3936 then
		doTransformItem(item.uid, 3832)
	elseif item.itemid == 3937 then
		doTransformItem(item.uid, 2064)
	elseif item.itemid == 3938 then
		doTransformItem(item.uid, 1750)
	else
		return 0
	end
	doSendMagicEffect(topos, 2)
	return 1
end