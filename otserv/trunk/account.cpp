#include <algorithm>
#include <functional>
#include <iostream>

#include "definitions.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "item.h"
#include "account.h"
#include "player.h"

Account::Account()
{
}


Account::~Account()
{
}


bool Account::openAccount(const std::string &account, const std::string &givenpassword)
{
  std::string filename = "data/accounts/" + account + ".xml";
  std::transform(filename.begin(), filename.end(), filename.begin(), tolower);

  xmlDocPtr doc = xmlParseFile(filename.c_str());

  if (doc)
  {
    xmlNodePtr root, p, tmp;
    root = xmlDocGetRootElement(doc);

    if (xmlStrcmp(root->name,(const xmlChar*) "account"))
    {
      xmlFreeDoc(doc);
      return false;
    }

    p = root->children;

    // perhaps verify name
    const char* pwd = (const char*)xmlGetProp(root, (const xmlChar *)"pass");

    if ((pwd == NULL) || (givenpassword != pwd)) {
      xmlFreeDoc(doc);
      return false;
    }

    password  = pwd;

    accType   = atoi((const char*)xmlGetProp(root, (xmlChar*)"type"));
    premDays  = atoi((const char*)xmlGetProp(root, (xmlChar*)"premDays"));


    // now load in characters.
    while (p)
    {
      const char* str = (char*)p->name;

      if (strcmp(str, "characters") == 0)
      {
        tmp = p->children;
        while(tmp)
        {
          const char* temp_a = (const char*)xmlGetProp(tmp, (xmlChar*)"name");

          if ((strcmp((const char*)tmp->name, "character") == 0) && (temp_a != NULL))
            charList.push_back(temp_a);

          tmp = tmp->next;
        }
      }
      p = p->next;
    }
    xmlFreeDoc(doc);

    // Organize the char list.
    charList.sort();

    return true;
  }

  return false;
}


bool Account::openPlayer(const std::string &name, const std::string &givenpassword, Player &player)
{
  std::string filename="data/players/"+name+".xml";
  std::transform (filename.begin(),filename.end(), filename.begin(), tolower);

  xmlDocPtr doc;
  doc = xmlParseFile(filename.c_str());

  if (doc)
  {
    xmlNodePtr root, tmp, p, slot;
    root=xmlDocGetRootElement(doc);

    if (xmlStrcmp(root->name,(const xmlChar*) "player"))
    {
      std::cout << "Strange. Player-Savefile was no savefile for " << name << std::endl;
    }

    p = root->children;

    const char *account = (const char*)xmlGetProp(root, (const xmlChar *) "account");
    if (!openAccount(account, givenpassword)) {
      xmlFreeDoc(doc);
      return false;
    }

    player.sex=atoi((const char*)xmlGetProp(root, (const xmlChar *) "sex"));
    player.setDirection((Direction)atoi((const char*)xmlGetProp(root, (const xmlChar *) "lookdir")));
    player.experience=atoi((const char*)xmlGetProp(root, (const xmlChar *) "exp"));
    player.level=atoi((const char*)xmlGetProp(root, (const xmlChar *) "level"));
    player.maglevel=atoi((const char*)xmlGetProp(root, (const xmlChar *) "maglevel"));
    player.voc=atoi((const char*)xmlGetProp(root, (const xmlChar *) "voc"));
    player.access=atoi((const char*)xmlGetProp(root, (const xmlChar *) "access"));

    while (p)
    {
      std::string str=(char*)p->name;
      if(str=="mana")
      {
        player.mana=atoi((const char*)xmlGetProp(p, (const xmlChar *) "now"));
        player.manamax=atoi((const char*)xmlGetProp(p, (const xmlChar *) "max"));
        player.manaspent=atoi((const char*)xmlGetProp(p, (const xmlChar *) "spent"));
      }
      else if(str=="health")
      {
        player.health=atoi((const char*)xmlGetProp(p, (const xmlChar *) "now"));
        player.healthmax=atoi((const char*)xmlGetProp(p, (const xmlChar *) "max"));
        player.food=atoi((const char*)xmlGetProp(p, (const xmlChar *) "food"));
      }
      else if(str=="look")
      {
        player.looktype=atoi((const char*)xmlGetProp(p, (const xmlChar *) "type"));
        player.lookhead=atoi((const char*)xmlGetProp(p, (const xmlChar *) "head"));
        player.lookbody=atoi((const char*)xmlGetProp(p, (const xmlChar *) "body"));
        player.looklegs=atoi((const char*)xmlGetProp(p, (const xmlChar *) "legs"));
        player.lookfeet=atoi((const char*)xmlGetProp(p, (const xmlChar *) "feet"));
      }
      else if(str=="skills")
      {
        tmp=p->children;
        while(tmp)
        {
          int s_id, s_lvl, s_tries;
          if (strcmp((const char*)tmp->name, "skill") == 0)
          {
            s_id=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "skillid"));
            s_lvl=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "level"));
            s_tries=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "tries"));
            player.skills[s_id][SKILL_LEVEL]=s_lvl;
            player.skills[s_id][SKILL_TRIES]=s_tries;
          }
          tmp=tmp->next;
        }
      }
      else if(str=="inventory")
      {
        slot=p->children;
        while (slot)
        {
          if (strcmp((const char*)slot->name, "slot") == 0)
          {
            int sl_id = atoi((const char*)xmlGetProp(slot, (const xmlChar *)"slotid"));
            Item* myitem = new Item();
            myitem->unserialize(slot->children);
            player.items[sl_id]=myitem;
          }
          slot=slot->next;
        }
      }
      p=p->next;
    }
    std::cout << "loaded " << filename << std::endl;
    xmlFreeDoc(doc);

    return true;
  }

  return false;
}
