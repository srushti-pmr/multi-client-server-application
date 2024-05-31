#include <stdio.h>    
#include <sys/socket.h>           // Socket functions
#include <arpa/inet.h>            // Internet address functions
#include <unistd.h>               
#include <sys/wait.h>             
#include <errno.h>                
#include <sys/stat.h>    
#include <stdlib.h>               
#include <string.h>               // String manipulation functions
#include <time.h>                 
#include <dirent.h>               


// declaring constant global variables
#define PORT_NO_VAR 8189                 //  main server port number
#define VAR_BSIZE 1024              // The buff size
#define MP_1 7801  // Mirror Server 1 port num
#define MP_2 7802  //  Mirror Server 2  port num     

// Function for date validation for format
int sn_dat_valid(char* dat) 
{
    int sn_year, sn_month, sn_day;  // Year, month, day components

    // Check the format of date YYYY-MM-DD
    if (sscanf(dat, "%d-%d-%d", &sn_year, &sn_month, &sn_day) != 3) 
    {
        return 0;
    }
    if (sn_year < 1 || sn_year > 9999 || sn_month < 1 || sn_month > 12 || sn_day < 1 || sn_day > 31) 
    {
        return 0;
    }
    return 1;
}
 

// Function to list alphabetically dir 
void give_dir_alpha(char *sn_commd) {
    DIR *dir;
    struct dirent **namelist;
    int numbr;

    numbr = scandir(".", &namelist, NULL, alphasort); // curr dir scans nd sort
    if (numbr < 0) {
        perror("scandir error");
    } else {
        for (int i = 0; i < numbr; i++) {
            if (namelist[i]->d_type == DT_DIR) {
               // Skip "." and ".." directories
               if (strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0)
               {
                   continue;
                }
                
                // Skip directories containing dots in their names
               if (strchr(namelist[i]->d_name, '.') != NULL) {
                  continue;
                }

                strcat(sn_commd, namelist[i]->d_name); // Append directory name to buffer
                strcat(sn_commd, "\n");
            
           }
            free(namelist[i]);
        }
        free(namelist);
    }

}


// Function to list directories by creation order
void list_directories_by_creation_order(char *sn_commd) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    struct {
        char *name;
        time_t time;
    } files[1024];

    int count = 0;

    if ((dir = opendir(".")) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
               // Skip "." and ".." directories
               if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
               {
                   continue;
                }

                // Skip directories containing dots in their names
               if (strchr(entry->d_name, '.') != NULL) {
                  continue;
                }

                char path[1024];
                snprintf(path, sizeof(path), "./%s", entry->d_name);
                if (stat(path, &statbuf) == -1) {
                    perror("stat");
                    continue;
                }
                files[count].name = strdup(entry->d_name);
                files[count].time = statbuf.st_ctime;
                count++;
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
    }

    // Sort files by creation time
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (files[j].time > files[j + 1].time) {
                // Swap
                char *temp_name = files[j].name;
                time_t temp_time = files[j].time;
                files[j].name = files[j + 1].name;
                files[j].time = files[j + 1].time;
                files[j + 1].name = temp_name;
                files[j + 1].time = temp_time;
            }
        }
    }

    // Append sorted directories to sn_commd
    for (int i = 0; i < count; i++) {
        strcat(sn_commd, files[i].name);
        strcat(sn_commd, "\n");
    }
}



