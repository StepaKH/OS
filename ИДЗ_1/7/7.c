#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
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

    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";
    mkfifo(fifo1, 0666);
    mkfifo(fifo2, 0666);

    pid_t pid1, pid2;
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Child process (Process 1)

        int fifo1_fd = open(fifo1, O_WRONLY);
        if (fifo1_fd == -1) {
            perror("open fifo1 for writing");
            exit(EXIT_FAILURE);
        }

        // Read data from input file
        FILE* input_file = fopen(argv[1], "r");
        if (!input_file) {
            perror("fopen input_file");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, input_file)) > 0) {
            if (write(fifo1_fd, buffer, bytes_read) != bytes_read) {
                perror("write fifo1");
                exit(EXIT_FAILURE);
            }
        }

        close(fifo1_fd); // Close the write end of fifo1
        fclose(input_file); // Close the input file
        wait(NULL); // Wait for the child process (Process 2) to complete

        exit(EXIT_SUCCESS);
    }

    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        // Child process (Process 2)

        int fifo1_fd = open(fifo1, O_RDONLY);
        if (fifo1_fd == -1) {
            perror("open fifo1 for reading");
            exit(EXIT_FAILURE);
        }

        int fifo2_fd = open(fifo2, O_WRONLY);
        if (fifo2_fd == -1) {
            perror("open fifo2 for writing");
            exit(EXIT_FAILURE);
        }

        // Read data from parent process (Process 1)
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(fifo1_fd, buffer, BUFFER_SIZE);
        if (bytes_read == -1) {
            perror("read fifo1");
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

        if (write(fifo2_fd, result_buffer, result_index) != result_index) {
            perror("write fifo2");
            exit(EXIT_FAILURE);
        }

        close(fifo1_fd); // Close the read end of fifo1
        close(fifo2_fd); // Close the write end of fifo2

        exit(EXIT_SUCCESS);
    }

    // Parent process

    int fifo2_fd = open(fifo2, O_RDONLY);
    if (fifo2_fd == -1) {
        perror("open fifo2 for reading");
        exit(EXIT_FAILURE);
    }

    // Open output file
    FILE* output_file = fopen(argv[2], "w");
    if (!output_file) {
        perror("fopen output_file");
        exit(EXIT_FAILURE);
    }

    // Read processed data from child process (Process 2)
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(fifo2_fd, buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
        perror("read fifo2");
        exit(EXIT_FAILURE);
    }

    // Write processed data to output file
    if (fwrite(buffer, 1, bytes_read, output_file) != bytes_read) {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }

    close(fifo2_fd); // Close the read end of fifo2
    fclose(output_file); // Close the output file

    // Wait for child process (Process 2) to complete
    wait(NULL);

    // Remove FIFOs
    unlink(fifo1);
    unlink(fifo2);

    return 0;
}