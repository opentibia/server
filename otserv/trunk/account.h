


#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__


#include <list>
#include <string>

using namespace std;


class Player;


class Account
{
public:
  Account();
  ~Account();


  bool openAccount(const string &account, const string &givenpassword);
  bool openPlayer(const string &name, const string &givenpassword, Player &player);

  int accType;     // ?
  int premDays;    // Premium days

  string name;
  string password;

  list<std::string> charList;



protected:
  bool parseAccountFile(string filename);
};


#endif  // #ifndef __ACCOUNT_H__