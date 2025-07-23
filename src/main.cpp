#include<iostream>
#include<thread>
#include<chrono>
#include "../include/RedisServer.h"
#include "../include/RedisDatabase.h"

int main(int argc, char* argv[]){
    int port = 6380;
    if(argc >= 2)   port = std::stoi(argv[1]);

    if(RedisDatabase::getInstance().load("dump.my_rdb")){
        std::cout << "Database Loaded From dump.my_rdb\n";
    }
    else{
        std::cout << "No Dump File Found or Load Failed\n";
    }

    RedisServer server(port);

    // BackGround Persistance : Dumping the Databse every 300 seconds (5 Minutes)
    std::thread persistanceThread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(300));
            // Dump Database 
            if(!RedisDatabase::getInstance().dump("dump.my_rdb")){
                std::cerr << "Error Dumping Database\n";
            }
            else{
                std::cout << "Database Dumped to dump.my_rdb\n";
            }
            
        }
    });
    persistanceThread.detach();

    server.run();

    
    return 0;
}