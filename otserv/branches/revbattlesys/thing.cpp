
#include "thing.h"
#include "cylinder.h"
#include "tile.h"
#include "creature.h"
#include "item.h"
#include "player.h"

Thing::Thing()
{
	parent = NULL;
	useCount = 0;
}


Thing::~Thing()
{
	//
	//std::cout << "thing destructor " << this << std::endl;
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
	//tile
	if(getParent() == NULL)
		return dynamic_cast<Cylinder*>(this);

	Cylinder* aux = getParent();
	Cylinder* prevaux = dynamic_cast<Cylinder*>(this);

	while(aux->getParent() != NULL){
		prevaux = aux;
		aux = aux->getParent();
	}

	if(dynamic_cast<Cylinder*>(prevaux)){
		return prevaux;
	}

	return aux;
}

const Cylinder* Thing::getTopParent() const
{
	//tile
	if(getParent() == NULL)
		return dynamic_cast<const Cylinder*>(this);

	const Cylinder* aux = getParent();
	const Cylinder* prevaux = dynamic_cast<const Cylinder*>(this);

	while(aux->getParent() != NULL){
		prevaux = aux;
		aux = aux->getParent();
	}

	if(dynamic_cast<const Cylinder*>(prevaux)){
		return prevaux;
	}

	return aux;
}

Tile* Thing::getTile()
{
	Cylinder* cylinder = getTopParent();

	#ifdef __DEBUG__MOVESYS__
	if(!cylinder){
		std::cout << "Failure: [Thing::getTile()],  NULL tile" << std::endl;
		DEBUG_REPORT
		return &(Tile::null_tile);
	}
	#endif

	//get root cylinder
	if(cylinder->getParent())
		cylinder = cylinder->getParent();

	return dynamic_cast<Tile*>(cylinder);
}

const Tile* Thing::getTile() const
{
	const Cylinder* cylinder = getTopParent();

	#ifdef __DEBUG__MOVESYS__
	if(!cylinder){
		std::cout << "Failure: [Thing::getTile() const],  NULL tile" << std::endl;
		DEBUG_REPORT
		return &(Tile::null_tile);
	}
	#endif

	//get root cylinder
	if(cylinder->getParent())
		cylinder = cylinder->getParent();

	return dynamic_cast<const Tile*>(cylinder);
}

const Position& Thing::getPosition() const
{
	const Tile* tile = getTile();
	if(tile){
		return tile->getTilePosition();
	}
	else{
		#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Thing::getPosition],  NULL tile" << std::endl;
		DEBUG_REPORT
		#endif
		return Tile::null_tile.getTilePosition();
	}
}

bool Thing::isRemoved() const
{
	if(parent == NULL)
		return true;

	const Cylinder* aux = getParent();
	if(aux->isRemoved())
		return true;

	return false;
}