// clints req handling funx
void crequest(int sockfd) 
{
    char sn_buf[1024] = {0};         // Receive buffer
    char sn_commd[1024] = {0};      // Command buffer
    char sn_tf[1024] = {0};           // Temporary buffer
   printf("here..");
    while (1) 
    {
        memset(sn_buf, 0, sizeof(sn_buf));
        memset(sn_commd, 0, sizeof(sn_commd));
        memcpy(sn_tf, sn_buf, sizeof(sn_buf));

        int sn_rval = read(sockfd, sn_buf, sizeof(sn_buf));
        sn_buf[sn_rval] = '\0';

        char* sn_tok = strtok(sn_buf, " ");   // Parse command
        if (sn_tok == NULL) 
        {
            sprintf(sn_commd, "Syntax is not valid. -- Please Enter again.\n");
        } 


// Execution of dirlist -a and -t 
else if (strcmp(sn_tok, "dirlist") == 0)  
{ 
    char* npsd_option = strtok(NULL, " ");
    if (npsd_option == NULL) {
        sprintf(sn_commd, "\nInvalid syntax for dirlist. Use 'dirlist -a' or 'dirlist -t'.\n");
    } else if (strcmp(npsd_option, "-a") == 0) {
        // Directory listing code goes here for alphabetical order
        sprintf(sn_commd, "\nDirectory listing in alphabetical order:\n");
        // Call the function to list directories alphabetically
        give_dir_alpha(sn_commd);
    } else if (strcmp(npsd_option, "-t") == 0) {
        // Directory listing code goes here for creation order
        sprintf(sn_commd, "\nDirectory listing in order of creation:\n");
        // Call the function to list directories by creation order
         list_directories_by_creation_order(sn_commd);
    } else {
        sprintf(sn_commd, "\nInvalid syntax for dirlist. Use 'dirlist -a' or 'dirlist -t'.\n");
    }
} 


// Execution of filesrch command
else if (strcmp(sn_tok, "w24fn") == 0)  
{ 
    char* sn_fnam = strtok(NULL, " ");
    if (sn_fnam == NULL) {
        sprintf(sn_commd, "\nSyntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        char* h_dir = getenv("HOME");
            if (h_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(sn_commd, "\nHome directory not found\n");
                perror("error");
             //   return 1; // Exit with error
            }
        char sn_pri_path[1024];
        sprintf(sn_pri_path, "%s/", h_dir);
       
        char sn_com_buf[VAR_BSIZE]; 
        sprintf(sn_com_buf, "find %s -name %s -printf \"%%f file name,%%s bytes,%%Tc created time,%%m permission\n\"", sn_pri_path, sn_fnam);

        FILE* fp = popen(sn_com_buf, "r");
        char line[VAR_BSIZE];
        if (fgets(line, VAR_BSIZE, fp) != NULL) 
        {
            sprintf(sn_commd, "%s", line); // File found in root directory
        } 
        else 
        {
            sprintf(sn_commd, "\nFile not found\n");  // prints File not found
        }
        pclose(fp);
    }
}


// w24fz command
else if (strcmp(sn_tok, "w24fz") == 0)  
{ 
    char* npsd_s1_str = strtok(NULL, " ");
    char* npsd_s2_str = strtok(NULL, " ");

    if (npsd_s1_str == NULL || npsd_s2_str == NULL) 
    {
        sprintf(sn_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        int size1 = atoi(npsd_s1_str);
        int size2 = atoi(npsd_s2_str);

        if (size1 <= 0 || size2 <= 0 || size1 > size2) 
        {
            sprintf(sn_commd, "\nPlease enter valid size range.\n");
        } 
        else 
        {
             // Get the home directory path
            char* h_dir = getenv("HOME");
            if (h_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(sn_commd, "\nHome directory not found\n");
                perror("\nError in Home enviroment variable"); 
            }

            char sn_pri_path[1024];
            sprintf(sn_pri_path, "%s/", h_dir);
            char sn_com_buf[VAR_BSIZE]; // Finding files matching the size range
            sprintf(sn_com_buf, "find %s -type f -size +%db -size -%db -print0 | xargs -0 tar -czf temp.tar.gz",
                    sn_pri_path, size1, size2);
            int sn_stat = system(sn_com_buf);

            if (sn_stat == 0)  // Check if the files were found
            {
                sprintf(sn_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file\n");
            } 
            else 
            {
                sprintf(sn_commd, "\nNo file found.\n");
            }
        }
    }
}


// Execution of w24fdb command
else if (strcmp(sn_tok, "w24fdb") == 0)  
{  
    char* npsd_date_str = strtok(NULL, " ");
     
    if (npsd_date_str == NULL) {
        sprintf(sn_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        // Add validation for date format
        if (!sn_dat_valid(npsd_date_str)) {
            sprintf(sn_commd, "\nInvalid date format. Please enter date in YYYY-MM-DD format.\n");
        }
        else 
        {
            // Get the home directory path
            char* h_dir = getenv("HOME");
            if (h_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(sn_commd, "\nHome directory not found\n");
                perror("\nError in Home environment variable\n"); 
               
            }

            char sn_pri_path[1024];
            sprintf(sn_pri_path, "%s/", h_dir);

            char sn_com_buf[VAR_BSIZE]; // Finding files that match the date range
            
            sprintf(sn_com_buf, "find %s -type f -exec sh -c 'test $(stat -c %%Y \"$1\") -le $(date -d \"%s\" +%%s)' _ {} \\; -print0 | xargs -0 tar --ignore-failed-read -czf temp.tar.gz",
                    sn_pri_path, npsd_date_str);
           
            system(sn_com_buf);

            sprintf(sn_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file.\n");
        }
    }
}


 


// Execution of w24fda command
else if (strcmp(sn_tok, "w24fda") == 0)  
{  
    char* npsd_date_str = strtok(NULL, " ");
     
    if (npsd_date_str == NULL) {
        sprintf(sn_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        // Add validation for date format
        if (!sn_dat_valid(npsd_date_str)) {
            sprintf(sn_commd, "\nInvalid date format. Please enter date in YYYY-MM-DD format.\n");
        }
        else 
        {
            // Get the home directory path
            char* h_dir = getenv("HOME");
            if (h_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(sn_commd, "\nHome directory not found\n");
                perror("\nError in Home environment variable\n"); 
                //return 1; // Exit with error
            }

            char sn_pri_path[1024];
            sprintf(sn_pri_path, "%s/", h_dir);

            char sn_com_buf[VAR_BSIZE]; // Finding files that match the date range
            sprintf(sn_com_buf, "find %s -type f -exec sh -c 'test $(stat -c %%Y \"$1\") -ge $(date -d \"%s\" +%%s)' _ {} \\; -print0 | xargs -0 tar --ignore-failed-read -czf temp.tar.gz",
                    sn_pri_path, npsd_date_str);
            system(sn_com_buf);

            sprintf(sn_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file.\n");
        }
    }
}






else if (strcmp(sn_tok, "w24fda") == 0)  
{  
    char* npsd_date_str = strtok(NULL, " ");
     
    if (npsd_date_str == NULL) {
        sprintf(sn_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        // Add validation for date format
        if (!sn_dat_valid(npsd_date_str)) {
            sprintf(sn_commd, "\nInvalid date format. Please enter date in YYYY-MM-DD format.\n");
        }
        else 
        {
            // Get the home directory path
            char* h_dir = getenv("HOME");
            if (h_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(sn_commd, "\nHome directory not found\n");
                perror("\nError in Home environment variable\n"); 
                //return 1; // Exit with error
            }

            char sn_pri_path[1024];
            sprintf(sn_pri_path, "%s/", h_dir);

            char sn_com_buf[VAR_BSIZE]; // Finding files that match the date range
            sprintf(sn_com_buf, "find %s -type f -newermt \"%s\" -print0 > temp_file_list.txt",
                    sn_pri_path, npsd_date_str);
            system(sn_com_buf);

            // Create the tar.gz file
            char tar_command[VAR_BSIZE];
            snprintf(tar_command, sizeof(tar_command), "tar -czf temp.tar.gz -T temp_file_list.txt");
            int ret = system(tar_command);

            // Check if the tar command executed successfully
            if (ret == -1) {
                perror("Error creating tar.gz file");
                sprintf(sn_commd, "\nError creating tar.gz file\n");
            } else {
                sprintf(sn_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file.\n");
            }

            // Clean up: delete the temporary file list
            remove("temp_file_list.txt");
        }
    }
}


else if (strcmp(sn_tok, "w24fda") == 0)  
{  
    char* npsd_date_str = strtok(NULL, " ");
     
    if (npsd_date_str == NULL) {
        sprintf(sn_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        // Add validation for date format
        if (!sn_dat_valid(npsd_date_str)) {
            sprintf(sn_commd, "\nInvalid date format. Please enter date in YYYY-MM-DD format.\n");
        }
        else 
        {
            // Get the home directory path
            char* h_dir = getenv("HOME");
            if (h_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(sn_commd, "\nHome directory not found\n");
                perror("\nError in Home enviroment variable\n"); 
               //return 1; // Exit with error
            }

            char sn_pri_path[1024];

            sprintf(sn_pri_path, "%s/", h_dir);
       

            char sn_com_buf[VAR_BSIZE]; // Finding files that match the date range
            sprintf(sn_com_buf, "find %s -type f ! -newermt \"%s\" -print0 | xargs -0 tar -czf temp.tar.gz",
                    sn_pri_path, npsd_date_str);
            system(sn_com_buf);

            sprintf(sn_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file.\n");
        }
    }
}

       
// Execution of w24ft command
else if (strcmp(sn_tok, "w24ft") == 0)
{ 
    char* sn_ex1 = strtok(NULL, " ");
    char* sn_ex2 = strtok(NULL, " ");
    char* sn_ex3 = strtok(NULL, " ");
    char* sn_ex4 = strtok(NULL, " "); // Additional extension

    if (sn_ex4 != NULL) {
        sprintf(sn_commd, "\nMaximum three file extensions allowed.\n");
        //return; // Exit the function
    } else{
    
    // Get the home directory path
            char* h_dir = getenv("HOME");
            if (h_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(sn_commd, "\nHome directory not found\n");
                perror("\nError in Home enviroment variable");           
     }

    char sn_pri_path[1024];
    sprintf(sn_pri_path, "%s/", h_dir);
    char sn_com_buf[VAR_BSIZE];  // Checking whether any of the specified files are present
    sprintf(sn_com_buf, "find %s -type f \\( ", sn_pri_path);
    if (sn_ex1 != NULL) sprintf(sn_com_buf + strlen(sn_com_buf), "-iname \"*.%s\" -o ", sn_ex1);
    if (sn_ex2 != NULL) sprintf(sn_com_buf + strlen(sn_com_buf), "-iname \"*.%s\" -o ", sn_ex2);
    if (sn_ex3 != NULL) sprintf(sn_com_buf + strlen(sn_com_buf), "-iname \"*.%s\" -o ", sn_ex3);
    sprintf(sn_com_buf + strlen(sn_com_buf), "-false \\) -print0 | xargs -0 tar -czf temp.tar.gz");

    int sn_stat = system(sn_com_buf);

    if (sn_stat == 0)  // Check if the files were found
    {
        sprintf(sn_commd, "\nFiles are retrieved successfully.Please check temp.tar.gz file.\n");
    } 
    else 
    {
        sprintf(sn_commd, "\nNo file found.\n");
    }
  }
}
        //quitc command
        else if (strcmp(sn_tok, "quitc") == 0) 
        {
            sprintf(sn_commd, "\nBye\n");
            break;
        } 
        else 
        {
            sprintf(sn_commd, "\nEntered syntax is not valid. Please Enter again %s.\n",sn_tok);
        }
        send(sockfd, sn_commd, strlen(sn_commd), 0); // Send response to client
    }

    close(sockfd);
    exit(0);
}

// Function to handle connection on mirror server with specified port for mirror1 and mirror2
void sn_hand_mir_conn(int npsd_client_fd, int mirror_port) {
    char npsd_redirecting_msg[1024];
    snprintf(npsd_redirecting_msg, 1024, "%d\n", mirror_port);
    send(npsd_client_fd, npsd_redirecting_msg, strlen(npsd_redirecting_msg), 0);
    close(npsd_client_fd);
}


// Function to handle connection on serverw24
void sn_hand_conn(int npsd_server_fd, int npsd_new_soc){
   pid_t pid = fork();
   if (pid == -1) 
   {
      perror("fork");
      exit(EXIT_FAILURE);
   }
   else if (pid == 0) // Child process
   {  
      close(npsd_server_fd);
      crequest(npsd_new_soc);
   } 
   else // Parent process
   {  
      close(npsd_new_soc);
      while (waitpid(-1, NULL, WNOHANG) > 0); // Clean up zombie processes
   }
} 


// Main function
int main(int argc, char const* argv[]) 
{
    // Intilizing necessary variables
    int npsd_server_fd, npsd_new_soc;
    struct sockaddr_in address;
    int opt = 1;
    int addrrlen = sizeof(address);
    int sn_actv_clnt = 0;

    //SOCKET()
    if ((npsd_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  // Create socket file descriptor
    {
       // Error if socket fails
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(npsd_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) // Attach socket to the port 8080
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_NO_VAR);

    //BIND()
    if (bind(npsd_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //LISTEN()
    if (listen(npsd_server_fd, 3) < 0) 
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //Infinte Loop 
    while (1) 
    {
        if ((npsd_new_soc = accept(npsd_server_fd, (struct sockaddr*)&address, (socklen_t*)&addrrlen)) < 0) 
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //printf("New client has been connected --->. Forking child process\n");
        if (sn_actv_clnt < 3) 
        {
            // Handle connection on serverw24
            printf("Handling connection on serverw24\n");
            sn_hand_conn(npsd_server_fd, npsd_new_soc);
        } 
        else if (sn_actv_clnt >=3 &&  sn_actv_clnt < 6) 
        {
            // Handle connection on mirror1
            printf("Handling connection on mirror1\n");
            sn_hand_mir_conn(npsd_new_soc, MP_1);
        } 
        else if (sn_actv_clnt >=6 && sn_actv_clnt < 9) 
        {
            // Handle connection on mirror2
            printf("Handling connection on mirror2\n");
            sn_hand_mir_conn(npsd_new_soc, MP_1);
        } 
        else 
        {
            // Determine which server to redirect based on the remaining connections count
           if ((sn_actv_clnt - 9) % 3 == 0) {
               // Handle connection on serverw24
               printf("Handling connection on serverw24\n");
               sn_hand_conn(npsd_server_fd, npsd_new_soc);
           } else if ((sn_actv_clnt - 9) % 3 == 1) {
               // Handle connection on mirror2
               printf("Handling connection on mirror2\n");
               sn_hand_mir_conn(npsd_new_soc, MP_1);
           } else {
              // Handle connection on mirror1
              printf("Handling connection on mirror1\n");
              sn_hand_mir_conn(npsd_new_soc, MP_1);
           }
            sn_actv_clnt++; // Increment active clients count
        }
        sn_actv_clnt++; // Count for number of connections
    }

    return 0;
}