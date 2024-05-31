#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <stdbool.h>


#define PORT 7802 //Port number 
#define BUFSIZE 1024 //buffer


// Function to check if a date is valid
int npsd_date_input_valid(char* date) 
{
    int npsd_year, npsd_month, npsd_day;  // Year, month, day components

    // Check the format of date YYYY-MM-DD
    if (sscanf(date, "%d-%d-%d", &npsd_year, &npsd_month, &npsd_day) != 3) 
    {
        return 0;
    }
    if (npsd_year < 1 || npsd_year > 9999 || npsd_month < 1 || npsd_month > 12 || npsd_day < 1 || npsd_day > 31) 
    {
        return 0;
    }
    return 1;
}
 
    
void give_dir_alpha(char *npsd_commd) {
    DIR *dir;
    struct dirent **namelist;
    int numbr;

    // Get the full path to the home directory
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf(stderr, "Failed to get home directory path\n");
        exit(1);
    }

    // Construct the command using the full path to the home directory
    char command[256]; // Adjust size as needed
   // snprintf(command, sizeof(command), "ls -d %s | xargs -n 1 basename | sort", home_dir);
       snprintf(command, sizeof(command), "ls -d %s */ | xargs -n 1 basename | sort", home_dir);    

    // Open pipe to execute command and read its output
    FILE *pipe_fp = popen(command, "r");
    if (pipe_fp == NULL) {
        perror("popen");
        exit(1);
    }

    // Read the output line by line and append to the buffer
    char line[256]; // Assuming maximum directory name length of 255
    while (fgets(line, sizeof(line), pipe_fp) != NULL) {
        // Remove trailing newline character
        line[strcspn(line, "\n")] = '\0';
        strcat(npsd_commd, line); // Append directory name to buffer
        strcat(npsd_commd, "\n");
    }

    // Close the pipe
    pclose(pipe_fp);
}


// Function to list directories by creation order
void list_directories_by_creation_order(char *npsd_commd) {
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

    // Append sorted directories to npsd_commd
    for (int i = 0; i < count; i++) {
        strcat(npsd_commd, files[i].name);
        strcat(npsd_commd, "\n");
    }
}



