#include "tprot65.h"
#include "network.h"
#include "eventscheduler.h"
#include <cmath>

#include <unistd.h> // read

extern EventScheduler es;

namespace Protokoll {
	TProt65::TProt65(const Socket& sock, const string& in) throw(texception) {
		// every data starts with 2 bytes equaling to the length of the data...
		// so firt we test if that value would be higher than the length of the
		// input...
		size_t length = (unsigned char)in[0]+((unsigned char)in[1])*256;
		if (length+2 > in.length()) throw texception("wrong protokoll!",false);

		// the message should the contain 0x01 0x00
		int i=2;
		if (in[i++]!= 0x01) throw texception("wrong protokoll!",false);
		i++;
		if (in[i++]!= 0x00) throw texception("wrong protokoll!",false);

		// maybe client version?
		#if 0
		if (((unsigned char)in[i++])!= (unsigned char)0x8a) {
			cout << "clientversion error? (1)" << endl;
			throw texception("wrong protokoll!",false);
		}
		#endif
		i++;
		if (in[i++]!= 0x02) {
			cout << "clientversion error? (2)" << endl;
			throw texception("wrong protokoll!",false);
		}
		
		// next we have the playernumber.. 4 bytes...
		int pnum = 0;
		for (int j=0; j<4; j++) {
			pnum += (int)pow(0xFF,(j))*((unsigned char)in[i++]);
		}

		// now we encounter password length...
		int passlen = (unsigned char) in[i++];
		passlen += 0xFF * (unsigned char)in[i++];

		if (i+passlen != in.length()) {
			throw texception("wrong protokoll!",false);
		}
		
		passwd = "";
		for (int j=0; j<passlen; j++) {
			passwd += in[i++];
		} // for (int j=0; j<passlen; j++)
			
		cout << passwd  << " " << passwd.length() << endl;

		// seems we have a client v6.5+
		cout << "6.5 client accountnumber: " << pnum << " pwd: " << passwd 
			<< endl;

		throw texception("Protokoll 6.5+ redirected...", true);

	} // TProt65::TProt65(Socket sock, string in) throw(texception) 	

	TProt65::~TProt65() throw() {
		TNetwork::ShutdownClient(psocket);
	} // TProt65::~TProt65() 

	const std::string TProt65::getName() const throw() {
		return name;
	}

	const std::string TProt65::getPassword() const throw() {
		return passwd;
	}

	void TProt65::clread(const Socket& sock) throw() {
		  static const int MAXMSG = 4096;
		  char buffer[MAXMSG];

		  int nbytes = read(sock, buffer, MAXMSG);
		  if (nbytes < 0) { // error
			  cerr << "read" << endl;
			  exit(-1);
		  } else if (nbytes == 0) { // eof (means logout)
			  cerr << "logout" << endl;
			  es.deletesocket(sock);
			  close(sock);
		  } else {  // lesen erfolgreich
			  buffer[nbytes] = 0;
			  cout << "read" << endl;
//			  printf("%s\n", buffer);
		  }
	}

	/*	void TProt::setMap(mapposition newpos) throw(texception) {
	// first we save the new map position...
	our_pos = newpos;
	} // void TProt::setMap(mapposition, Map&) throw(texception)
	 */

}
