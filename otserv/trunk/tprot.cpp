#include "tprot.h"
#include "network.h"
#include "eventscheduler.h"

#include <unistd.h> // read

extern EventScheduler es;

namespace Protokoll {
	TProt::TProt(const SOCKET& sock, const string& in) throw(texception) {
		// first we save the socket the player connected on...
		psocket = sock;

		// every data starts with 2 bytes equaling to the length of the data...
		// so firt we test if that value would be higher than the length of the
		// input...
		size_t length = (unsigned char)in[0]+((unsigned char)in[1])*256;
		if (length+2 > in.length()) throw texception("wrong protokoll!",false);

		// the message should the contain 0x00 0x00
		int i=2;
		if (in[i++]!= 0) throw texception("wrong protokoll!",false);
		if (in[i++]!= 0) throw texception("wrong protokoll!",false);

		// next we encounter the selection of journey onward/new game...
		// since new game is only in very old clients and journey onward is in
		// both new and old client versions set...
		// we don't support new game in this version...
		// new players should be generated in another way...
		if (in[i++] != 0x01 || length != 67 || in.length() != 69) 
		throw texception("wrong protokoll!", false);

		// so everything looks still ok...
		// next we have the client os...
		// 0x00 -> linux, 0x01 -> windows
		clientos = in[i++];
		if (clientos != 0x00 && clientos != 0x01) 
			throw texception("wrong protokoll!",false);

		// 0x00 should follow
		if (in[i++] != 0) throw texception("wrong protokoll!",false);

		// then the version should follow...
		version = (unsigned char)in[i++];

		// and an unknown byte (0x02?)
		if (in[i++] != 0x02) throw texception("wrong protokoll!",false);

		// now the name should follow...
		for (;in[i]!='\0' && i < 39; i++)
			name += in[i];

		if (in[i] != '\0' || name == "") 
		throw texception("wrong protokoll!",false);

		// and then the password...
		for (i=39; in[i]!='\0' && i < 69; i++)
			passwd += in[i];

		if (in[i] != '\0') 
		throw texception("wrong protokoll!",false);

	} // TProt::TProt(SOCKET sock, string in) throw(texception) 	

	TProt::~TProt() throw() {
		TNetwork::ShutdownClient(psocket);
	} // TProt::~TProt() 

	const std::string TProt::getName() const throw() {
		return name;
	}

	const std::string TProt::getPassword() const throw() {
		return passwd;
	}

	void TProt::clread(const SOCKET& sock) throw() {
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