// Function to process client requests
void crequest(int sockfd) 
{
    char npsd_buf[1024] = {0};         // Receive buffer
    char npsd_commd[1024] = {0};      // Command buffer
    char npsd_tf[1024] = {0};           // Temporary buffer

    while (1) 
    {
        memset(npsd_buf, 0, sizeof(npsd_buf));
        memset(npsd_commd, 0, sizeof(npsd_commd));
        memcpy(npsd_tf, npsd_buf, sizeof(npsd_buf));

        int npsd_rvalue = read(sockfd, npsd_buf, sizeof(npsd_buf));
        npsd_buf[npsd_rvalue] = '\0';

        char* npsd_token = strtok(npsd_buf, " ");   // Parse command
        if (npsd_token == NULL) 
        {
            sprintf(npsd_commd, "Syntax is not valid. -- Please Enter again.\n");
        } 


// Execution of dirlist -a and -t 
else if (strcmp(npsd_token, "dirlist") == 0)  
{ 
    char* npsd_option = strtok(NULL, " ");
    if (npsd_option == NULL) {
        sprintf(npsd_commd, "\nInvalid syntax for dirlist. Use 'dirlist -a' or 'dirlist -t'.\n");
    } else if (strcmp(npsd_option, "-a") == 0) {
        // Directory listing code goes here for alphabetical order
        sprintf(npsd_commd, "\nDirectory listing in alphabetical order:\n");
        // Call the function to list directories alphabetically
       // list_directories_alphabetically(npsd_commd);
       give_dir_alpha(npsd_commd);
    } else if (strcmp(npsd_option, "-t") == 0) {
        // Directory listing code goes here for creation order
        sprintf(npsd_commd, "\nDirectory listing in order of creation:\n");
        // Call the function to list directories by creation order
         list_directories_by_creation_order(npsd_commd);
    } else {
        sprintf(npsd_commd, "\nInvalid syntax for dirlist. Use 'dirlist -a' or 'dirlist -t'.\n");
    }
} 


// Execution of filesrch command
else if (strcmp(npsd_token, "w24fn") == 0)  
{ 
    char* npsd_flname = strtok(NULL, " ");
    if (npsd_flname == NULL) {
        sprintf(npsd_commd, "\nSyntax is not valid. -- Please Enter again.\n");
    } 
    else 
    {
        char* home_dir = getenv("HOME");
            if (home_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(npsd_commd, "\nHome directory not found\n");
                perror("error");
             //   return 1; // Exit with error
            }
        char npsd_primary_path[1024];
        sprintf(npsd_primary_path, "%s/", home_dir);
        //sprintf(npsd_primary_path, "/home/");  // Search file in the root directory
       
        char npsd_commd_buf[BUFSIZE]; 
       // sprintf(npsd_commd_buf, "find %s -name %s -printf \"%%f file name,%%s bytes,%%Tc created time,%%m permission\n\"", npsd_primary_path, npsd_flname);
       sprintf(npsd_commd_buf, "find %s -name %s -printf \"File name: %%f ,Bytes: %%s,Created time: %%Tc, Permission: %%m\n\"", npsd_primary_path, npsd_flname);

        FILE* fp = popen(npsd_commd_buf, "r");
        char line[BUFSIZE];
        if (fgets(line, BUFSIZE, fp) != NULL) 
        {
            sprintf(npsd_commd, "%s", line); // File found in root directory
        } 
        else 
        {
            sprintf(npsd_commd, "\nFile not found\n");  // prints File not found
        }
        pclose(fp);
    }
}


// w24fz command
else if (strcmp(npsd_token, "w24fz") == 0)  
{ 
    char* npsd_s1_str = strtok(NULL, " ");
    char* npsd_s2_str = strtok(NULL, " ");

    if (npsd_s1_str == NULL || npsd_s2_str == NULL) 
    {
        sprintf(npsd_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        int size1 = atoi(npsd_s1_str);
        int size2 = atoi(npsd_s2_str);

        if (size1 <= 0 || size2 <= 0 || size1 > size2) 
        {
            sprintf(npsd_commd, "\nPlease enter valid size range.\n");
        } 
        else 
        {
             // Get the home directory path
            char* home_dir = getenv("HOME");
            if (home_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(npsd_commd, "\nHome directory not found\n");
                perror("\nError in Home enviroment variable"); 
            }

            char npsd_primary_path[1024];
            sprintf(npsd_primary_path, "%s/", home_dir);
            char npsd_commd_buf[BUFSIZE]; // Finding files matching the size range
            sprintf(npsd_commd_buf, "find %s -type f -size +%db -size -%db -print0 | xargs -0 tar -czf temp.tar.gz",
                    npsd_primary_path, size1, size2);
            int npsd_status = system(npsd_commd_buf);

            if (npsd_status == 0)  // Check if the files were found
            {
                sprintf(npsd_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file\n");
            } 
            else 
            {
                sprintf(npsd_commd, "\nNo file found.\n");
            }
        }
    }
}


// Execution of w24fdb command
else if (strcmp(npsd_token, "w24fdb") == 0)  
{  
    char* npsd_date_str = strtok(NULL, " ");
     
    if (npsd_date_str == NULL) {
        sprintf(npsd_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        // Add validation for date format
        if (!npsd_date_input_valid(npsd_date_str)) {
            sprintf(npsd_commd, "\nInvalid date format. Please enter date in YYYY-MM-DD format.\n");
        }
        else 
        {
            // Get the home directory path
            char* home_dir = getenv("HOME");
            if (home_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(npsd_commd, "\nHome directory not found\n");
                perror("\nError in Home environment variable\n"); 
               
            }

            char npsd_primary_path[1024];
            sprintf(npsd_primary_path, "%s/", home_dir);

            char npsd_commd_buf[BUFSIZE]; // Finding files that match the date range
            
            sprintf(npsd_commd_buf, "find %s -type f -exec sh -c 'test $(stat -c %%Y \"$1\") -le $(date -d \"%s\" +%%s)' _ {} \\; -print0 | xargs -0 tar --ignore-failed-read -czf temp.tar.gz",
                    npsd_primary_path, npsd_date_str);
            /*sprintf(npsd_commd_buf, "find %s -type f -exec sh -c 'test $(stat -c %%Y \"$1\") -le $(date -d \"%s\" +%s)' _ {} \\; -print0 | xargs -0 tar -czf temp.tar.gz",
                    npsd_primary_path, npsd_date_str, npsd_date_str);*/
            system(npsd_commd_buf);

            sprintf(npsd_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file.\n");
        }
    }
}



// Execution of w24fda command
else if (strcmp(npsd_token, "w24fda") == 0)  
{  
    char* npsd_date_str = strtok(NULL, " ");
     
    if (npsd_date_str == NULL) {
        sprintf(npsd_commd, "\nEntered syntax is not valid. Please Enter again.\n");
    } 
    else 
    {
        // Add validation for date format
        if (!npsd_date_input_valid(npsd_date_str)) {
            sprintf(npsd_commd, "\nInvalid date format. Please enter date in YYYY-MM-DD format.\n");
        }
        else 
        {
            // Get the home directory path
            char* home_dir = getenv("HOME");
            if (home_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(npsd_commd, "\nHome directory not found\n");
                perror("\nError in Home environment variable\n"); 
                //return 1; // Exit with error
            }

            char npsd_primary_path[1024];
            sprintf(npsd_primary_path, "%s/", home_dir);

            char npsd_commd_buf[BUFSIZE]; // Finding files that match the date range
            sprintf(npsd_commd_buf, "find %s -type f -exec sh -c 'test $(stat -c %%Y \"$1\") -ge $(date -d \"%s\" +%%s)' _ {} \\; -print0 | xargs -0 tar --ignore-failed-read -czf temp.tar.gz",
                    npsd_primary_path, npsd_date_str);
            system(npsd_commd_buf);

            sprintf(npsd_commd, "\nFiles are retrieved successfully. Please check temp.tar.gz file.\n");
        }
    }
}

       
// Execution of w24ft command
else if (strcmp(npsd_token, "w24ft") == 0)
{ 
    char* npsd_exten1 = strtok(NULL, " ");
    char* npsd_exten2 = strtok(NULL, " ");
    char* npsd_exten3 = strtok(NULL, " ");
    char* npsd_exten4 = strtok(NULL, " "); // Additional extension

    if (npsd_exten4 != NULL) {
        sprintf(npsd_commd, "\nMaximum three file extensions allowed.\n");
        //return; // Exit the function
    } else{
    
    // Get the home directory path
            char* home_dir = getenv("HOME");
            if (home_dir == NULL) {
                // Handle case where HOME environment variable is not set
                sprintf(npsd_commd, "\nHome directory not found\n");
                perror("\nError in Home enviroment variable");           
     }

    char npsd_primary_path[1024];
    sprintf(npsd_primary_path, "%s/", home_dir);
    char npsd_commd_buf[BUFSIZE];  // Checking whether any of the specified files are present
    sprintf(npsd_commd_buf, "find %s -type f \\( ", npsd_primary_path);
    if (npsd_exten1 != NULL) sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten1);
    if (npsd_exten2 != NULL) sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten2);
    if (npsd_exten3 != NULL) sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-iname \"*.%s\" -o ", npsd_exten3);
    sprintf(npsd_commd_buf + strlen(npsd_commd_buf), "-false \\) -print0 | xargs -0 tar -czf temp.tar.gz");

    int npsd_status = system(npsd_commd_buf);

    if (npsd_status == 0)  // Check if the files were found
    {
        sprintf(npsd_commd, "\nFiles are retrieved successfully.Please check temp.tar.gz file.\n");
    } 
    else 
    {
        sprintf(npsd_commd, "\nNo file found.\n");
    }
  }
}
        //quitc command
        else if (strcmp(npsd_token, "quitc") == 0) 
        {
            sprintf(npsd_commd, "\nBye\n");
            break;
        } 
        else 
        {
            sprintf(npsd_commd, "\nEntered syntax is not valid. Please Enter again %s.\n",npsd_token);
        }
        send(sockfd, npsd_commd, strlen(npsd_commd), 0); // Send response to client
    }

    close(sockfd);
    exit(0);
}



// Main method for mirror server
int main(int argc, char const *argv[]) {
    int npsd_server_fd, npsd_new_soc;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creatinf socket file descriptor
    if ((npsd_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Attach socket to the port 8080
    if (setsockopt(npsd_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //BIND()
    if (bind(npsd_server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //LISTEN()
    if (listen(npsd_server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    
    //INFINTE loop
    while (1) {
        if ((npsd_new_soc = accept(npsd_server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
           perror("accept");
           exit(EXIT_FAILURE);
       }


        printf("The New client is connected --->. Forking child process...\n");

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // Child process
            close(npsd_server_fd);
            crequest(npsd_new_soc);
        } else {  // Parent process
            close(npsd_new_soc);
            while (waitpid(-1, NULL, WNOHANG) > 0);  // Clean up zombie processes
        }
    }

    return 0;
}