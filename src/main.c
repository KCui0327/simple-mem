#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include "main.h"

pthread_mutex_t global_malloc_lock;

header_t* get_free_blk(size_t size) {
    header_t* curr = head;

    while (curr != NULL) {
        if (curr->s.is_free && curr->s.size >= size) {
            return curr;
        }
        curr = curr->s.next;
    }

    return NULL;
}

void* malloc(size_t size) {
    size_t total_size;
    void* blk;
    header_t* header;

    if (size == 0) {
        return NULL;
    }

    pthread_mutex_lock(&global_malloc_lock);
    
    header = get_free_blk(size);
    if (header != NULL) {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void* )(header + 1);
    }

    total_size = sizeof(header_t) + size;
    blk = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (blk == (void* ) - 1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    header = blk;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if (!head) head = header;
    if (tail) tail->s.next = header;

    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void* )(header + 1);
}

void free(void* blk) {
    header_t* header;
    header_t* tmp;
    void* pb;

    if (blk == NULL) {
        return;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t* )blk - 1;

    pb = mmap(NULL, 0, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((char* )blk + header->s.size == pb) {
        if (head == tail) {
            head = tail = NULL;
        } else {
            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        munmap(header, sizeof(header_t) + header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }

    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

void* calloc(size_t num, size_t nsize) {
    size_t size;
    void* blk;

    if (num == 0 || nsize == 0) {
        return NULL;
    }

    size = num * nsize;

    // check for multiplication overflow
    if (nsize != size / num) {
        return NULL;
    }

    blk = malloc(size);
    if (blk == NULL) {
        return NULL;
    }

    memset(blk, 0, size);

    return blk;
}

void* realloc(void* blk, size_t size) {
    header_t* header;
    void* ret;

    if (blk == NULL || size == 0) {
        return malloc(size);
    }

    header = (header_t* )blk - 1;

    if (header->s.size >= size) {
        return blk;
    }

    ret = malloc(size);
    if (ret != NULL) {
        memcpy(ret, blk, header->s.size);
        free(blk);
    }

    return ret;
}

int main(void) {
    int* arr;
    int* temp;

    arr = (int* )malloc(sizeof(int) * 20);
    temp = (int* )calloc(sizeof(int), 20);

    for (int i = 0; i < 20; i++) {
        arr[i] = i + 1;
    }

    for (int i = 0; i < 20; i++) {
        printf("arr[i] = %d\n", arr[i]);
        printf("temp[i] = %d\n", temp[i]);
    }

    temp = realloc(temp, sizeof(int) * 40);

        for (int i = 0; i < 40; i++) {
        printf("temp[i] = %d\n", temp[i]);
    }

    free(arr);
    free(temp);

    return 0;
}
