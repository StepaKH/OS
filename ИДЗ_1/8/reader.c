#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("->Incorrect number of arguments<-\n");
        exit(10);
    }
    
    char input_filename[256]; // имя файла для чтения
    char output_filename[256]; // имя файла для записи

    int input_descriptor;
    int output_descriptor;

    int fd1;
    int fd2;

    char buffer[BUF_SIZE]; // буфер

    (void) umask(0);

    strcpy(input_filename, argv[1]); // получение имен файлов из командной строки
    strcpy(output_filename, argv[2]);

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

    // открытие файла для чтения
    if ((input_descriptor = open(input_filename, O_RDONLY)) == -1) {
        printf("->Error with opening input file\n");
        exit(10);
    }

    int size = read(input_descriptor, buffer, BUF_SIZE); // считывание текста из файла в буфер
    if (size < 0) {
        printf("->Reader: error with reading file<-\n");
        exit(10);
    }
    
    if ((fd1 = open(fd1_name, O_WRONLY)) < 0) { // открытие первого канала
        printf("->Reader: error with open FIFO 1 for writing<-\n");
        exit(10);
    }

    size = write(fd1, buffer, size); // запись текста из буфера в канал
    if (size < 0) {
        printf("->Reader: error with writing in pipe 1<-\n");
        exit(10);
    }

    if (close(fd1) != 0) { // закрытие канала
        printf("->Reader: error with closing fifo 1<-\n");
        exit(10);
    }

    // закрытие файла
    if (close(input_descriptor) != 0) {
        printf("->Error with closing input file<-\n");
        exit(10);
    }

    char other_buffer[BUF_SIZE]; // буфер, другой для верности

    if ((fd2 = open(fd2_name, O_RDONLY)) < 0) { // открытие второго канала
        printf("->Reader: error with open FIFO 2 for writing<-\n");
        exit(10);
    }

    size = read(fd2, other_buffer, BUF_SIZE); // если fd2 пустой, то процесс блокируется
    if (size < 0) {
        printf("->Reader: error with reading from pipe 2<-\n");
        exit(10);
    }

    if (close(fd2) != 0) { // закрытие канала
        printf("->Reader: error with closing fifo 2<-\n");
        exit(10);
    }

    // открытие файла для записи. Если его нет, он будет создан. Если он уже существует, содержимое удалится
    if ((output_descriptor = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        printf("->Error with opening output file\n");
        exit(10);
    }

    size = write(output_descriptor, other_buffer, size);
    if (size < 0) {
        printf("->Reader: error with writing output file<-\n");
        exit(10);
    }

    // закрытие файла
    if (close(output_descriptor) != 0) {
        printf("->Error with closing output file<-\n");
        exit(10);
    }

    printf("->Reader: exit<-\n");

    if (sem_wait(sem) == -1) {
        printf("Reader: incorrect wait of reader semaphore");
        exit(10);
    }

    if (sem_close(sem) == -1) {
        printf("Reader: incorrect close of busy semaphore");
        exit(10);
    }

    remove(fd1_name);
    remove(fd2_name);

    return 0;
}