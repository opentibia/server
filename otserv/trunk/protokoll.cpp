#include "protokoll.h"
#include "tprot.h"
#include "network.h"

namespace Protokoll {

	ProtokollP::ProtokollP(const SOCKET& sock) throw(texception) {
		// first we check the socket...
		SOCKET psocket = TNetwork::AcceptPlayer(sock);

		// then we try to read it...
		string buf=TNetwork::ReceiveData(psocket,false);


		// now we need to find out the protokoll...
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
