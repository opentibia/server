
#include "thing.h"
#include "cylinder.h"
#include "tile.h"
#include "player.h"

Thing::Thing()
{
	parent = NULL;
	useCount = 0;
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
		return dynamic_cast<Cylinder*>(this);

	if(dynamic_cast<Player*>(this)){
		return dynamic_cast<Cylinder*>(this);
	}

	Cylinder* aux = getParent();

	while(aux->getParent() != NULL){
		if(dynamic_cast<Player*>(aux)){
			return aux;
		}

		aux = aux->getParent();
	}

	return aux;
}

const Cylinder* Thing::getTopParent() const
{
	if(getParent() == NULL)
		return dynamic_cast<const Cylinder*>(this);

	if(dynamic_cast<const Player*>(this)){
		return dynamic_cast<const Cylinder*>(this);
	}

	const Cylinder* aux = getParent();

	while(aux->getParent() != NULL){
		if(dynamic_cast<const Player*>(aux)){
			return aux;
		}

		aux = aux->getParent();
	}

	return aux;
}

Tile* Thing::getTile()
{
	Cylinder* cylinder = getTopParent();

	//get root cylinder
	if(cylinder->getParent())
		cylinder = cylinder->getParent();

	return dynamic_cast<Tile*>(cylinder);
}

const Tile* Thing::getTile() const
{
	const Cylinder* cylinder = getTopParent();

	//get root cylinder
	if(cylinder->getParent())
		cylinder = cylinder->getParent();

	return dynamic_cast<const Tile*>(cylinder);
}

const Position& Thing::getPosition() const
{
	return getTile()->getTilePosition();
}
