#include <fstream>
#include <list>
#include <sys/time.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
using namespace std;

#define HASH_SIZE 131101
#define hash_func(val) (val % HASH_SIZE)
#define INITIAL_CAPACITY 100000
#define PAGE_SIZE 4096 

typedef struct LRU_Node {
    int val;   
    int dirty;  
    CLEAN_Node *clean_node;
    LRU_Node *next, *prev;
    LRU_Node(int val, int dirty, LRU_Node *prev, LRU_Node *next, CLEAN_Node *clean_node) 
        : val(val), dirty(dirty), prev(prev), next(next) , clean_node(clean_node)  {}
} LRU_Node;

typedef struct CLEAN_Node {
    LRU_Node *node;
    CLEAN_Node *next, *prev;
    CLEAN_Node(LRU_Node *node, CLEAN_Node *prev, CLEAN_Node *next) 
        : node(node), prev(prev), next(next) {}
} CLEAN_Node;

LRU_Node* getNode(list<pair<int, LRU_Node*>> *hash, int page) {
    auto &bucket = hash[hash_func(page)];
    for (auto &entry : bucket) {
        if (entry.first == page) return entry.second;
    }
    return NULL;
}

void LRU_simulate(char* accessType, unsigned int* addresses, int sz) {
     for (int frame = 4096; frame <= 65536; frame <<= 1) {
        LRU_Node *head = NULL;
        LRU_Node *tail = NULL;
        list<pair<int, LRU_Node*>> hash[HASH_SIZE];
        int hit = 0, miss = 0, write_back = 0, curr_frames = 0;

        for (int i = 0; i < sz; ++i) {
            int page = addresses[i];
            char op = accessType[i];
            LRU_Node *curr = getNode(hash, page);

            if (curr) {
                hit++;
                if (op == 'W') curr->dirty = 1;
                if (curr != head) {
                    if (curr->prev) curr->prev->next = curr->next;
                    if (curr->next) curr->next->prev = curr->prev;
                    else tail = curr->prev;

                    curr->next = head;
                    curr->prev = NULL;
                    if (head) head->prev = curr;
                    head = curr;
                }
            } else {
                miss++;
                if (curr_frames == frame) {
                    if (tail) {
                        if (tail->dirty) write_back++;
                        auto &bucket = hash[hash_func(tail->val)];
                        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
                            if (it->first == tail->val) {
                                bucket.erase(it);
                                break;
                            }
                        }

                        if (tail->prev) {
                            LRU_Node *old_tail = tail;
                            tail = tail->prev;
                            tail->next = NULL;
                            delete old_tail;
                        } else {
                            delete tail;
                            head = tail = NULL;
                        }
                        curr_frames--;
                    }
                }

                curr = new LRU_Node(page, op == 'W' ? 1 : 0, NULL, head);
                if (head) head->prev = curr;
                head = curr;
                if (!tail) tail = curr;

                hash[hash_func(page)].push_back(make_pair(page, curr));
                curr_frames++;
            }
        }

        double ratio = (double)miss / (double)(hit + miss);
        printf("%d\t%d\t%d\t\t%.10f\t\t%d\n", frame, hit, miss, ratio, write_back);

        while (head) {
            LRU_Node *temp = head;
            head = head->next;
            delete temp;
        }
    }

}

void CFLRU_simulate(char* accessType, unsigned int* addresses, int sz) {
     for (int frame = 4096; frame <= 65536; frame <<= 1) {
        LRU_Node *head = NULL;
        LRU_Node *mid_node = NULL;
        LRU_Node *tail = NULL;
        CLEAN_Node *clean_head = NULL;
        CLEAN_Node *clean_tail = NULL;
        list<pair<int, LRU_Node*>> hash[HASH_SIZE];
        int hit = 0, miss = 0, write_back = 0, curr_frames = 0;
        int Working_Region = frame * 3 / 4;

        for (int i = 0; i < sz; ++i) {
            int page = addresses[i];
            char op = accessType[i];
            LRU_Node *curr = getNode(hash, page);

            if (curr) {
                hit++;
                if (op == 'W') {
                    curr->dirty = 1;
                }
                if (curr != head) {
                    if (curr->prev) curr->prev->next = curr->next;
                    if (curr->next) curr->next->prev = curr->prev;
                    else tail = curr->prev;

                    curr->next = head;
                    curr->prev = NULL;
                    if (head) head->prev = curr;
                    head = curr;
                }
            } else {
                miss++;
                if (curr_frames == frame) {
                    if (tail) {
                        if (tail->dirty) write_back++;
                        auto &bucket = hash[hash_func(tail->val)];
                        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
                            if (it->first == tail->val) {
                                bucket.erase(it);
                                break;
                            }
                        }

                        if (tail->prev) {
                            LRU_Node *old_tail = tail;
                            tail = tail->prev;
                            tail->next = NULL;
                            delete old_tail;
                        } else {
                            delete tail;
                            head = tail = NULL;
                        }
                        curr_frames--;
                    }
                }

                curr = new LRU_Node(page, op == 'W' ? 1 : 0, NULL, head);
                if (head) head->prev = curr;
                head = curr;
                if (!tail) tail = curr;

                hash[hash_func(page)].push_back(make_pair(page, curr));
                curr_frames++;
            }
        }

        double ratio = (double)miss / (double)(hit + miss);
        printf("%d\t%d\t%d\t\t%.10f\t\t%d\n", frame, hit, miss, ratio, write_back);

        while (head) {
            LRU_Node *temp = head;
            head = head->next;
            delete temp;
        }
    }

}

int main(int argc, char *argv[]) {
    return 0;
    if (argc < 2) {
        cerr << "請提供輸入檔案名稱！" << endl;
        return -1;
    }

    FILE *file = fopen(argv[1], "r");

    int capacity = INITIAL_CAPACITY;
    char *accessType = (char*)malloc(capacity * sizeof(char));
    unsigned int *addresses = (unsigned int*)malloc(capacity * sizeof(unsigned int));

    int sz = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char type;
        unsigned int address;

        if (sscanf(line, "%c %x", &type, &address) == 2) {
            if (sz >= capacity) {
                char *newAccessType = (char*)realloc(accessType, capacity * 2 * sizeof(char));
                unsigned int *newAddresses = (unsigned int*)realloc(addresses, capacity * 2 * sizeof(unsigned int));
                accessType = newAccessType;
                addresses = newAddresses;
                capacity <<= 1;
            }
            accessType[sz] = type;
            addresses[sz] = address / PAGE_SIZE;  
            sz++;
            if (sz % 1000000 == 0) {
                printf("已讀取 %d 筆數據\n", sz);
            }
        }
    }
    fclose(file);

    timeval start, end;
    cout << "LRU policy:\nFrame\tHit\t\tMiss\t\tPage fault ratio\tWrite back count\n";
    gettimeofday(&start, 0);
    LRU_simulate(accessType, addresses, sz);
    gettimeofday(&end, 0);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Total elapsed time %.6f sec\n", elapsed);
    // TODO: CFLRU
    cout << "CFLRU policy:\nFrame\tHit\t\tMiss\t\tPage fault ratio\tWrite back count\n";
    gettimeofday(&start, 0);
    CFLRU_simulate(accessType, addresses, sz);
    gettimeofday(&end, 0);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Total elapsed time %.6f sec\n", elapsed);

    free(accessType);
    free(addresses);
    return 0;
}
