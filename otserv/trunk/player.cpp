#include "player.h"

namespace Creatures {
Player::Player(const SOCKET& sock) : client(sock) {

	// we get name and password from the client...
	name = client->getName();
	password = client->getPassword();

	// now we should check both... (TODO)

	// if everything was checked we should load the player... (TODO)
	
} // Player::Player(SOCKET sock) 

Player::~Player() {
} // Player::~Player() 

} // namespace Creature 
