//+-----------------------------------------------------------------+
//| Tibia Server, 2004 - Gerald Monaco
//+-----------------------------------------------------------------+
//| Filename: battleSystem.CPP
//| Desc: Code for battleSystem.H class.
//+-----------------------------------------------------------------+

//+-----------------------------------------------------------------+
//|  Includes and Defines
//+-----------------------------------------------------------------+
#include "battleSystem.h"
//+-----------------------------------------------------------------+
//| End of ( Includes and Defines )
//+-----------------------------------------------------------------+

//+-----------------------------------------------------------------+
//| Code follows
//+-----------------------------------------------------------------+

//  This should return whether or not we can attack someone.
bool battle_StartAttack(Creature* pAttacker, Map& map)
{
return true;
}

void battle_Attack(int use, int skill, position pos, Creature* pAttackingr, Map& map) {
		  Action* a= new Action;

		  Action* c= new Action;

		  int damage;
		  int chance=(rand()%4);
		  //  Xbow.
		  if (use==1) {
					 std::cout << "use = 1" << std::endl;
					 damage=(rand()%skill+rand()%50);
					 if (damage==0)
					 {
								//  Code this?
								damage=1;
					 }

					 if (chance==1)
					 {
								std::cout << "miss" << std::endl;
								// We missed. :(
								// FOR BOLTS
								a->type=ACTION_ANI_ITEM;
								a->id=1;
								a->pos1=pos;
								a->pos2=pAttackingr->getPosition();
								a->count=180;
								a->stack=0;
								//
								c->type=ACTION_ANIMATION;
								c->id=2;
								c->pos1=pAttackingr->getPosition();
								map.distributeAction(pAttackingr->getPosition(),c);

								map.distributeAction(a->pos2,a); 
					 }
					 else {
								if (chance==2) {
										  std::cout << "spark" << std::endl;
										  //  Spark.
										  // FOR BOLTS
										  a->type=ACTION_ANI_ITEM;
										  a->id=1;
										  a->pos1=pos;
										  a->pos2=pAttackingr->getPosition();
										  a->count=180;
										  a->stack=0;
										  //
										  c->type=ACTION_ANIMATION;
										  c->id=3;
										  c->pos1=pAttackingr->getPosition();
										  map.distributeAction(pAttackingr->getPosition(),c);

										  map.distributeAction(a->pos2,a); 
								} else {
										  std::cout << "hit" << std::endl;
										  //  WE HIT!!!!!111
										  a->type=ACTION_ANI_ITEM;
										  a->id=1;
										  a->pos1=pos;
										  a->pos2=pAttackingr->getPosition();
										  a->count=180;
										  a->stack=damage;

										  //  Drain health now.
										  map.distributeAction(a->pos2,a); 

										  map.drainHP(pAttackingr->getPosition(), damage);

								}
					 }
		  }

}

void battle_End()
{

}
