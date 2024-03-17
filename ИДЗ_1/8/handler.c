#include "common.h"

int main(int argc, char *argv[]) {
    char input_filename[256];  // имя файла для чтения
    char output_filename[256]; // имя файла для записи

    int input_descriptor;
    int output_descriptor;

    int fd1;
    int fd2;

    char buffer[BUF_SIZE]; // буфер

    (void)umask(0);

    if ((sem = sem_open(sem_name, O_CREAT, 0666, 0)) == 0) {
        perror("Reader: can't create admin semaphore");
        exit(10);
    }

    if (access(fd1_name, F_OK) == -1) {
        if (mknod(fd1_name, S_IFIFO | 0666, 0) < 0) {
            printf("->Reader: error while creating channel 1<-\n");
            exit(10);
        }
    }

    if (access(fd2_name, F_OK) == -1) {
        if (mknod(fd2_name, S_IFIFO | 0666, 0) < 0) {
            printf("->Reader: error while creating channel 2<-\n");
            exit(10);
        }
    }

    if ((fd1 = open(fd1_name, O_RDONLY)) < 0) { // открытие первого канала
        printf("->Handler: error with open FIFO 1 for reading<-\n");
        exit(10);
    }

    int size = read(fd1, buffer, BUF_SIZE);
    if (size < 0) {
        printf("->Handler: error with reading from pipe 1<-\n");
        exit(10);
    }

    char result_buffer[BUF_SIZE];
    int result_index = 0;
    char current_word[BUF_SIZE];
    int word_index = 0;

    for (int i = 0; i <= size; i++) {
        if (is_alpha(buffer[i])) {
            current_word[word_index++] = tolower(buffer[i]);
        } else {
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

    if ((fd2 = open(fd2_name, O_WRONLY)) < 0) { // открытие второго канала
        printf("->Handler: error with open FIFO 1 for reading<-\n");
        exit(10);
    }

    size = write(fd2, result_buffer, result_index); // запись текста из буфера в канал
    if (size < 0) {
        printf("->Handler: error with writing<-\n");
        exit(10);
    }

    if (close(fd1) != 0) { // закрытие канала
        printf("->Reader: error with closing fifo 1<-\n");
        exit(10);
    }

    if (close(fd2) < 0) {
        printf("->Reader: error with closing fifo 2<-\n");
        exit(10);
    }

    printf("->Handler: exit<-\n");

    if (sem_post(sem) == -1) {
        printf("Handler: can't increment admin semaphore");
        exit(10);
    }

    if (sem_close(sem) == -1) {
        printf("Handler: incorrect close of busy semaphore");
        exit(10);
    }

    return 0;
}
