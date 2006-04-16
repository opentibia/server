#include <algorithm>
#include <functional>
#include <iostream>

#include "definitions.h"

#include "account.h"

Account::Account()
{
	accnumber = 0;
}

Account::~Account()
{
	charList.clear();
}
