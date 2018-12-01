#include "communication.h"

std::string Communication::hackerMenu(std::string message)
{
  std::string newMessage;
  std::cout << "--- HACKER ---> Vous avez intercepté le payload '" + message + "'." << std::endl;
  std::cout << "--- HACKER ---> Tapez un payload de remplacement si vous le souhaitez: ";
  getline(std::cin, newMessage);
  if (newMessage.empty())
  {
    return message;
  }
  return newMessage;
}

std::string Communication::display(std::string step, bool clientToServer, std::string message)
{
  std::string way = "S -> C";
  if (clientToServer)
  {
    way = "C -> S";
  }
  std::cout << step << ". " << way << " : " << message << std::endl;
  std::string newMessage = hackerMenu(message);
  if (newMessage != message)
  {
    std::cout << step << "'. " << way << " : " << newMessage << std::endl;
    return newMessage;
  }
  return message;
}

std::string Communication::askUser(std::string question)
{
  std::cout << question;
  std::string input;
  getline(std::cin, input);
  return input;
}

std::string Communication::generateRandom()
{
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::Integer rnd(rng, 16);
  std::stringstream ss;
  ss << rnd;
  return ss.str().substr(0, ss.str().length() - 1);
}

std::string Communication::md5AndEncode(std::string text)
{
  byte digest[CryptoPP::Weak::MD5::DIGESTSIZE];

  CryptoPP::Weak::MD5 hash;
  hash.CalculateDigest(digest, (const byte *)text.c_str(), text.length());

  CryptoPP::Base64Encoder encoder(NULL, false);
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(digest, sizeof(digest));
  encoder.MessageEnd();

  return output;
}
