#include "uaf-client.h"

UAFClient::UAFClient(UAFServer &server)
{
  uafServer = &server;
}

int UAFClient::menu()
{
  if (masterPassword.empty())
  {
    std::cout << "Avant d'utiliser ce protocole, veuillez saisir le mot de passe maître du portefeuille de clés: ";
    getline(std::cin, masterPassword);
    std::cout << std::endl;
    computeKey();
    updateWallet();
  }

  int menu;
  do
  {
    std::cout << std::endl
              << " --------- Protocole 'UAF': Menu Client" << std::endl
              << std::endl;
    std::cout << "1. Enregistrer un nouveau compte" << std::endl;
    std::cout << "2. Authentification" << std::endl;
    std::cout << "3. Transaction" << std::endl;
    std::cout << "4. Afficher le portefeuille de clé" << std::endl;
    std::cout << "5. Menu précédent" << std::endl;
    std::cout << "6. Quitter" << std::endl;
    std::cout << "Choix : ";
    std::string line;
    getline(std::cin, line);
    std::istringstream ss(line);
    ss >> menu;
    std::cout << std::endl;

    switch (menu)
    {
    case 1:
      registerToServer();
      break;

    case 2:
      authenticate();
      break;

    case 3:
      transaction();
      break;

    case 4:
      displayWallet();
      break;

    case 5:
      return 0; // No effect to main menu
      break;

    case 6:
      return 4; // Option 4 in main menu is exit
      break;
    }
  } while (true);
}

void UAFClient::registerToServer()
{
  std::string username = askUser("Veuillez entrez le username: ");

  std::cout << "Génération des clés privée et public en cours ..." << std::endl;
  // Key generation
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::InvertibleRSAFunction params;
  params.GenerateRandomWithKeySize(rng, 1024);
  CryptoPP::RSA::PrivateKey privateKey(params);
  CryptoPP::RSA::PublicKey publicKey(params);

  // Encode public key with base64
  std::string encodedPublicKey;

  CryptoPP::Base64Encoder pubKeySink(new CryptoPP::StringSink(encodedPublicKey), false);
  publicKey.DEREncode(pubKeySink);
  pubKeySink.MessageEnd();

  // TEST
  std::string encodedPrivateKey;

  CryptoPP::Base64Encoder privKeySink(new CryptoPP::StringSink(encodedPrivateKey), false);
  privateKey.DEREncode(privKeySink);
  privKeySink.MessageEnd();

  std::cout << "Private key (base64)" << encodedPrivateKey << std::endl;

  // Send payload to server
  std::string payload = username + " " + encodedPublicKey;
  payload = display("E1", true, payload);
  std::string res = uafServer->registration(payload);

  // Save private key to wallet if response code is 200 (OK)
  if (res == "200")
  {
    saveToWallet(DEFAULT_SERVER, username, privateKey);
    updateWallet();
  }
}

void UAFClient::authenticate()
{
  std::string username = askUser("Veuillez entrez votre username: ");
  if (wallet[DEFAULT_SERVER].find(username) == wallet[DEFAULT_SERVER].end())
  {
    std::cout << "Username introuvable dans le wallet du server '" << DEFAULT_SERVER << "'." << std::endl;
    return;
  }
  std::string sessionID = generateRandom();
  std::string payload = display("A1", true, sessionID + " " + username);
  std::string res = uafServer->preAuthenticate(payload);
  res = res.substr(res.find(" ") + 1, res.length()); // Remove sessionID
  std::string ns = res.substr(0, res.find(" "));     // Extract ns

  // Sign ns
  std::string signature;
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Signer signer(wallet[DEFAULT_SERVER][username]);
  CryptoPP::StringSource ss1(ns, true, new CryptoPP::SignerFilter(rng, signer, new CryptoPP::StringSink(signature)));

  // Encode sign(ns) using base64
  std::string signatureEncoded;
  CryptoPP::StringSource ss2(
      signature, true, new CryptoPP::Base64Encoder(new CryptoPP::StringSink(signatureEncoded), false));

  payload = display("A3", true, sessionID + " " + signatureEncoded);
  uafServer->authenticate(payload);
}

void UAFClient::transaction()
{
  std::string username = askUser("Veuillez entrez votre username: ");
  if (wallet[DEFAULT_SERVER].find(username) == wallet[DEFAULT_SERVER].end())
  {
    std::cout << "Username introuvable dans le wallet du server '" << DEFAULT_SERVER << "'." << std::endl;
    return;
  }
  std::string command = askUser("Veuillez entrez votre command pour le server '" + DEFAULT_SERVER + "': ");
  std::string sessionID = generateRandom();
  std::string payload = display("A1", true, sessionID + " " + command);
  std::string res = uafServer->preTransaction(payload);
  res = res.substr(res.find(" ") + 1, res.length()); // Remove sessionID
  std::string ns = res.substr(0, res.find(" "));     // Extract ns

  // Sign ns
  std::string signature;
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::SHA1>::Signer signer(wallet[DEFAULT_SERVER][username]);
  CryptoPP::StringSource ss1(command + ns, true, new CryptoPP::SignerFilter(rng, signer, new CryptoPP::StringSink(signature)));

  // Encode sign(ns) using base64
  std::string signatureEncoded;
  CryptoPP::StringSource ss2(
      signature, true, new CryptoPP::Base64Encoder(new CryptoPP::StringSink(signatureEncoded), false));

  payload = display("T3", true, sessionID + " " + signatureEncoded);
  uafServer->authenticate(payload);
}

