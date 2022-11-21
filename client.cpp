//client.cpp                                                                    

#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

void error(const char *msg){
  perror(msg);
  exit(0);
}

struct ThreadArgs{
  int clientSock;
  string name;
  int turn;
};

int main(int argc, char *argv[]){
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  if(argc < 3){
    fprintf(stderr, "usage %s hostname port", argv[0]);
    exit(0);
  }
  portno = atoi(argv[2]);

  //create socket                                                               
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(sockfd < 0){
    error ("ERROR opening socket");
  }
  server = gethostbyname(argv[1]);

  if(server == NULL){
    fprintf(stderr, "ERROR, no such host \n");
    exit(0);
  }
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char*) server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);

  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    error((char*)"ERROR connecting");
  }
  
   string name;
  int guess;
  bool win = false;
  int turn = 0;
  char buffer[10];
  string correctStr = "correct \n";
  char charName[100];
  int bytesSent;

  //communication                                                               
  printf("Welcome to Number Guessing Game! \n");
  printf("Please enter your name (limit 100 characters): \n ");
  cin >> name;
  while(name.length() > 100){
    printf("ERROR: too many characters please enter a valid name \n");
    cin >> name;
  }
  
   // send name to the server                                                    
  strcpy(charName, name.c_str());
  bytesSent = send(sockfd, (void *) charName, 100, 0);
  if (bytesSent != 100){
    printf("error");
    exit(-1);
  }
  
    while (win == false){

    printf("turn: %d \n", turn + 1);
    printf("Please enter a guess: ");
    cin >> guess;
    while(cin.fail() || guess < 0 || guess > 999){
      printf("ERROR: please enter a integer from 0-999 \n" );
      cin.clear();
      cin.ignore(1000, '\n');
      cin >> guess;
    }
    n = write(sockfd, &guess, sizeof(guess));
    bzero(buffer, 10);
    n = read(sockfd, buffer, 10);
    if(n < 0){
      cout << "ERROR in read" << endl;
      exit (-1);
    }

    turn ++;
    printf("Result of Guess: %s \n", buffer);
    if(buffer == correctStr){
      cout << "CONGRATS you got it!" << endl;
      // send turn to the server                                                
      n = write(sockfd, &turn, sizeof(turn));
      win = true;
    }
  }

 bzero(buffer,10);

  int firstTurn = 9999;
  int secondTurn = 9999;
  int thirdTurn = 9999;

  char firstName [100];
  char secondName [100];
  char thirdName [100];

  // recieve the leaderboard from the server                                    
  n = read(sockfd, firstName, 100);
  if(n < 0){
      cout << "ERROR in read" << endl;
      exit (-1);
  }

  read(sockfd, &firstTurn, sizeof(firstTurn));
  n = read(sockfd, secondName, 100);
  if(n < 0){
      cout << "ERROR in read" << endl;
      exit (-1);
  }

  read(sockfd, &secondTurn, sizeof(secondTurn));
  n = read(sockfd, thirdName, 100);
  if(n < 0){
      cout << "ERROR in read" << endl;
      exit (-1);
  }

  read(sockfd, &thirdTurn, sizeof(thirdTurn));

  //print leaderboard                                                           
  printf("\nLEADERBOARD \n");
  printf("First Place: %s with turns: %d \n", firstName, firstTurn);
  if(secondTurn != 9999){
    printf("Second Place: %s with turns: %d \n", secondName, secondTurn);
    if(thirdTurn != 9999){
      printf("Third Place: %s with turns: %d \n", thirdName, thirdTurn);
    }
  }
  //close                                                                       
  close(sockfd);
  return 0;
}
