
#include "thing.h"
#include "cylinder.h"
#include "tile.h"

Thing::Thing()
{
	parent = NULL;
	useCount = 0;

  //throwRange = 1;
  //isRemoved = true;
}


Thing::~Thing()
{
	//
}

void Thing::useThing2()
{
	++useCount;
}

void Thing::releaseThing2()
{
	--useCount;

	if(useCount <= 0)
		delete this;
}

Cylinder* Thing::getTopParent()
{
	if(getParent() == NULL)
		return NULL;

	Cylinder* aux = this->getParent();

	while(aux->getParent() != NULL){
		aux = aux->getParent();
	}

	return aux;
}

const Cylinder* Thing::getTopParent() const
{
	if(getParent() == NULL)
		return NULL;

	const Cylinder* aux = this->getParent();

	while(aux->getParent() != NULL){
		aux = aux->getParent();
	}

	return aux;
}

Tile* Thing::getTile()
{
	Cylinder* cylinder = getTopParent();
	return dynamic_cast<Tile*>(cylinder);
	//return NULL;
}

const Tile* Thing::getTile() const
{
	const Cylinder* cylinder = getTopParent();
	return dynamic_cast<const Tile*>(cylinder);
}

const Position& Thing::getPosition() const
{
	return getTile()->getTilePosition(); //getPosition();
}

/*
bool Thing::canMovedTo(const Tile *tile) const
{
	return false;
  //return !tile->isBlocking();
}
*/


