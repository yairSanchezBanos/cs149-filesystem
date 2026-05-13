#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <pthread.h>

#define MAX_FILENAME 64
#define DISK_SIZE 1048576
#define MAXFILE 64
#define READ 0
#define WRITE 1

struct Super_block
{

    int totalDiskSize; //total size of the disk
    int used_space;     //space being used up 
    int num_files;      //total number of files on the disk

};

struct FCB
{
    char file_name[MAX_FILENAME];// max size of a name will be 64 bytes
    int start; 
    int size;
    int in_use;

};

struct File_descriptor
{
    struct FCB *pfcb;
    int offset;
    int mode;
    int is_open;

};

//global variables that will be used
char sim_disk[DISK_SIZE];
struct Super_block s1 = {DISK_SIZE, 0, 0};
struct FCB fcb_table[MAXFILE];
struct File_descriptor fd_table[MAXFILE];
pthread_mutex_t lock;

//takes care of oppening the file 
//consider two cases
//case 1: open a file that does not exist in mode write or read
//if if in the mode for write then the file will be created, form of creating a file
//case 2: open a file that does exist, here index for file descriptor will be returned

int fs_open(char *file_name, int mode){
    pthread_mutex_lock(&lock);
    int fcb_index = -1;
    int fd_index = -1;

    //leanear search for the file
    for(int i = 0; i < MAXFILE; i++){
        if(fcb_table[i].in_use == 1 && 
            strcmp(file_name, fcb_table[i].file_name) == 0) fcb_index = i;
    }

    if (mode == READ && fcb_index == -1){
        pthread_mutex_unlock(&lock);
        return -1; //can not read a file that does not exist
    }else if (mode == WRITE && fcb_index == -1)//going to create a new file
    {

        for (int  i = 0; i < MAXFILE; i++)
        {
            if (fcb_table[i].in_use == 0){
                fcb_index = i;
                break;
            }
        }
        
        if(fcb_index == -1){

            pthread_mutex_unlock(&lock);
            return -1; // disk is full, cant assign anything new
        } 
        //initializing all aspects of the new file within the table
        strcpy(fcb_table[fcb_index].file_name, file_name);
        fcb_table[fcb_index].start = s1.used_space;
        fcb_table[fcb_index].size = 0;
        fcb_table[fcb_index].in_use = 1;
        s1.num_files++;
    }
    
    //finding available descriptor 
    for(int i = 0; i < MAXFILE; i++){
        if(fd_table[i].is_open == 0){
            fd_index = i;
            break;
        }
    }
    
    //table is maxed out
    if(fd_index == -1){

        pthread_mutex_unlock(&lock);
        return fd_index;
    } 

    //initiazlize everything
    fd_table[fd_index].pfcb = &fcb_table[fcb_index];
    fd_table[fd_index].mode = mode;
    fd_table[fd_index].offset = 0;
    fd_table[fd_index].is_open = 1;

    pthread_mutex_unlock(&lock);
    return fd_index;

}

//writes to a file, using a buffer and loop that will copy to the disk
int fs_write(int fd, char *buffer, int num_bytes){

   pthread_mutex_lock(&lock);

   //either descriptor is not open, or the file is not made for writing
    if(fd_table[fd].is_open != 1 || fd_table[fd].mode != WRITE) {
        pthread_mutex_unlock(&lock);
        return -1;
    }

    int pos = fd_table[fd].pfcb->start + fd_table[fd].offset;

    //"write to the disk", copy from buffer to disk 
    for(int i = 0; i < num_bytes; i++){

        sim_disk[pos + i] = buffer[i];

    }

    fd_table[fd].offset += num_bytes;
    fd_table[fd].pfcb->size += num_bytes;
    s1.used_space += num_bytes;

    pthread_mutex_unlock(&lock);
    return num_bytes;
}

//read from a file, same concpet as write except opposite order disk-> buffer
int fs_read(int fd, char *buffer, int num_bytes){

    pthread_mutex_lock(&lock);

    if(fd_table[fd].is_open != 1 || fd_table[fd].mode != READ) {
        pthread_mutex_unlock(&lock);
        return -1;
    }

    int pos = fd_table[fd].pfcb->start + fd_table[fd].offset;

    //uinique scenario where could read past file, want to prevent that 
    int bytes_left = fd_table[fd].pfcb->size - fd_table[fd].offset;

    if(num_bytes > bytes_left) num_bytes = bytes_left;

    for(int i = 0; i < num_bytes; i++){

        buffer[i] = sim_disk[pos + i];

    }

    fd_table[fd].offset += num_bytes;

    pthread_mutex_unlock(&lock);
    return num_bytes;
}

