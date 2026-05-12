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

int sim_disk[DISK_SIZE];
struct Super_block s1 = {DISK_SIZE, 0, 0};
struct FCB fcb_table[MAXFILE];
struct File_descriptor fd_table[MAXFILE];
pthread_mutex_t lock;

int fs_open(char *file_name, int mode){
    pthread_mutex_lock(&lock);
    int fcb_index = -1;
    int fd_index = -1;

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
        strcpy(fcb_table[fcb_index].file_name, file_name);
        fcb_table[fcb_index].start = s1.used_space;
        fcb_table[fcb_index].size = 0;
        fcb_table[fcb_index].in_use = 1;
        s1.num_files++;
    }
    
    for(int i = 0; i < MAXFILE; i++){
        if(fd_table[i].is_open == 0){
            fd_index = i;
            break;
        }
    }
    
    
    if(fd_index == -1){

        pthread_mutex_unlock(&lock);
        return fd_index;
    } 

    fd_table[fd_index].pfcb = &fcb_table[fcb_index];
    fd_table[fd_index].mode = mode;
    fd_table[fd_index].offset = 0;
    fd_table[fd_index].is_open = 1;

    pthread_mutex_unlock(&lock);
    return fd_index;

}

int fs_write(int fd, char *buffer, int num_bytes){

   pthread_mutex_lock(&lock);

    if(fd_table[fd].is_open != 1 || fd_table[fd].mode != WRITE) {
        pthread_mutex_unlock(&lock);
        return -1;
    }

    int pos = fd_table[fd].pfcb->start + fd_table[fd].offset;

    for(int i = 0; i < num_bytes; i++){

        sim_disk[pos + i] = buffer[i];

    }

    fd_table[fd].offset += num_bytes;
    fd_table[fd].pfcb->size += num_bytes;
    s1.used_space += num_bytes;

    pthread_mutex_unlock(&lock);
    return num_bytes;
}

int fs_read(int fd, char *buffer, int num_bytes){

    pthread_mutex_lock(&lock);

    if(fd_table[fd].is_open != 1 || fd_table[fd].mode != READ) {
        pthread_mutex_unlock(&lock);
        return -1;
    }

    int pos = fd_table[fd].pfcb->start + fd_table[fd].offset;

    int bytes_left = fd_table[fd].pfcb->size - fd_table[fd].offset;

    if(num_bytes > bytes_left) num_bytes = bytes_left;

    for(int i = 0; i < num_bytes; i++){

        buffer[i] = sim_disk[pos + i];

    }

    fd_table[fd].offset += num_bytes;

    pthread_mutex_unlock(&lock);
    return num_bytes;
}

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

void fs_init(){

    pthread_mutex_init(&lock, NULL);
    memset(fcb_table, 0, sizeof(fcb_table));
    memset(fd_table, 0, sizeof(fd_table));

}

int main(){
    fs_init();

    // open file in write mode
    int fd = fs_open("test.txt", WRITE);
    printf("fs_open (write): fd = %d\n", fd);

    // write to file
    char *msg = "Hello, File System!";
    int bytes_written = fs_write(fd, msg, strlen(msg));
    printf("fs_write: bytes written = %d\n", bytes_written);

    // close file
    int closed = fs_close(fd);
    printf("fs_close: result = %d\n", closed);

    // open file in read mode
    fd = fs_open("test.txt", READ);
    printf("fs_open (read): fd = %d\n", fd);

    // read file
    char buffer[100] = {0};
    int bytes_read = fs_read(fd, buffer, strlen(msg));
    printf("fs_read: bytes read = %d\n", bytes_read);
    printf("fs_read: content = %s\n", buffer);

    // close file
    fs_close(fd);

    // search for file
    int found = fs_search("test.txt");
    printf("fs_search: fcb_index = %d\n", found);

    // delete file
    int deleted = fs_delete("test.txt");
    printf("fs_delete: result = %d\n", deleted);

    // confirm deletion
    found = fs_search("test.txt");
    printf("fs_search after delete: fcb_index = %d\n", found);

    return 0;
}