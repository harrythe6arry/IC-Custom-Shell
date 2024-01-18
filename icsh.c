/* ICCS227: Project 1: icsh
 * Name: Voravit Srikuruwal
 * StudentID: 6580215
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#define MAX_CMD_BUFFER 255


int print_string(char *str) {
    printf("%s\n", str);
    return 0;
}


int main() {
    // printing the prompt
    char input_last_command[MAX_CMD_BUFFER] = ""; 
    printf("./icsh\n");
    printf("Starting IC shell\n");
    printf("PCSA > <waiting for commaand>\n");

    char buffer[MAX_CMD_BUFFER];
    while (1) {
        printf("PCSA > ");
        fgets(buffer, 255, stdin);
        // printf("you said: %s\n", buffer);
        // how can i built echo command in c

        // command line to print echo commandw
        // for (int i = 0; i < strlen(buffer); i++) {
        //     if (buffer[i] == '\n') {
        //         buffer[i] = '\0';
        //     }
        if (strcmp(buffer, "echo") == 0) { 
            print_string(buffer + 5);
            strcpy(input_last_command, buffer + 5);
        // if the command isnt in built in, then its error

        // if (strcmp(buffer, "!!") == 0) {
        //     if (strlen(input_last_command) > 0) {
        //         printf("%s\n", input_last_command); // print the last command
        //     } else {
        //         printf("No commands in history\n");
        //     }
        // }

        
        // } else if (strncmp(buffer, "!!", 2) == 0) {
        //       if (strlen(input_last_command) > 0) {
        //         printf("%s\n",input_last_command);
        } // why does this code keeps exiting
        }
        
    return 0; 
    }
    // how to check if command is echo




// create an echo built in command for icsh


