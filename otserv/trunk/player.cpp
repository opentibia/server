#include "player.h"
#include <iostream>

namespace Creatures {
Player::Player(const Socket& sock) : client(sock) {

	// we get name and password from the client...
	std::cout << (name = client->getName()) << std::endl;
	std::cout << (password = client->getPassword()) << std::endl;

	// now we should check both... (TODO)

	// if everything was checked we should load the player... (TODO)
	
} // Player::Player(Socket sock) 

Player::~Player() {
} // Player::~Player() 

} // namespace Creature 
