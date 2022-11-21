//server.cpp                                                                    

#include <iostream>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

void error(const char *msg){
  perror(msg);
  exit(1);
}

struct ThreadArgs{
  int clientSock;
  int actualNum;
};

struct Player{
  char charName[100];
  int turn = 9999;
};

Player leaderboard[3];

void *threadMain(void *args){
  //extract socket files descriptor from argument                               
  struct ThreadArgs * threadArgs = (struct ThreadArgs *) args;
  int clientSock = threadArgs -> clientSock;
  int actual = threadArgs -> actualNum;
  delete threadArgs;

  string msgStr;
  char msg[10];
  bool win = false;
  int guess;
  int bytesSent;
  int finalTurn;
  string name;
  char charInput[100];
  int n;

  //recieve name                                                                
  n = read(clientSock, charInput, 100);
  if(n < 0){
    cout << "ERROR in read" << endl;
    exit (-1);
  }

  printf("%s is connected to the server and their number to guess is %d \n",
         charInput, actual);

while(win == false){
    //recieve guess                                                             
    read(clientSock, &guess, sizeof(guess));

    //if the guess is too high                                                  
    if(guess > actual){
      msgStr = "too high\n";
      strcpy(msg,msgStr.c_str());

      bytesSent = send(clientSock, (void *) msg, 10, 0);
      if (bytesSent != 10){
        printf("error sending feedback");
        exit(-1);
      }
    }

    //if guess is too low                                                       
    else if(guess < actual){
      msgStr = "too low \n";
      strcpy(msg, msgStr.c_str());

      bytesSent = send(clientSock, (void *) msg, 10, 0);
      if (bytesSent != 10){
        printf("error sending feedback");
        exit(-1);
      }
    }

    //guess is correct                                                          
    else if(guess == actual){
      printf( "%s guessed correctly \n", charInput);
      msgStr = "correct \n";
      strcpy(msg,msgStr.c_str());

      bytesSent = send(clientSock, (void *) msg, 10, 0);
      if (bytesSent != 10){
        printf("error sending feedback");
        exit(-1);
      }
      win = true;
    }
    msgStr.clear();
  }

  //recieve turn                                                                
  read(clientSock, &finalTurn, sizeof(finalTurn));

  //providing a lock on updating the leaderboard                                
  sem_t mutex;
  sem_init(&mutex, 0, 1);

  sem_wait(&mutex);
  //compare the turn to the existing 3                                          
  for(int i = 0; i < 3; i++){
    if(leaderboard[i].turn > finalTurn){
      if(i == 0){
        leaderboard[2].turn = leaderboard[1].turn;
        strncpy(leaderboard[2].charName, leaderboard[1].charName, 100);
        leaderboard[1].turn = leaderboard[i].turn;
        strncpy(leaderboard[1].charName, leaderboard[i].charName, 100);
      }
      if(i == 1){
        leaderboard[2].turn = leaderboard[1].turn;
        strncpy(leaderboard[2].charName, leaderboard[1].charName, 100);
      }
      if(i == 2){
        leaderboard[3].turn = leaderboard[2].turn;
        strncpy(leaderboard[3].charName, leaderboard[2].charName, 100);
      }
      leaderboard[i].turn = finalTurn;
      strncpy(leaderboard[i].charName, charInput, 100);
      break;
    }
  }
  
   //releasing the lock                                                          
  sem_post(&mutex);

  //send updated leaderboard                                                    
  for(int i = 0; i < 3; i++){
    bytesSent = send(clientSock, (void *) leaderboard[i].charName, 100, 0);
    if (bytesSent != 100){
      printf("error");
      exit(-1);
    }
    n = write(clientSock, &leaderboard[i].turn, sizeof(leaderboard[i].turn));
  }

  //closing the socket                                                          
  pthread_detach(pthread_self());
  close(clientSock);

  return NULL;
}

int main(int argc, char *argv[]){
  int sockfd, newsockfd, portno, n;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;

  if(argc < 2){
    fprintf(stderr, "ERROR, no port provided" );
    exit(-1);
  }

  //creating a socket                                                           
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(sockfd < 0){
    error("ERROR opening socket");
  }

  //assign port to socket                                                       
  bzero((char*) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(sockfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
    error((char*) "ERROR on binding");
  }

  // set socket to listen                                                       
  printf( "listening \n");
  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  int clientSock;
  int status;
  int ID = 0;
  pthread_t threadID;
  unsigned int addrLen;
  string name;
  int bytesLeft;

//accepting multiple clients                                                  
  while(true){
    //accept connection from client                                             
    struct sockaddr_in clientAddr;
    addrLen = sizeof(cli_addr);
    clientSock = accept(sockfd,(struct sockaddr *) &cli_addr, &addrLen);

    if(clientSock < 0){
      printf("ERROR: unable to accept");
      exit(-1);
    }

    //create random number                                                      
    srand(time(0));
    int NUM = rand() % 1000;

    //create and initialize argument struct                                     
    ThreadArgs* threadArgs = new ThreadArgs;
    threadArgs -> clientSock = clientSock;
    threadArgs -> actualNum = NUM;

    // Create client thread                                                     
    threadID = ID;
    status = pthread_create(&threadID, NULL, threadMain, (void*)threadArgs);

    if(status != 0){
      printf("ERROR: unable to create thread \n");
      exit(-1);
    }
    ID++;
  }

  //close                                                                       
  close(newsockfd);
  close(sockfd);
  return 0;
}