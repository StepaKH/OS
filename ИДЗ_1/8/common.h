#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <semaphore.h>

char fd1_name[] = "fd1"; // первый именованный канал
char fd2_name[] = "fd2"; // второй именованный канал

const char *sem_name = "/full-semaphore";
sem_t *sem;

#define BUF_SIZE 5000

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