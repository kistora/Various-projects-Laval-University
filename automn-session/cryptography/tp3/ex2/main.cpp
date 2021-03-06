// To compile: `g++ main.cpp communication/*.cpp servers/*.cpp clients/*.cpp -o main.out -lcryptopp`
// To execute: `./main.out`

#include <string>
#include <iostream>
#include <limits>
#include "servers/password-server.h"
#include "clients/password-client.h"
#include "servers/http-digest-server.h"
#include "clients/http-digest-client.h"
#include "servers/uaf-server.h"
#include "clients/uaf-client.h"

// Client & server class instanciation
PasswordServer passwordServer;
PasswordClient passwordClient(passwordServer);
HTTPDigestServer httpDigestServer;
HTTPDigestClient httpDigestClient(httpDigestServer);
UAFServer uafServer;
UAFClient uafClient(uafServer);

// Display the main menu. Call client menu according to user choice.
void mainMenu()
{
  int menu;
  do
  {
    std::cout << std::endl
              << " ----------------------- MENU -----------------------" << std::endl
              << std::endl;
    std::cout << "1. Mot de passe" << std::endl;
    std::cout << "2. HTTP-Digest" << std::endl;
    std::cout << "3. UAF" << std::endl;
    std::cout << "4. Quitter" << std::endl;
    std::cout << "Choix : ";
    std::string input;
    getline(std::cin, input);
    while (input.empty())
    {
      std::cout << "Chaine vide, ré-essayez: ";
      getline(std::cin, input);
    }
    std::istringstream ss(input);
    ss >> menu;
    std::cout << std::endl;
    std::cout << std::endl;

    switch (menu)
    {
    case 1:
      menu = passwordClient.menu();
      break;

    case 2:
      menu = httpDigestClient.menu();
      break;

    case 3:
      menu = uafClient.menu();
      break;
    }
  } while (menu != 4);
}

void credit()
{
  std::cout << R"(   
__________                __          __         .__   
\______   \_______  _____/  |_  ____ |  | ______ |  |  
 |     ___/\_  __ \/  _ \   __\/  _ \|  |/ /  _ \|  |  
 |    |     |  | \(  <_> )  | (  <_> )    <  <_> )  |__
 |____|     |__|   \____/|__|  \____/|__|_ \____/|____/
                                          \/
                                Coded by Corentin Giraud
    )" << std::endl;
}

int main(int argc, char *argv[])
{
  credit();

  mainMenu();

  return 0;
}
