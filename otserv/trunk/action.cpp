#include "action.h"
#include "eventscheduler.h"

Action::Action() : receiveTime(EventScheduler::getNow()) {
// Set default action type to none.
		type=ACTION_NONE;
//  No count.
		count=0;
//  Default stack pos: 0.
		stack=0;
	}
