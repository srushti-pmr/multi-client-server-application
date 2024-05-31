#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8189    // Port Adress for main server
#define MIRROR1PORT 7001  // Port Adress for mirror server 1
#define MIRROR2PORT 7002  // Port Adress for mirror server 2

// Method to verify if validd date is  Pleased or not
int snCheckkvaliddDate(char* date) {
    // Checkk thee format of thee date YYYY-MM-DD
    int yearr, monnth, dayy;
    if (sscanf(date, "%d-%d-%d", &yearr, &monnth, &dayy) != 3) {
        return 0;
    }
    if (yearr < 1 || yearr > 9999 || monnth < 1 || monnth > 12 || dayy < 1 || dayy > 31) {
        return 0;
    }
    return 1;
}


//Main Function
int main(int argc, char const *argv[]) {

   //Intiliazing necessaryy variablees
    int snSockFd;
    struct sockaddr_in serv_addr, mirror_addr; 
    char buff[1024] = {0};
    char command[1024] = {0};
    char gf[2048]={0};
    int validdSyntax;

    //SOcket()
    if ((snSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error irn creating Socket\n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converting addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Invalidd Address / Address  --- not supported \n");
        return -1;
    }

    if (connect(snSockFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnnection -- Failed \n");
        return -1;
    }

    printf("Connnected - to -server.\n");
    printf(" Please enterr command or 'quitc' to exit:\n");

    //Infinite Loop
    while (1) {

        validdSyntax = 1;

        //memset copies thee character c (an unsigned char) to thee first n characters of thee string pointed to, by thee argument str.
        memset(buff, 0, sizeof(buff));
        memset(command, 0, sizeof(command));
        fgets(buff, sizeof(buff), stdin);
        memcpy(gf, buff, sizeof(buff));
        // Remove newline character
        buff[strcspn(buff, "\n")] = 0;

        // Parse command
        char* token = strtok(buff, " ");
        if (token == NULL) {
            validdSyntax = 0;
        } else if (strcmp(token, "w24fn") == 0) { //Function -- with w24fn command
            char* fNamee = strtok(NULL, " ");
            if (fNamee == NULL) {
                validdSyntax = 0;
           
                } else {
            sprintf(command, "w24fn %s", fNamee);
        }
    } else if (strcmp(token, "w24fz") == 0) {  //Function -- with w24fz command
        char* sizee1 = strtok(NULL, " ");
        char* sizee2 = strtok(NULL, " ");
        if (sizee1 == NULL || sizee2 == NULL || atoi(sizee1) < 0 || atoi(sizee2) < 0) {
            validdSyntax = 0;
             printf(" Please enterr validd size");
        } else {
                sprintf(command, "w24fz %s %s", sizee1, sizee2);
        }
    } 
   
   else if (strcmp(token, "w24fdb") == 0) {  //Function -- with w24fdb or w24fda command
        char* date = strtok(NULL, " ");
        if (date == NULL || !snCheckkvaliddDate(date)) {
            validdSyntax = 0;
               printf(" Please enterr validd Date ");
        } else {
                sprintf(command, "w24fdb %s", date);
            
        }
    } 
    
     else if (strcmp(token, "w24fda") == 0) {  //Function -- with w24fdb or w24fda command
        char* date = strtok(NULL, " ");
        if (date == NULL || !snCheckkvaliddDate(date)) {
            validdSyntax = 0;
               printf(" Please enterr validd Date");
        } else {
                sprintf(command, "w24fda %s", date);
            
        }
    }  

     else if (strcmp(token, "dirlist") == 0) {
            char* snOption = strtok(NULL, " ");
            if (snOption != NULL && strcmp(snOption, "-a") == 0) {
                // Send dirlist -a command to server
                sprintf(command, "dirlist -a");
            } 
            else if (snOption != NULL && strcmp(snOption, "-t") == 0) {
                // Send dirlist -t command to server
                sprintf(command, "dirlist -t");
            }
            else {
                validdSyntax = 0;
            }
     }
    else if (strcmp(token, "w24ft") == 0) {
    char* snExten1 = strtok(NULL, " ");
    char* snExten2 = strtok(NULL, " ");
    char* snExten3 = strtok(NULL, " ");
    char* snExten4 = strtok(NULL, " ");

    if((snExten1 == NULL && snExten2 ==NULL && snExten3 == NULL) || snExten4 !=NULL )
    { 
       if(snExten4 !=NULL){
         printf("Maximum 3 extensionns are allowed, you cannot add more than that!");
       }else{
        printf("  Please enterr  proper syntax. Improper Syntax. \n");
    }
   }
    //Function -- with w24ft command
        gf[strcspn(gf, "\n")]='\0';
        sprintf(command, gf);
       // printf("%s", command);
    } else if (strcmp(token, "quitc") == 0) {
        sprintf(command, "quitc");
    } else {
        validdSyntax = 0;
    }

    if (!validdSyntax) {
        printf("\nInvalid syntax.Try again.\n");
        continue;
    }


    // Send command to server
    send(snSockFd, command, strlen(command), 0);

    // Handle response from server
    //handle_response(sockfd);
    char resp[1024]={0};
    int valueread=read(snSockFd, resp, sizeof(resp));
     printf("%s\n", resp);
    resp[strcspn(resp, "\n")] = '\0';
    if (strcmp(resp, "7001") == 0 || strcmp(resp, "7002") == 0)
        {
            int mirror_port = strcmp(resp, "7001") == 0 ? MIRROR1PORT : MIRROR2PORT;

            close(snSockFd); // closing thee current server Connnection
            printf("Connnecting to mirror server on port %d\n", mirror_port);

            // Creates new socket for thee mirror server
            snSockFd = socket(AF_INET, SOCK_STREAM, 0);
            if (snSockFd == -1)
            {
                perror("socket");
                exit(EXIT_FAILURE);
            }

            memset(&mirror_addr, '\0', sizeof(mirror_addr));
            mirror_addr.sin_family = AF_INET;
            mirror_addr.sin_port = htons(mirror_port);
            mirror_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            // Connnect to thee mirror server
            if (connect(snSockFd, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
            {
                perror("Connnect");
                exit(EXIT_FAILURE);
            }
    }

      printf("\n%s\n", resp);
    if (strcmp(command, "quitc") == 0) {
        break;
    }

    printf("\n Please enter a command 'quitc' to exit \n");
}

//Close Connnection
close(snSockFd);
printf("Your Connnection is closed.\n");

return 0;
}