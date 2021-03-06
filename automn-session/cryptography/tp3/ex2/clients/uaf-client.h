#ifndef DEF_UAF_CLIENT
#define DEF_UAF_CLIENT
 
#include <iostream>
#include <limits>
#include <string>
#include <map>
#include <cryptopp/modes.h>
#include <cryptopp/rsa.h>
#include <cryptopp/pssr.h>
#include "../servers/uaf-server.h"
#include "../communication/communication.h"
 
class UAFClient: public Communication
{
    public:
        UAFClient(UAFServer &server);
        int menu();

    private:
        UAFServer *uafServer;
        std::string masterPassword;
        byte masterPasswordHash[ CryptoPP::Weak::MD5::DIGESTSIZE ];
        const std::string PERSISTENCE_PATH = "persistence/uaf.client.wallet";
        const std::string DEFAULT_SERVER = "Server 1";
        std::map<std::string, std::map<std::string, CryptoPP::RSA::PrivateKey>> wallet;
        std::string sessionID;
        std::string currentUsername;

        void registerToServer();
        void authenticate();
        void transaction();
        void updateWallet();
        void computeKey();
        void displayWallet();
        void removeSession();        
        void saveToWallet(std::string serverName, std::string username, CryptoPP::RSA::PrivateKey privateKey);
};
 
#endif