//release the descriptor after reading or writing
int fs_close(int fd){

    pthread_mutex_lock(&lock);
    if(fd_table[fd].is_open != 1) {
        pthread_mutex_unlock(&lock);
        return -1; //check that the descriptor is really open
    }
    
    fd_table[fd].offset = 0;
    fd_table[fd].mode = 0;
    fd_table[fd].pfcb = NULL;
    fd_table[fd].is_open = 0;

    pthread_mutex_unlock(&lock);
    return 0; //closed succesfully
}

//linear search for a file
int fs_search(char *file_name){

    pthread_mutex_lock(&lock);
    int fcb_index = -1;

    for(int i = 0; i < MAXFILE; i++){
        if(fcb_table[i].in_use == 1 && 
            strcmp(file_name, fcb_table[i].file_name) == 0) fcb_index = i;
    }

    pthread_mutex_unlock(&lock);
    return fcb_index;
}

//deletes instance of file by setting the value in file control block to 0, 
//will be overwritten
int fs_delete(char *file_name){

    pthread_mutex_lock(&lock);
    int fcb_ind = -1;

    for(int i = 0; i < MAXFILE; i++){
        if(fcb_table[i].in_use == 1 && 
            strcmp(file_name, fcb_table[i].file_name) == 0) fcb_ind = i;
    }

    if (fcb_ind == -1){
        
        pthread_mutex_unlock(&lock);
        return fcb_ind;
    }

    fcb_table[fcb_ind].in_use = 0;
    memset(fcb_table[fcb_ind].file_name, 0, MAX_FILENAME);
    s1.num_files--;

    pthread_mutex_unlock(&lock);
    return 0;

}

//initialize mutex, and tabels
void fs_init(){

    pthread_mutex_init(&lock, NULL);
    memset(fcb_table, 0, sizeof(fcb_table));
    memset(fd_table, 0, sizeof(fd_table));

}

//demo for threads shows prevention of race conditions
void *thread_demo(void *arg){
    char *filename = (char *)arg;
    printf("Thread opening: %s\n", filename);
    
    int fd = fs_open(filename, WRITE);
    fs_write(fd, "thread data", 11);
    fs_close(fd);

    // reopen and verify data integrity
    fd = fs_open(filename, READ);
    char buffer[20] = {0};
    fs_read(fd, buffer, 11);
    fs_close(fd);
    
    printf("Thread done: %s | data verified: %s\n", filename, buffer);
    return NULL;
}

//demo of playing with the files
int main(){

    fs_init();
    char input[256];
    char cmd[20];
    char filename[MAX_FILENAME];
    char mode_str[5];
    char data[256];
    int fd;

    printf("File System Simulator\n");
    printf("Commands: open <filename> <r/w>, write <fd> <data>, read <fd>" 
        "close <fd>, search <filename>, delete <filename>, exit\n\n");

    while(1){
        printf("> ");
        fgets(input, sizeof(input), stdin);

        sscanf(input, "%s", cmd);

        if(strcmp(cmd, "exit") == 0){
            printf("Exiting file system.\n");
            break;

        }else if(strcmp(cmd, "open") == 0){
            sscanf(input, "%s %s %s", cmd, filename, mode_str);
            int mode = (strcmp(mode_str, "w") == 0) ? WRITE : READ;
            fd = fs_open(filename, mode);
            printf("fs_open: fd = %d\n", fd);

        }else if(strcmp(cmd, "write") == 0){
            sscanf(input, "%s %d %[^\n]", cmd, &fd, data);
            int bytes = fs_write(fd, data, strlen(data));
            printf("fs_write: bytes written = %d\n", bytes);

        }else if(strcmp(cmd, "read") == 0){
            sscanf(input, "%s %d", cmd, &fd);
            char buffer[256] = {0};
            int bytes = fs_read(fd, buffer, 255);
            printf("fs_read: bytes read = %d\n", bytes);
            printf("fs_read: content = %s\n", buffer);

        }else if(strcmp(cmd, "close") == 0){
            sscanf(input, "%s %d", cmd, &fd);
            int result = fs_close(fd);
            printf("fs_close: result = %d\n", result);

        }else if(strcmp(cmd, "search") == 0){
            sscanf(input, "%s %s", cmd, filename);
            int index = fs_search(filename);
            if(index == -1) printf("fs_search: file not found\n");
            else printf("fs_search: file found at fcb_index = %d\n", index);

        }else if(strcmp(cmd, "delete") == 0){
            sscanf(input, "%s %s", cmd, filename);
            int result = fs_delete(filename);
            printf("fs_delete: result = %d\n", result);

        }else{
            printf("Unknown command.\n");
        }
    }

    // multi-thread demo
    printf("\n--- Thread Safety Demo ---\n");
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread_demo, "file1.txt");
    pthread_create(&t2, NULL, thread_demo, "file2.txt");
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Both threads completed successfully\n");

    return 0;
}