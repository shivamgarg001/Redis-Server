#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream> //debug

// RESP protocol
// *2\r\n$4\r\n\PING\r\n$4\r\n\TEST\r\n
// *2 -> array has 2 elements PING & TEST
// $4 -> next string has 4 characters

std::vector<std::string> processRespCommand(const std::string& input) {
    std::vector<std::string> tokens;
    if(input.empty())   return tokens;

    // if command doesn't start with '*', fallback to splitting with whitespaces
    if(input[0] != '*'){
        std::istringstream iss(input);
        std::string token;
        while(iss >> token)    tokens.push_back(token);
        return tokens;
    }

    size_t pos = 0;
    // Expect '*' followed by number of elements
    if(input[pos] != '*')   return tokens;
    pos++; // skip '*'

    // crlf -> Carriage Return (\r) and Line Feed (\n)
    size_t crlf = input.find("\r\n", pos);
    if(crlf == std::string::npos)   return tokens;

    int numElements = std::stoi(input.substr(pos, crlf - pos));
    pos = crlf + 2;

    for(int i = 0; i < numElements; i++){
        if(pos >= input.size() || input[pos] != '$')    break; // Format Error
        pos++; //skip '$' sign

        crlf = input.find("\r\n", pos);
        if(crlf == std::string::npos)   break;
        int len = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;

        if(len + pos > input.size())    break;
        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        pos += len + 2; // skip token and CRLF
    }
    return tokens;
}

RedisCommandHandler::RedisCommandHandler() {}

std::string RedisCommandHandler::processCommand(const std::string& commandLine) {
    // Use RESP Parser
    auto tokens = processRespCommand(commandLine);
    if(tokens.empty())  return "[Error] : Empty Commands\r\n";

    // DEBUG
    // std::cout << commandLine << "\n";
    // for (auto& t : tokens){
    //     std::cout << t << "\n";
    // }
    // DEBUGOVER

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;

    // Connect to Database
    RedisDatabase& RDB = RedisDatabase::getInstance();

    // Common Commands
    if(cmd == "PING"){
        response << "+PONG\r\n";
    }
    else if(cmd == "ECHO"){
        if(tokens.size() < 2){
            response << "-Error : ECHO requires a message\r\n";
        }
        else{
            response << "+" << tokens[1] << "\r\n";
        }
    }
    else if(cmd == "FLUSHALL"){
        RDB.flushAll();
        response << "+OK\r\n";
    }
    // ALL THE KEY VALUE OPERATIONS
    else if(cmd == "SET"){
        if(tokens.size() < 3){
            response << "-Error : SET requires Key and Value\r\n";
        }
        else{
            RDB.set(tokens[1], tokens[2]);
            response << "+OK\r\n";
        }
    }
    else if(cmd == "GET"){
        if(tokens.size() < 2){
            response << "-Error : GET requires Key\r\n";
        }
        else{
            std::string value;
            if(RDB.get(tokens[1], value)){
                response << "$" << value.size() << "\r\n" << value << "\r\n";
            }
            else{
                response << "$-1\r\n";
            }
        }
    }
    else if(cmd == "KEYS"){
        std::vector<std::string> allKeys = RDB.keys();
        response << "*" << allKeys.size() << "\r\n";
        for(const auto& key : allKeys){
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    }
    else if(cmd == "TYPE"){
        if(tokens.size() < 2){
            response << "-Error : TYPE requires Key\r\n";
        }
        else{
            response << "+" << RDB.type(tokens[1]) << "\r\n";
        }
    }
    else if(cmd == "DEL" || cmd == "UNLINK"){
        if(tokens.size() < 2){
            response << "-Error : " << cmd << " requires Key\r\n";
        }
        else{
            bool res = RDB.del(tokens[1]);
            response << ":" << (res ? 1 : 0) << "\r\n";
        }
    }
    else if(cmd == "EXPIRE"){
        if(tokens.size() < 3){
            response << "-Error : EXPIRE requires Key and Time (in seconds)\r\n";
        }
        else{
            if(RDB.expire(tokens[1], tokens[2])){
                response << "+OK\r\n";
            }
            else{
                response << "-Error : Key Not Found\r\n";
            }
        }
    }
    else if(cmd == "RENAME"){
        if(tokens.size() < 3){
            response << "-Error : TYPE requires Old Key and New Key Name\r\n";
        }
        else{
            if(RDB.rename(tokens[1], tokens[2])){
                response << "+OK\r\n";
            }
            else{
                response << "-Error : Key Not Found or Rename Failed\r\n";
            }
        }
    }
    else{
        response << "-Error : Unknown Command\r\n";
    }

    return response.str();
}