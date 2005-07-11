#include "scheduler.h"


SchedulerTask* makeTask(boost::function1<void, Game*> f) {
	return new TSchedulerTask(f);
}

SchedulerTask* makeTask(__int64 ticks, boost::function1<void, Game*> f) {
	SchedulerTask* ret = new TSchedulerTask(f);
	ret->setTicks(ticks);
	return ret;
}
