//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Scheduler-Objects for OpenTibia
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include "scheduler.h"

SchedulerTask* makeTask(boost::function1<void, Game*> f)
{
	return new TSchedulerTask(f);
}

SchedulerTask* makeTask(int64_t ticks, boost::function1<void, Game*> f)
{
	SchedulerTask* ret = new TSchedulerTask(f);
	//ret->setEventId(0);
	ret->setTicks(ticks);
	return ret;
}
