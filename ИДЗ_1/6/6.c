#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#define BUFFER_SIZE 5000

// Function to check if a character is a letter
int is_alpha(char c) {
    return isalpha(c);
}

// Function to check if a word is a palindrome
int is_palindrome(const char* word) {
    int len = strlen(word);
    for (int i = 0; i < len / 2; ++i) {
        if (word[i] != word[len - i - 1]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    struct stat st;
    if (stat(argv[1], &st) == 0 && st.st_size == 0) {
        printf("Input file is empty. Exiting...\n");
        exit(EXIT_SUCCESS);
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1, pid2;
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Child process (Process 1)

        // Close read end of pipe
        close(pipe_fd[0]);
        
        // Read data from input file
        FILE* input_file = fopen(argv[1], "r");
        if (!input_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(fileno(input_file), buffer, BUFFER_SIZE)) > 0) {
            // Sending data to to Process 2 through the pipe
            if (write(pipe_fd[1], buffer, bytes_read) != bytes_read) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }

        // Close write end of pipe
        close(pipe_fd[1]);

        // Wait for child process (Process 2) to complete
        wait(NULL);
        fclose(input_file); // Close input file
        exit(EXIT_SUCCESS);
    }

   // Create a new pipe for communication between Process 2 and Process 1
    int pipe_fd2[2];
    if (pipe(pipe_fd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
   
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        // Child process (Process 2)

        // Close write end of pipe
        close(pipe_fd[1]);

        // Close read end of second pipe
        close(pipe_fd2[0]);

        // Read data from parent process (Process 1)
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE);
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Data processing (search for palindromes)
        char result_buffer[BUFFER_SIZE];
        int result_index = 0;
        char current_word[BUFFER_SIZE];
        int word_index = 0;
        
        for (int i = 0; i <= bytes_read; i++) {
            if (is_alpha(buffer[i])) {
                current_word[word_index++] = tolower(buffer[i]);
            }
            else {
                if (word_index >= 1 && is_palindrome(current_word)) {
                    // If the word is a palindrome, add it to the result
                    strcpy(&result_buffer[result_index], current_word);
                    result_index += word_index;
                    result_buffer[result_index++] = ' '; // Separator between words
                }
                word_index = 0;
                memset(current_word, 0, sizeof(current_word));
            }
        }

        // Write processed data to parent process (Process 1) through the second pipe
        if (write(pipe_fd2[1], result_buffer, result_index) != result_index) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        // Close write end of second pipe
        close(pipe_fd2[1]);
        exit(EXIT_SUCCESS);
    }

    // Parent process (Process 1)

    // Close read end of first pipe
    close(pipe_fd[0]);

    // Close write end of second pipe
    close(pipe_fd2[1]);

    // Open output file
    FILE *output_file = fopen(argv[2], "w");
    if (!output_file) {
        perror("fopen output_file");
        exit(EXIT_FAILURE);
    }
    
    // Read processed data from child process (Process 2)
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(pipe_fd2[0], buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    
    // Write processed data to output file
    ssize_t bytes_written = fwrite(buffer, 1, bytes_read, output_file);
    if (bytes_written < bytes_read) {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }

    // Close read end of second pipe
    close(pipe_fd2[0]);

    // Close output file
    fclose(output_file);

    // Wait for child process (Process 2) to complete
    wait(NULL);

    return 0;
}
