#include "player.h"

namespace Creatures {
Player::Player(const Socket& sock) : client(sock) {

	// we get name and password from the client...
	cout << (name = client->getName()) << endl;
	cout << (password = client->getPassword()) << endl;

	// now we should check both... (TODO)

	// if everything was checked we should load the player... (TODO)
	
} // Player::Player(Socket sock) 

Player::~Player() {
} // Player::~Player() 

} // namespace Creature 
