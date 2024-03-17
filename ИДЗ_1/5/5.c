#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>

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

    // Creating named pipes (FIFOs)
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";
    mkfifo(fifo1, 0666);
    mkfifo(fifo2, 0666);

    pid_t pid1, pid2, pid3;
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Process 1: Reading from input file and writing to FIFO 1
        FILE* input_file = fopen(argv[1], "r");
        if (!input_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        
        int fifo1_fd = open(fifo1, O_WRONLY);
        if (fifo1_fd == -1) {
            perror("open fifo1");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(fileno(input_file), buffer, BUFFER_SIZE)) > 0) {
            // Sending data to FIFO 1
            if (write(fifo1_fd, buffer, bytes_read) != bytes_read) {
                perror("write fifo1");
                exit(EXIT_FAILURE);
            }
        }

        close(fifo1_fd); // Close writing to FIFO 1
        fclose(input_file); // Close input file
        exit(EXIT_SUCCESS);
    }

    // Process 2: Reading from FIFO 1, processing data, and writing to FIFO 2
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        int fifo1_fd = open(fifo1, O_RDONLY);
        if (fifo1_fd == -1) {
            perror("open fifo1");
            exit(EXIT_FAILURE);
        }

        int fifo2_fd = open(fifo2, O_WRONLY);
        if (fifo2_fd == -1) {
            perror("open fifo2");
            exit(EXIT_FAILURE);
        }

        // Reading data from FIFO 1
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(fifo1_fd, buffer, BUFFER_SIZE);
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

        // Writing the result to FIFO 2
        if (write(fifo2_fd, result_buffer, result_index) != result_index) {
            perror("write fifo2");
            exit(EXIT_FAILURE);
        }

        close(fifo1_fd);
        close(fifo2_fd);
        exit(EXIT_SUCCESS);
    }

    // Process 3: Reading from FIFO 2 and writing to output file
    pid3 = fork();
    if (pid3 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid3 == 0) {
        // Open output file
        FILE* output_file = fopen(argv[2], "w");
        if (!output_file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        
        int fifo2_fd = open(fifo2, O_RDONLY);
        if (fifo2_fd == -1) {
            perror("open fifo2");
            exit(EXIT_FAILURE);
        }

        // Read data from FIFO 2 and write to output file
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(fifo2_fd, buffer, BUFFER_SIZE)) > 0) {
            if (write(fileno(output_file), buffer, bytes_read) != bytes_read) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }

        close(fifo2_fd); // Close reading from FIFO 2
        fclose(output_file); // Close output file
        exit(EXIT_SUCCESS);
    }

    // Wait for all child processes to complete
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}