#include "protokoll.h"
#include "tprot.h"	// client <= 6.4 and 6.5+ clients after redirect
#include "tprot65.h" // v6.5+ clients
#include "network.h"

namespace Protokoll {

	Protokoll::Protokoll() throw() : cread(*this) { };

	ProtokollP::ProtokollP(const Socket& psocket) throw(texception) {

		// we try to read from our socket...
		string buf=TNetwork::ReceiveData(psocket,false);

		// now we need to find out the protokoll...
		try {
			// if it's the tibia protokoll 6.5+ the client get's redirected
			// back to the server for the second login step...
			// and an exception get's thrown...
			prot = new TProt65(psocket, buf);
				cout << "WRONG" << endl;
				TNetwork::ShutdownClient(psocket);
				throw texception("TProt 6.5",false);
		}
		catch (texception e) {
			// if it's critical it's the 6.5+ protokoll...
			if (e.isCritical()) {
				TNetwork::ShutdownClient(psocket);
				throw texception(e.toString(),false);
			} // if (e.isCritical()) 
		}

		try {
			// if the protokoll is the tibia protokoll there will be no
			// exception, else there will be one and we need to check the next
			// protokoll...
			prot = new TProt(psocket, buf);
			return;
		}
		catch (texception e) {
			// so it's not the tibia protokoll...
			if (e.isCritical()) {
				TNetwork::ShutdownClient(psocket);
				throw;
			} // if (e.isCritical()) 
		}

		// other protokolls insert here...

		// since it's none of the protokolls above we throw an exception...
		TNetwork::ShutdownClient(psocket);
		throw texception("no protokoll found!", false);
	}

	ProtokollP::~ProtokollP() throw() {
		delete prot;
	}

} // namespace Protokoll 