void UAFClient::saveToWallet(std::string serverName, std::string username, CryptoPP::RSA::PrivateKey privateKey)
{
  // Counter initialization
  byte ctr[CryptoPP::AES::BLOCKSIZE];
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(ctr, sizeof(ctr));

  // Convert private key to std::string
  std::string privateKeyStr;
  CryptoPP::StringSink s1(privateKeyStr);
  privateKey.Save(s1);

  std::string privateKeyEncrypted, privateKeyEncryptedEncoded, ctrEncoded;

  // Encode counter using base64
  CryptoPP::StringSource ss1(ctr, 16, true, new CryptoPP::Base64Encoder(new CryptoPP::StringSink(ctrEncoded), false));

  // Setup AES
  CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption e;
  e.SetKeyWithIV(masterPasswordHash, sizeof(masterPasswordHash), ctr);

  // Crypt private key
  CryptoPP::StringSource ss2(
      privateKeyStr, true,
      new CryptoPP::StreamTransformationFilter(e, new CryptoPP::StringSink(privateKeyEncrypted)));

  // Encode private key using base64
  CryptoPP::StringSource ss3(privateKeyEncrypted, true, new CryptoPP::Base64Encoder(new CryptoPP::StringSink(privateKeyEncryptedEncoded), false));

  // Save Private key to wallet
  std::string line = serverName + ":" + username + ":" + ctrEncoded + "$" + privateKeyEncryptedEncoded;
  std::fstream file;
  file.open(PERSISTENCE_PATH, std::ios_base::app);
  file << line << std::endl;
  file.close();
}

void UAFClient::displayWallet()
{
  for (auto server : wallet)
  {
    std::cout << "Nom du serveur: " << server.first << std::endl
              << std::endl;
    for (auto user : server.second)
    {
      std::cout << "   - Username: " << user.first << std::endl;

      // Encode privateKey using base64
      std::string encodedPrivateKey;
      CryptoPP::Base64Encoder privKeySink(new CryptoPP::StringSink(encodedPrivateKey), false);
      user.second.DEREncode(privKeySink);
      privKeySink.MessageEnd();

      std::cout << "   - PrivateKey (base64): " << encodedPrivateKey << std::endl;
      std::cout << std::endl;
    }
  }
}

void UAFClient::updateWallet()
{
  std::ifstream ifs(PERSISTENCE_PATH);
  std::string line, serverName, delimiter, username, encodedCTR, privateKeyEncoded;

  while (getline(ifs, line))
  {
    // Parse line
    delimiter = ":";
    size_t pos = line.find(delimiter);
    serverName = line.substr(0, pos);
    line.erase(0, pos + delimiter.length());
    pos = line.find(delimiter);
    username = line.substr(0, pos);
    line.erase(0, pos + delimiter.length());
    delimiter = "$";
    pos = line.find(delimiter);
    encodedCTR = line.substr(0, pos);
    line.erase(0, pos + delimiter.length());
    privateKeyEncoded = line;

    // Decode private key
    std::string cipherText;
    CryptoPP::StringSource s1(privateKeyEncoded, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(cipherText)));

    // Decode and create CTR
    std::string ctrString;
    CryptoPP::StringSource s2(encodedCTR, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(ctrString)));
    byte ctr[CryptoPP::AES::BLOCKSIZE];
    for (int i = 0; i < CryptoPP::AES::BLOCKSIZE; i++)
    {
      ctr[i] = ctrString[i];
    }

    // Decrypt privateKey
    CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption d;
    d.SetKeyWithIV(masterPasswordHash, sizeof(masterPasswordHash), ctr);
    std::string decryptedPrivateKey;
    CryptoPP::StringSource s3(cipherText, true,
                              new CryptoPP::StreamTransformationFilter(d,
                                                                       new CryptoPP::StringSink(decryptedPrivateKey)));

    // Create private key
    CryptoPP::StringSource ss(decryptedPrivateKey, true);
    CryptoPP::RSA::PrivateKey privateKey;
    privateKey.Load(ss);

    // Define wallet attribute
    std::map<std::string, CryptoPP::RSA::PrivateKey> *serverWallet = &(wallet[serverName]);
    (*serverWallet)[username] = privateKey;
  }
}

void UAFClient::computeKey()
{
  hash1.CalculateDigest(masterPasswordHash, (const byte *)masterPassword.c_str(), masterPassword.length());
}
