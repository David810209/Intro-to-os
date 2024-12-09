/*
Student No.: 111550076
Student Name: 楊子賝
Email: zichen55.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/
#include <fstream>
#include <list>
#include <sys/time.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <chrono>

using namespace std;
#define INITIAL_CAPACITY 100000

#define HASH_SIZE 131101
#define hash_func(val) (val % HASH_SIZE)
#define PAGE_SIZE 4096
#define CLEAN_FIRST_REGION 1
#define WORKING_REGION 0

struct CLEAN_Node;

struct LRU_Node {
    int val;
    int dirty;
    bool region;
    CLEAN_Node* clean_node;
    LRU_Node *next, *prev;

    LRU_Node(int val, int dirty, LRU_Node *prev, LRU_Node *next, bool region = WORKING_REGION, CLEAN_Node* clean_node = NULL) 
        : val(val), dirty(dirty), region(region), clean_node(clean_node),prev(prev), next(next) {}
};

struct CLEAN_Node {
    LRU_Node* node;
    CLEAN_Node *next, *prev;
    CLEAN_Node(LRU_Node* node,  CLEAN_Node* prev = NULL, CLEAN_Node* next = NULL)
        : node(node), prev(prev), next(next) {}
};

LRU_Node* getNode(list<pair<int, LRU_Node*>>* hash, int page) {
    auto& bucket = hash[hash_func(page)];
    for (auto& entry : bucket) {
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
                    if (!tail) tail = curr;
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
        LRU_Node *work_head = NULL, *work_tail = NULL;
        LRU_Node *CF_head = NULL, *CF_tail = NULL;
        CLEAN_Node *clean_head = NULL, *clean_tail = NULL;
        list<pair<int, LRU_Node*>> hash[HASH_SIZE];
        int hit = 0, miss = 0, write_back = 0;
        int curr_frames = 0;
        int clean_list_cnt = 0, CF_cnt = 0;
        int Working_Region = frame - (frame >> 2);
        int working_frames = 0;

        for (int i = 0; i < sz; ++i) {
            int page = addresses[i];
            char op = accessType[i];
            LRU_Node* curr = getNode(hash, page);

            if (curr) {
                // Page hit
                hit++;
                if (op == 'W') curr->dirty = 1;
                if(curr == work_head) continue;
                // Move to Working Region MRU position
                if (curr->prev) curr->prev->next = curr->next;
                else if (curr == CF_head) CF_head = curr->next;
                if (curr->next) curr->next->prev = curr->prev;
                else {
                    // Update tail pointers
                    if (curr == work_tail) work_tail = curr->prev;
                    if (curr == CF_tail) CF_tail = curr->prev;
                }
                

                // Insert into Working Region head
                curr->next = work_head;
                if (work_head) work_head->prev = curr;
                work_head = curr;
                curr->prev = NULL;
                if (!work_tail) work_tail = curr;
                if(curr->region == WORKING_REGION) continue;
                
                if (curr->clean_node) {
                    // Remove from Clean List
                    if (curr->clean_node->prev)
                        curr->clean_node->prev->next = curr->clean_node->next;
                    else
                        clean_head = curr->clean_node->next;

                    if (curr->clean_node->next)
                        curr->clean_node->next->prev = curr->clean_node->prev;
                    else
                        clean_tail = curr->clean_node->prev;

                    delete curr->clean_node;
                    curr->clean_node = NULL;
                }

                curr->region = WORKING_REGION;
                // Update working_frames and adjust regions if necessary
                if (working_frames < Working_Region) {
                    working_frames++;
                } else {
                    // Move LRU page from Working Region to Clean First Region
                    if (work_tail) {
                        LRU_Node* temp = work_tail;
                        work_tail = work_tail->prev;
                        if (work_tail) work_tail->next = NULL;
                        else work_head = NULL;

                        // Insert into Clean First Region head
                        temp->next = CF_head;
                        if (CF_head) CF_head->prev = temp;
                        CF_head = temp;
                        temp->prev = NULL;
                        temp->region = CLEAN_FIRST_REGION;
                        if (!CF_tail) CF_tail = temp;

                        // If the page is clean and not in Clean List, add it
                        if (temp->dirty == 0) {
                            CLEAN_Node* clean_node = new CLEAN_Node(temp, NULL, clean_head);
                            if(clean_head) clean_head->prev = clean_node;
                            clean_head = clean_node;
                            if (!clean_tail) clean_tail = clean_node;
                            temp->clean_node = clean_node;
                        }
                    }
                }
            } 
            
            else {
                // Page miss
                miss++;
                LRU_Node* victim = NULL;

                if (curr_frames == frame) {
                    // Select victim from Clean List
                    if (clean_tail) {
                        clean_list_cnt++;
                        victim = clean_tail->node;

                        // Remove from Clean List
                        if (clean_tail->prev)
                            clean_tail->prev->next = NULL;
                        else
                            clean_head = NULL;
                        CLEAN_Node* temp_clean = clean_tail;
                        clean_tail = clean_tail->prev;
                        delete temp_clean;
                        victim->clean_node = NULL;

                        // Remove from Clean First Region
                        if (victim->prev) victim->prev->next = victim->next;
                        else CF_head = victim->next;
                        if (victim->next) victim->next->prev = victim->prev;
                        else CF_tail = victim->prev;

                    } else if (CF_tail) {
                        CF_cnt++;
                        victim = CF_tail;

                        // Remove from Clean First Region
                        if (CF_tail->prev) CF_tail->prev->next = NULL;
                        else CF_head = NULL;
                        CF_tail = CF_tail->prev;
                    } 

                    if (victim) {
                        // Write back if dirty
                        if (victim->dirty) write_back++;

                        // Remove from hash table
                        auto& bucket = hash[hash_func(victim->val)];
                        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
                            if (it->first == victim->val) {
                                bucket.erase(it);
                                break;
                            }
                        }
                        delete victim;
                        curr_frames--;
                    }
                }

                // Insert new page into Working Region head
                curr = new LRU_Node(page, op == 'W' ? 1 : 0, NULL, work_head, WORKING_REGION);
                if (work_head) work_head->prev = curr;
                work_head = curr;
                if (!work_tail) work_tail = curr;
                hash[hash_func(page)].push_back(make_pair(page, curr));
                curr_frames++;
                if (working_frames < Working_Region) {
                    working_frames++;
                } else {
                    // Move LRU page from Working Region to Clean First Region
                    if (work_tail) {
                        LRU_Node* temp = work_tail;
                        work_tail = work_tail->prev;
                        if (work_tail) work_tail->next = NULL;
                        else work_head = NULL;

                        // Insert into Clean First Region head
                        temp->next = CF_head;
                        if (CF_head) CF_head->prev = temp;
                        CF_head = temp;
                        temp->prev = NULL;
                        temp->region = CLEAN_FIRST_REGION;
                        if (!CF_tail) CF_tail = temp;

                        // If the page is clean and not in Clean List, add it
                        if (temp->dirty == 0) {
                            CLEAN_Node* clean_node = new CLEAN_Node(temp, NULL, clean_head);
                            if(clean_head) clean_head->prev = clean_node;
                            clean_head = clean_node;
                            if (!clean_tail) clean_tail = clean_node;
                            temp->clean_node = clean_node;
                        }
                    }
                }
            }
        }

        double ratio = (double)miss / (double)(hit + miss);
        printf("%d\t%d\t%d\t\t%.10f\t\t%d\n", frame, hit, miss, ratio, write_back);
        // printf("clean_list:%d, working:%d, CF:%d\n", clean_list_cnt, working_cnt, CF_cnt);
        // printf("work frames:%d\n", working_frames);
        // Clean up memory
        while (work_head) {
            LRU_Node* temp = work_head;
            work_head = work_head->next;
            delete temp;
        }
        while (CF_head) {
            LRU_Node* temp = CF_head;
            CF_head = CF_head->next;
            delete temp;
        }
        while (clean_head) {
            CLEAN_Node* temp = clean_head;
            clean_head = clean_head->next;
            delete temp;
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "請提供輸入檔案名稱！" << endl;
        return -1;
    }

    // FILE *file = fopen(argv[1], "r");
    // if (!file) {
    //     cerr << "無法打開檔案！" << endl;
    //     return -1;
    // }

    int capacity = INITIAL_CAPACITY;
    char *accessType = (char*)malloc(capacity * sizeof(char));
    unsigned int *addresses = (unsigned int*)malloc(capacity * sizeof(unsigned int));

    int sz = 0;
    string trace_file = argv[1];
    ifstream infile(trace_file);
    string line;
    while(getline(infile, line)){
        if (sz >= capacity) {
            capacity <<= 1;
            if(capacity > 100000005) capacity = 100000005;
            accessType = (char*)realloc(accessType, capacity * sizeof(char));
            addresses = (unsigned int*)realloc(addresses, capacity * sizeof(unsigned int));
        }
        accessType[sz] = line[0];
        addresses[sz] = stoul(line.substr(2), nullptr, 16) / PAGE_SIZE;
        sz++;
        // if (sz % 10000000 == 0) {
        //     printf("已讀取 %d 筆數據\n", sz);
        // }
    }


    timeval start, end;
    cout << "LRU policy:\nFrame\tHit\t\tMiss\t\tPage fault ratio\tWrite back count\n";
    gettimeofday(&start, 0);
    LRU_simulate(accessType, addresses, sz);
    gettimeofday(&end, 0);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Elapsed time: %.6f sec\n\n", elapsed);

    cout << "CFLRU policy:\nFrame\tHit\t\tMiss\t\tPage fault ratio\tWrite back count\n";
    gettimeofday(&start, 0);
    CFLRU_simulate(accessType, addresses, sz);
    gettimeofday(&end, 0);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Elapsed time: %.6f sec\n\n", elapsed);

    free(accessType);
    free(addresses);
    return 0;
}
