//run: gcc -shared -fPIC multilevelBF.c -o multilevelBF.so

/*
Student No.: 111550076
Student Name: 楊子賝
Email: zichen55.cs11@mycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h> // for open

#define HEAP_SIZE 20000
#define ALIGNMENT 32

typedef struct block {
    uint32_t size;
    uint32_t free;
    struct block *fl_prev;
    struct block *fl_next;
    struct block *next;
} block_t;

block_t *free_list[11] = {NULL};
/*
[0]: 0-31
[1]: 32-63
[2]: 64-127
[3]: 128-255
[4]: 256-511
[5]: 512-1023
[6]: 1024-2047
[7]: 2048-4095
[8]: 4096-8191
[9]: 8192-16383
[10]: 16384-32767
*/
void *heap_start = NULL;
block_t *initial_block = NULL;

int get_idx(uint32_t size){
    int index = 0;
    uint32_t s = ALIGNMENT;
    while(size > s && index < 10){
        s <<= 1;
        index++;
    }
    return index;
}

void insert_free_block(block_t *blk){
    int index = get_idx(blk->size);
    blk->fl_next = free_list[index];
    blk->fl_prev = NULL;
    if(free_list[index]){
        free_list[index]->fl_prev = blk;
    }
    free_list[index] = blk;
    blk->free = 1;
}

void remove_free_block(block_t *blk){
    int index = get_idx(blk->size);

    if (blk->fl_prev == NULL) { // blk is head of list
        free_list[index] = blk->fl_next;
    } else {
        blk->fl_prev->fl_next = blk->fl_next;
    }

    if (blk->fl_next != NULL) { // blk is not tail
        blk->fl_next->fl_prev = blk->fl_prev;
    }

    blk->fl_prev = NULL;
    blk->fl_next = NULL;
}

int init_memory() {
    heap_start = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (heap_start == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }
    initial_block = (block_t *)heap_start;
    initial_block->size = HEAP_SIZE - sizeof(block_t);
    initial_block->free = 1;
    initial_block->next = NULL; 
    initial_block->fl_prev = NULL;
    initial_block->fl_next = NULL;
    insert_free_block(initial_block);
    return 0;
}

// void print_memory_layout(const char *operation, size_t size) {
//     int file = open("memory_layout.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
//     if (file < 0) {
//         perror("Failed to open file");
//         return;
//     }
    
//     char output[2000]; // Increased buffer size for memory layout information
//     int ptr = 0;
//     ptr += sprintf(output + ptr, "Operation: %s, Size: %zu\n", operation, size);
//     ptr += sprintf(output + ptr, "Memory Layout:\n");

//     block_t *blk = initial_block;
//     while (blk) {
//         ptr += sprintf(output + ptr, "[Addr: %p, Size: %u, Free: %u] -> ", (void*)blk, blk->size, blk->free);
//         blk = blk->next;
//     }
//     ptr += sprintf(output + ptr, "NULL\n\n");

//     write(file, output, ptr);
//     close(file);
// }


// void print_free_lists(const char *operation, size_t size) {
//     int file = open("free_list_status.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
//     if (file < 0) {
//         perror("Failed to open file");
//         return;
//     }
    
//     char output[2000]; // Increased buffer size for addr information
//     int ptr = 0;
//     ptr += sprintf(output + ptr, "Operation: %s, Size: %zu\n", operation, size);
//     ptr += sprintf(output + ptr, "Free lists status:\n");

//     for (int i = 0; i < 11; i++) {
//         ptr += sprintf(output + ptr, "List[%d]: ", i);
//         block_t *blk = free_list[i];
        
//         while (blk) {
//             ptr += sprintf(output + ptr, "[Addr: %p, Size: %u, Free: %u] -> ", (void*)blk, blk->size, blk->free);
//             blk = blk->fl_next;
//         }
//         ptr += sprintf(output + ptr, "NULL\n");
//     }
    
//     write(file, output, ptr);
//     close(file);
// }

void *malloc(size_t size) {
    uint32_t size_32 = (uint32_t)size;
    if (heap_start == NULL) {
        if (init_memory() != 0) {
            return NULL;
        }
    }

    if (size_32 == 0) {
        uint32_t ans = 0;
        block_t *curr = initial_block;
        while (curr) {
            if (curr->free && curr->size > ans) ans = curr->size;
            curr = curr->next;
        }
        char output[40];
        int ptr = 0;
        ptr += sprintf(output + ptr, "Max Free Chunk Size = %u\n", ans);
        write(1, output, ptr);
        munmap(heap_start, HEAP_SIZE);
        return NULL;
    }

    size_32 = ((size_32 + (ALIGNMENT - 1)) / ALIGNMENT) * ALIGNMENT;
    int idx = get_idx(size_32);
    block_t *best = NULL;
    
    for (int i = idx; i < 11; ++i) {
        block_t *head = free_list[i];
        while (head) {
            if (head->size >= size_32) {
                if (best == NULL || head->size <= best->size) {
                    best = head;
                }
            }
            head = head->fl_next;
        }
        if (best) break;
    }
    
    if (best) {
        remove_free_block(best);
        if (best->size >= size_32 + sizeof(block_t)) {
            block_t* new_block = (block_t*)((char*)best + sizeof(block_t) + size_32);
            new_block->size = best->size - size_32 - sizeof(block_t);
            new_block->free = 1;
            new_block->fl_prev = NULL;
            new_block->fl_next = NULL;
            new_block->next = best->next;
            best->next = new_block;
            insert_free_block(new_block);
            best->size = size_32;
        }
        best->free = 0;

        // print_free_lists("malloc", size_32); // Output free list after malloc
        // print_memory_layout("malloc", size_32);
        return (void *)(best + 1);
    }

    // print_free_lists("malloc", size_32); // Output free list after malloc attempt (no block found)
    // print_memory_layout("malloc", size_32);
    return NULL;
}

void free(void *ptr) {
    if (ptr == NULL) return;

    block_t *curr = ((block_t*)ptr) - 1;
    curr->free = 1;

    block_t *head = initial_block;
    while (head && head->next != curr) head = head->next;

    if (head && head->free) {
        remove_free_block(head);
        head->size += curr->size + sizeof(block_t);
        head->next = curr->next;
        curr = head;
    }

    if (curr->next && curr->next->free) {
        remove_free_block(curr->next);
        curr->size += curr->next->size + sizeof(block_t);
        curr->next = curr->next->next;
    }

    insert_free_block(curr);
    // print_free_lists("free", 0); // Output free list after free
    // print_memory_layout("free", 0);
}