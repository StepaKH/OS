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

    // Creating channels
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1, pid2, pid3;
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Process 1
        close(pipe1[0]); // Close reading channel 1
        // Open input file
        FILE* input_file = fopen(argv[1], "r");
        if (!input_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(fileno(input_file), buffer, BUFFER_SIZE)) > 0) {
            // Send data to channel 1
            if (write(pipe1[1], buffer, bytes_read) != bytes_read) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }

        close(pipe1[1]); // Close recording to channel 1
        fclose(input_file); // Close input file
        exit(EXIT_SUCCESS);
    }

    // Process 2
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        close(pipe1[1]); // Close channel 1 recording
        close(pipe2[0]); // Close reading channel 2

        // Reading data from channel 1
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(pipe1[0], buffer, BUFFER_SIZE);
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

        // Recording the result to channel 2
        if (write(pipe2[1], result_buffer, result_index) != result_index) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(pipe1[0]); // Close reading channel 1
        close(pipe2[1]); // Close recording to channel 2
        exit(EXIT_SUCCESS);
    }

    // Process 3
    pid3 = fork();
    if (pid3 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid3 == 0) {
        close(pipe2[1]); // Close channel 2 recording

        // Open output file
        FILE* output_file = fopen(argv[2], "w");
        if (!output_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Read data from channel 2 and write to output file
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(pipe2[0], buffer, BUFFER_SIZE)) > 0) {
            if (write(fileno(output_file), buffer, bytes_read) != bytes_read) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }

        close(pipe2[0]); // Close reading channel 2
        fclose(output_file); // Close output file
        exit(EXIT_SUCCESS);
    }

    // Close unused pipe ends in parent process
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    // Wait for all child processes to complete
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}