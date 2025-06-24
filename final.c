#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#define M 3 // Order: Max children = M, Max keys = M-1
#define MAX_SPACES 50
#define MAX_VEHICLES 150

typedef struct Vehicle {
    char v_num[20];
    char owner[50];
    char arr_date[11];
    char arr_time[5];
    char dep_date[11];
    char dep_time[5];
    int  membership;
    float total_hrs;
    int space_id;
    int parks;
    float revenue;
} Vehicle;

typedef struct ParkingSpace {
    int id;
    int status;
    float revenue;
    float hrs;
} ParkingSpace;

typedef struct BPlusTreeNode {
    int nkeys;
    bool leaf_flag;
    struct BPlusTreeNode *parent;

    union {
        struct {
            char int_vkeys[M - 1][20];       
            int int_skeys[M - 1];          
            struct BPlusTreeNode *child[M]; 
        } internal;

        struct {
            Vehicle leaf_v[M - 1];         
            ParkingSpace leaf_s[M - 1];    
            struct BPlusTreeNode *next;   
            struct BPlusTreeNode *prev;    
        } leaf;
    } data;

} BPlusTreeNode;

BPlusTreeNode *v_root = NULL;
BPlusTreeNode *s_root = NULL;

void swapElements(void *a, void *b, int size) {
    char *p1 = (char *)a;
    char *p2 = (char *)b;
    char temp;
    for (int i = 0; i < size; i++) {
        temp = p1[i];
        p1[i] = p2[i];
        p2[i] = temp;
    }
}

int partition(void *base, int low, int high, int size, int (*compare)(const void*, const void*)) {
    void *pivot = (char *)base + high * size;
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        void *current = (char *)base + j * size;
        if (compare(current, pivot) <= 0) {
            i++;
            swapElements((char *)base + i * size, current, size);
        }
    }
    swapElements((char *)base + (i + 1) * size, (char *)base + high * size, size);
    return (i + 1);
}

void quickSortRecursive(void *base, int low, int high, int size, int (*compare)(const void*, const void*)) {
    if (low < high) {

        int pi = partition(base, low, high, size, compare);
        quickSortRecursive(base, low, pi - 1, size, compare);
        quickSortRecursive(base, pi + 1, high, size, compare);
    }
}

void quickSort(void *base, int num, int size, int (*compare)(const void*, const void*)) {
    if (num > 1) {
        quickSortRecursive(base, 0, num - 1, size, compare);
    }
}

Vehicle* findVehicle(BPlusTreeNode *node, const char *v_num);
ParkingSpace* findSpace(BPlusTreeNode *node, int s_id);
BPlusTreeNode* findLeafNodeV(BPlusTreeNode* node, const char* v_num);
BPlusTreeNode* findLeafNodeS(BPlusTreeNode* node, int s_id);
void insertVehicle(Vehicle v);
void insertSpace(ParkingSpace s);
void collectVehicles(BPlusTreeNode *node, Vehicle *v_arr, int *cnt);
void collectSpaces(BPlusTreeNode *node, ParkingSpace *s_arr, int *cnt);
void saveDataAndFree();
int compareSpacesByID(const void *a, const void *b);
void loadSpaces();
void loadVehicles();

BPlusTreeNode *createNode(bool is_leaf) {
    BPlusTreeNode *node = (BPlusTreeNode *)calloc(1, sizeof(BPlusTreeNode));
    if (node) {
       node->leaf_flag = is_leaf;
    } else {
       perror("Failed to allocate node in createNode");

    }
    return node;
}


BPlusTreeNode* findLeafNodeV(BPlusTreeNode* node, const char* v_num) {
    BPlusTreeNode* result = NULL;
    if (node) {
        BPlusTreeNode* curr = node;
        bool error_occurred = false;
        while (!curr->leaf_flag && !error_occurred) {
            int i = 0;
            while (i < curr->nkeys && strcmp(curr->data.internal.int_vkeys[i], v_num) <= 0) {
                i++;
            }

            if (i <= curr->nkeys && curr->data.internal.child[i]) {
                curr = curr->data.internal.child[i];
            } else {

                error_occurred = true;

            }
        }


        if (!error_occurred && curr->leaf_flag) {
            result = curr;
        }

    }


    return result;
}


BPlusTreeNode* findLeafNodeS(BPlusTreeNode* node, int s_id) {
    BPlusTreeNode* result = NULL;
    if (node) {
        BPlusTreeNode* curr = node;
        bool error_occurred = false;
        while (!curr->leaf_flag && !error_occurred) {
            int i = 0;
            while (i < curr->nkeys && curr->data.internal.int_skeys[i] <= s_id) {
                i++;
            }
             if (i <= curr->nkeys && curr->data.internal.child[i]) {
                 curr = curr->data.internal.child[i];
             } else {

                error_occurred = true;

             }
        }

        if (!error_occurred && curr->leaf_flag) {
             result = curr;
        }

    }


    return result;
}

void insertIntoLeafV(BPlusTreeNode* l_node, Vehicle v);
void insertIntoParentV(BPlusTreeNode* left, const char* k, BPlusTreeNode* right);
void insertIntoLeafS(BPlusTreeNode* l_node, ParkingSpace s);
void insertIntoParentS(BPlusTreeNode* left, int k, BPlusTreeNode* right);


void insertIntoLeafV(BPlusTreeNode* l_node, Vehicle v) {
    int i = l_node->nkeys - 1;
    while (i >= 0 && strcmp(l_node->data.leaf.leaf_v[i].v_num, v.v_num) > 0) {
        l_node->data.leaf.leaf_v[i + 1] = l_node->data.leaf.leaf_v[i];
        i--;
    }
    l_node->data.leaf.leaf_v[i + 1] = v;
    l_node->nkeys++;
}


void insertIntoParentV(BPlusTreeNode* left, const char* k, BPlusTreeNode* right) {
    if (!left) {
        fprintf(stderr, "Error: insertIntoParentV called with NULL left child.\n");
    } else {
        BPlusTreeNode* p = left->parent;
        bool processed = false;

        if (p == NULL) {
            BPlusTreeNode* new_root_node = createNode(false);

            if (new_root_node) {
                strcpy(new_root_node->data.internal.int_vkeys[0], k);
                new_root_node->data.internal.child[0] = left;
                new_root_node->data.internal.child[1] = right;
                new_root_node->nkeys = 1;
                if(left) left->parent = new_root_node;
                if(right) right->parent = new_root_node;
                v_root = new_root_node;
            } else {
                 fprintf(stderr, "Error: Failed to allocate new root node in insertIntoParentV.\n");
            }
            processed = true;

        }

        if (!processed && p->nkeys < M - 1) {
            int i = p->nkeys - 1;
            while (i >= 0 && strcmp(p->data.internal.int_vkeys[i], k) > 0) {
                strcpy(p->data.internal.int_vkeys[i + 1], p->data.internal.int_vkeys[i]);
                p->data.internal.child[i + 2] = p->data.internal.child[i + 1];
                i--;
            }
            strcpy(p->data.internal.int_vkeys[i + 1], k);
            p->data.internal.child[i + 2] = right;
            if (right) right->parent = p;
            p->nkeys++;
            processed = true;
        }

        if (!processed) {
            BPlusTreeNode* new_node = createNode(false);
            if (!new_node) {

                 fprintf(stderr, "Error: Failed to allocate new internal node in insertIntoParentV.\n");
                 processed = true;
            } else {
                char tmp_k[M][20]; BPlusTreeNode* tmp_c[M + 1]; int i = 0, j = 0;


                while (i < p->nkeys && strcmp(p->data.internal.int_vkeys[i], k) < 0) {
                    strcpy(tmp_k[j], p->data.internal.int_vkeys[i]); tmp_c[j] = p->data.internal.child[i]; i++; j++;
                }
                strcpy(tmp_k[j], k); tmp_c[j] = p->data.internal.child[i]; tmp_c[j + 1] = right; j++;
                while (i < p->nkeys) {
                     strcpy(tmp_k[j], p->data.internal.int_vkeys[i]); tmp_c[j + 1] = p->data.internal.child[i + 1]; i++; j++;
                }
                int split_idx = M / 2; char up_key[20]; strcpy(up_key, tmp_k[split_idx]);


                p->nkeys = split_idx;
                memset(p->data.internal.int_vkeys, 0, sizeof(p->data.internal.int_vkeys));
                memset(p->data.internal.child, 0, sizeof(p->data.internal.child));
                for(i=0; i < p->nkeys; ++i) {
                    strcpy(p->data.internal.int_vkeys[i], tmp_k[i]);
                    p->data.internal.child[i] = tmp_c[i];
                    if(p->data.internal.child[i]) p->data.internal.child[i]->parent = p;
                }
                p->data.internal.child[p->nkeys] = tmp_c[split_idx];
                if(p->data.internal.child[p->nkeys]) p->data.internal.child[p->nkeys]->parent = p;

                new_node->nkeys = M - 1 - p->nkeys;
                new_node->parent = p->parent;
                memset(new_node->data.internal.int_vkeys, 0, sizeof(new_node->data.internal.int_vkeys));
                memset(new_node->data.internal.child, 0, sizeof(new_node->data.internal.child));
                for(i=0, j=split_idx + 1; i < new_node->nkeys; ++i, ++j) {
                    strcpy(new_node->data.internal.int_vkeys[i], tmp_k[j]);
                    new_node->data.internal.child[i] = tmp_c[j];
                    if(new_node->data.internal.child[i]) new_node->data.internal.child[i]->parent = new_node;
                }
                new_node->data.internal.child[new_node->nkeys] = tmp_c[M];
                if(new_node->data.internal.child[new_node->nkeys]) new_node->data.internal.child[new_node->nkeys]->parent = new_node;


                for(i = p->nkeys + 1; i <= M; ++i) p->data.internal.child[i] = NULL;
                for(i = new_node->nkeys + 1; i <= M; ++i) new_node->data.internal.child[i] = NULL;


                insertIntoParentV(p, up_key, new_node);

            }
        }
    }

}


void insertVehicle(Vehicle v) {
    bool proceed = true;

    if (v_root == NULL) {
        v_root = createNode(true);
        if (v_root) {
            v_root->data.leaf.leaf_v[0] = v;
            v_root->nkeys = 1;
        } else {
            fprintf(stderr, "Error: Failed to create root node for vehicles.\n");
        }
        proceed = false;
    }

    BPlusTreeNode* l_node = NULL;
    if (proceed) {
        l_node = findLeafNodeV(v_root, v.v_num);
        if (!l_node) {
            fprintf(stderr, "  insertV: Failed find leaf for %s\n", v.v_num);
            proceed = false;

        }
    }

    bool found = false;
    if (proceed) {
        int i = 0;
        while(i < l_node->nkeys && !found) {
             if (strcmp(l_node->data.leaf.leaf_v[i].v_num, v.v_num) == 0) {
                 l_node->data.leaf.leaf_v[i] = v;
                 found = true;
             }
             i++;
         }
        if(found) {
            proceed = false;
        }
    }

    if (proceed) {
        if (l_node->nkeys < M - 1) {
            insertIntoLeafV(l_node, v);
        } else {
            BPlusTreeNode* new_l = createNode(true);
            if (!new_l) {
                 printf("Failed alloc new_leaf!\n");

                 proceed = false;
            }

            if (proceed) {
                Vehicle tmp_v[M];
                int i = 0, j = 0;

                while (i < l_node->nkeys && strcmp(l_node->data.leaf.leaf_v[i].v_num, v.v_num) < 0) {
                    tmp_v[j++] = l_node->data.leaf.leaf_v[i++];
                }
                tmp_v[j++] = v;
                while (i < l_node->nkeys) {
                    tmp_v[j++] = l_node->data.leaf.leaf_v[i++];
                }
                int split_pt = (int)ceil((double)M / 2.0);


                l_node->nkeys = split_pt;
                memset(l_node->data.leaf.leaf_v, 0, sizeof(l_node->data.leaf.leaf_v));
                for (i = 0; i < l_node->nkeys; i++) { l_node->data.leaf.leaf_v[i] = tmp_v[i]; }

                new_l->nkeys = M - split_pt;
                memset(new_l->data.leaf.leaf_v, 0, sizeof(new_l->data.leaf.leaf_v));
                for (i = 0, j = split_pt; i < new_l->nkeys; i++, j++) { new_l->data.leaf.leaf_v[i] = tmp_v[j]; }


                new_l->data.leaf.next = l_node->data.leaf.next;
                if (l_node->data.leaf.next) { l_node->data.leaf.next->data.leaf.prev = new_l; }
                l_node->data.leaf.next = new_l;
                new_l->data.leaf.prev = l_node;
                new_l->parent = l_node->parent;


                insertIntoParentV(l_node, new_l->data.leaf.leaf_v[0].v_num, new_l);
            }
        }
    }

}


void insertIntoLeafS(BPlusTreeNode* l_node, ParkingSpace s) {
    int i = l_node->nkeys - 1;
    while (i >= 0 && l_node->data.leaf.leaf_s[i].id > s.id) {
        l_node->data.leaf.leaf_s[i + 1] = l_node->data.leaf.leaf_s[i];
        i--;
    }
    l_node->data.leaf.leaf_s[i + 1] = s;
    l_node->nkeys++;
}

void insertIntoParentS(BPlusTreeNode* left, int k, BPlusTreeNode* right) {
     if (!left) {
         fprintf(stderr, "Error: insertIntoParentS called with NULL left child.\n");
     } else {
         BPlusTreeNode* p = left->parent;
         bool processed = false;

        if (p == NULL) {
            BPlusTreeNode* new_root_node = createNode(false);
            if (new_root_node) { // Check allocation
                new_root_node->data.internal.int_skeys[0] = k;
                new_root_node->data.internal.child[0] = left;
                new_root_node->data.internal.child[1] = right;
                new_root_node->nkeys = 1;
                if (left) left->parent = new_root_node;
                if (right) right->parent = new_root_node;
                s_root = new_root_node;
            } else {
                fprintf(stderr, "Error: Failed to allocate new root node in insertIntoParentS.\n");
            }
            processed = true;

        }

        if (!processed && p->nkeys < M - 1) {
            int i = p->nkeys - 1;
            while (i >= 0 && p->data.internal.int_skeys[i] > k) {
                p->data.internal.int_skeys[i + 1] = p->data.internal.int_skeys[i];
                p->data.internal.child[i + 2] = p->data.internal.child[i + 1];
                i--;
            }
            p->data.internal.int_skeys[i + 1] = k;
            p->data.internal.child[i + 2] = right;
            if (right) right->parent = p;
            p->nkeys++;
            processed = true;
        }

        if (!processed) {
            BPlusTreeNode* new_node = createNode(false);
            if (!new_node) {
                 fprintf(stderr, "Error: Failed to allocate new internal node in insertIntoParentS.\n");

                 processed = true;
            } else {

                int tmp_k[M]; BPlusTreeNode* tmp_c[M + 1]; int i = 0, j = 0;


                while (i < p->nkeys && p->data.internal.int_skeys[i] < k) {
                    tmp_k[j] = p->data.internal.int_skeys[i]; tmp_c[j] = p->data.internal.child[i]; i++; j++;
                }
                tmp_k[j] = k; tmp_c[j] = p->data.internal.child[i]; tmp_c[j + 1] = right; j++;
                while (i < p->nkeys) {
                     tmp_k[j] = p->data.internal.int_skeys[i]; tmp_c[j + 1] = p->data.internal.child[i + 1]; i++; j++;
                }


                int split_idx = M / 2;
                int up_key = tmp_k[split_idx];


                p->nkeys = split_idx;
                memset(p->data.internal.int_skeys, 0, sizeof(p->data.internal.int_skeys));
                memset(p->data.internal.child, 0, sizeof(p->data.internal.child));
                for(i=0; i < p->nkeys; ++i) {
                    p->data.internal.int_skeys[i] = tmp_k[i];
                    p->data.internal.child[i] = tmp_c[i];
                     if(p->data.internal.child[i]) p->data.internal.child[i]->parent = p;
                }
                p->data.internal.child[p->nkeys] = tmp_c[split_idx];
                if(p->data.internal.child[p->nkeys]) p->data.internal.child[p->nkeys]->parent = p;


                new_node->nkeys = M - 1 - p->nkeys;
                new_node->parent = p->parent;
                memset(new_node->data.internal.int_skeys, 0, sizeof(new_node->data.internal.int_skeys));
                memset(new_node->data.internal.child, 0, sizeof(new_node->data.internal.child));
                for(i=0, j=split_idx + 1; i < new_node->nkeys; ++i, ++j) {
                    new_node->data.internal.int_skeys[i] = tmp_k[j];
                    new_node->data.internal.child[i] = tmp_c[j];
                    if(new_node->data.internal.child[i]) new_node->data.internal.child[i]->parent = new_node;
                }
                new_node->data.internal.child[new_node->nkeys] = tmp_c[M];
                if(new_node->data.internal.child[new_node->nkeys]) new_node->data.internal.child[new_node->nkeys]->parent = new_node;


                for(i = p->nkeys + 1; i <= M; ++i) p->data.internal.child[i] = NULL;
                for(i = new_node->nkeys + 1; i <= M; ++i) new_node->data.internal.child[i] = NULL;


                insertIntoParentS(p, up_key, new_node);
            }
        }
     }

}


void insertSpace(ParkingSpace s) {
    bool proceed = true;

    if (s_root == NULL) {
        s_root = createNode(true);
        if (s_root) {
            s_root->data.leaf.leaf_s[0] = s;
            s_root->nkeys = 1;
        } else {
            fprintf(stderr, "Error: Failed to create root node for spaces.\n");
        }
        proceed = false;
    }

    BPlusTreeNode* l_node = NULL;
    if (proceed) {
        l_node = findLeafNodeS(s_root, s.id);
         if (!l_node) {
            fprintf(stderr, "Err: Cannot find leaf for space %d\n", s.id);
            proceed = false;
        }
    }

     bool found = false;
     if (proceed) {
         int i = 0;
         while (i < l_node->nkeys && !found) {
             if (l_node->data.leaf.leaf_s[i].id == s.id) {
                 l_node->data.leaf.leaf_s[i] = s;
                 found = true;
                  }
             i++;
         }
         if(found) {
             proceed = false;
         }
     }

    if (proceed) {
		if (l_node->nkeys < M - 1) {
            insertIntoLeafS(l_node, s);
        } else {
            BPlusTreeNode* new_l = createNode(true);
             if (!new_l) {
                 fprintf(stderr, "Error: Failed to allocate new leaf node in insertSpace.\n");
                 proceed = false;
             }

             if (proceed) {
                ParkingSpace tmp_s[M];
                int i = 0, j = 0;
                while (i < l_node->nkeys && l_node->data.leaf.leaf_s[i].id < s.id) {
                    tmp_s[j++] = l_node->data.leaf.leaf_s[i++];
                }
                tmp_s[j++] = s;
                while (i < l_node->nkeys) {
                    tmp_s[j++] = l_node->data.leaf.leaf_s[i++];
                }

                int split_pt = (int)ceil((double)M / 2.0);
                l_node->nkeys = split_pt;
                memset(l_node->data.leaf.leaf_s, 0, sizeof(l_node->data.leaf.leaf_s));
                for (i = 0; i < l_node->nkeys; i++) { l_node->data.leaf.leaf_s[i] = tmp_s[i]; }

                new_l->nkeys = M - split_pt;
                memset(new_l->data.leaf.leaf_s, 0, sizeof(new_l->data.leaf.leaf_s));
                for (i = 0, j = split_pt; i < new_l->nkeys; i++, j++) { new_l->data.leaf.leaf_s[i] = tmp_s[j]; }

                new_l->data.leaf.next = l_node->data.leaf.next;
                 if (l_node->data.leaf.next) { l_node->data.leaf.next->data.leaf.prev = new_l; }
                l_node->data.leaf.next = new_l;
                new_l->data.leaf.prev = l_node;
                new_l->parent = l_node->parent;

                insertIntoParentS(l_node, new_l->data.leaf.leaf_s[0].id, new_l);
            }
        }
    }

}

Vehicle* findVehicle(BPlusTreeNode *node, const char *v_num) {
    Vehicle* result = NULL; 
    BPlusTreeNode* l_node = findLeafNodeV(node, v_num);

    if (l_node) { 
        bool found = false; 
        for (int i = 0; i < l_node->nkeys && !found; i++) {
            if (strcmp(l_node->data.leaf.leaf_v[i].v_num, v_num) == 0) {
                result = &l_node->data.leaf.leaf_v[i];
                found = true; 
                
            }
        }
        
    }
    

    return result; // Single return statement
}

ParkingSpace* findSpace(BPlusTreeNode *node, int s_id) {
    ParkingSpace* result = NULL; 
    BPlusTreeNode* l_node = findLeafNodeS(node, s_id);

    if (l_node) { 
        bool found = false; 
        for (int i = 0; i < l_node->nkeys && !found; i++) {
            if (l_node->data.leaf.leaf_s[i].id == s_id) {
                result = &l_node->data.leaf.leaf_s[i];
                found = true;
                
            }
        }
        
    }
    

    return result; 
}

int allocateSpace(int membership) {
    int allocated_space_id = -1;
	int start, end;
    if (membership == 2) { start = 1; end = 10; }
    else if (membership == 1) { start = 11; end = 20; }
    else { start = 21; end = MAX_SPACES; }

    bool space_found = false;
    for (int i = start; i <= end && !space_found; i++) {
        ParkingSpace *sp = findSpace(s_root, i);
        if (sp && sp->status == 0) {
            sp->status = 1;
            printf("Allocated space %d (membership: %d)\n", i, membership);
            allocated_space_id = i;
            space_found = true;
        }
    }

    if (!space_found) {
         printf("Err: No available space.\n");
    }

    return allocated_space_id;
}

float calcHours(const char *arr_dt, const char *arr_tm,
                const char *dep_dt, const char *dep_tm) {
    struct tm arr = {0}, dep = {0};
    int ad, am, ay, ah, amin;
    int dd, dm, dy, dh, dmin;
    if (!arr_dt || !arr_tm || !dep_dt || !dep_tm ||
        strlen(arr_dt) == 0 || strlen(arr_tm) == 0 ||
        strlen(dep_dt) == 0 || strlen(dep_tm) == 0 ||
        strcmp(arr_dt, "-") == 0 || strcmp(arr_tm, "-") == 0 ||
        strcmp(dep_dt, "-") == 0 || strcmp(dep_tm, "-") == 0)
    {
         return 0.0;
    }

    if (sscanf(arr_dt, "%2d%2d%4d", &ad, &am, &ay) != 3 ||
        sscanf(arr_tm, "%2d%2d", &ah, &amin) != 2 ||
        sscanf(dep_dt, "%2d%2d%4d", &dd, &dm, &dy) != 3 ||
        sscanf(dep_tm, "%2d%2d", &dh, &dmin) != 2) {
        return 0.0;
    }
    arr.tm_mday = ad; arr.tm_mon = am - 1; arr.tm_year = ay - 1900;
    arr.tm_hour = ah; arr.tm_min = amin; arr.tm_isdst = -1;
    dep.tm_mday = dd; dep.tm_mon = dm - 1; dep.tm_year = dy - 1900;
    dep.tm_hour = dh; dep.tm_min = dmin; dep.tm_isdst = -1;


    time_t t_arr = mktime(&arr);
    time_t t_dep = mktime(&dep);

    if (t_arr == (time_t)-1 || t_dep == (time_t)-1) {
        return 0.0;
    }
    if (t_dep < t_arr) {

         return 0.0;
    }


    double diff_sec = difftime(t_dep, t_arr);
    return (float)(diff_sec / 3600.0);
}

float calcPay(float hrs, int membership) {
     if (hrs < 0) hrs = 0;
    float rate;
    float discount = 1.0;

    if (hrs <= 3.0) {
        rate = 100.0;
    } else {
         float extra_hrs = hrs - 3.0;
         rate = 100.0 + extra_hrs * 50.0;
    }

    if (membership == 1) discount = 0.9; // Premium
    else if (membership == 2) discount = 0.8; // Gold

    return rate * discount;
}

void checkMembership(Vehicle *v) {
    if (v) {
        int new_mem = 0;
        if (v->total_hrs >= 200.0) new_mem = 2; // Gold
        else if (v->total_hrs >= 100.0) new_mem = 1; // Premium

        if (new_mem > v->membership) {
             printf("Membership Upgraded! V# %s is now %s (%.2f hrs).\n",
                   v->v_num, (new_mem == 2 ? "Gold" : "Premium"), v->total_hrs);
            v->membership = new_mem;
        }
    }


}

void loadSpaces() {
    printf("Init parking spaces...\n");
    FILE *fp = fopen("parking-lot-data.txt", "r");
    int loaded_count = 0;
    bool loaded_ids[MAX_SPACES + 1] = {false};

    if (fp) {

        int s_id, s_stat; float s_rev, s_hrs; char line[100];

        while (fgets(line, sizeof(line), fp)) {

            if (line[0] != '\n' && line[0] != '#') {
                if (sscanf(line, "%d %d %f %f", &s_id, &s_stat, &s_hrs, &s_rev) == 4) {
                    if (s_id > 0 && s_id <= MAX_SPACES) {
                        ParkingSpace s = {s_id, s_stat, s_rev, s_hrs};
                        insertSpace(s);
                        loaded_ids[s_id] = true;
                        loaded_count++;
                    }
                }
            }
        }
        fclose(fp);
        printf("Loaded/Updated %d spaces from file.\n", loaded_count); 
    } else {
         printf("Info:parking-lot-data.txt not found.\n");
    }


}

void loadVehicles() {
    FILE *fp = fopen("complete-vehicle-database-100.txt", "r");
    if (!fp) {
        printf("Info: complete-vehicle-database-100.txt not found.\n"); 

    } else {
        printf("Loading vehicles...\n");
        char line[256]; int count = 0; int skipped = 0;

        while (fgets(line, sizeof(line), fp)) {
           if (line[0] != '\n' && line[0] != '#') {

                Vehicle v; memset(&v, 0, sizeof(Vehicle));
                char vn[20], own[50], ad[11], at[5], dd[11], dt[5];
                int membership, sid, np; float th, tr;

                int parsed = sscanf(line, "%19s %49[^0-9-]%10s %4s %10s %4s %d %f %d %d %f",
                                    vn, own, ad, at, dd, dt, &membership, &th, &sid, &np, &tr);

                char *end = own + strlen(own) - 1;
                while(end > own && isspace((unsigned char)*end)) end--;
                *(end + 1) = '\0';

                if (parsed == 11) {
                     if (strlen(vn) > 0) {
                         strcpy(v.v_num, vn); strcpy(v.owner, own);
                         strcpy(v.arr_date, (strcmp(ad, "-") == 0 ? "" : ad));
                         strcpy(v.arr_time, (strcmp(at, "-") == 0 ? "" : at));
                         strcpy(v.dep_date, (strcmp(dd, "-") == 0 ? "" : dd));
                         strcpy(v.dep_time, (strcmp(dt, "-") == 0 ? "" : dt));
                         v.membership = membership; v.total_hrs = th; v.space_id = sid; v.parks = np; v.revenue = tr;

                         insertVehicle(v);
                         count++;
                         if (v.space_id > 0 && strlen(v.dep_date) == 0) {
                             ParkingSpace *sp = findSpace(s_root, v.space_id);
                             if (sp) {
                                 if (sp->status == 0) {
								 	sp->status = 1;
                                 }
                             } else {
                                fprintf(stderr, "Warn: Loaded vehicle %s refers to non-existent space %d.\n", v.v_num, v.space_id);
                             }
                         }
                     } else {

                         skipped++;

                     }
                } else {
                   skipped++;

                }
            }
        }
        if (ferror(fp)) {
             perror("Err reading vehicle file");
        }
        printf("Loaded/Updated %d vehicles.", count);
        if (skipped > 0) printf(" Skipped %d lines.", skipped);
        printf("\n");
        fclose(fp);
    }

}


void vehicleEntry(const char *v_num, const char *owner) {
    time_t now; struct tm *local_tm;
    char date_str[11]; char time_str[5];
    bool success = true;

    time(&now); local_tm = localtime(&now);
    if (local_tm == NULL) {
        perror("localtime err");

        strcpy(date_str, "00000000"); strcpy(time_str, "0000");
        fprintf(stderr, "Warning: Could not get current time for vehicle entry.\n");

    } else {
        strftime(date_str, sizeof(date_str), "%d%m%Y", local_tm);
        strftime(time_str, sizeof(time_str), "%H%M", local_tm);
    }

    Vehicle* ev = findVehicle(v_root, v_num);

    if (ev) {
        printf("Welcome back, %s (%s)!\n", owner, v_num);
        if (ev->space_id > 0 && strlen(ev->dep_date) == 0) {
             printf("Err: Vehicle %s already parked in %d.\n", v_num, ev->space_id);
             success = false;
        }

        if (success) {
            strcpy(ev->owner, owner);
            strcpy(ev->arr_date, date_str); strcpy(ev->arr_time, time_str);
            strcpy(ev->dep_date, ""); strcpy(ev->dep_time, "");

            int alloc_sp = allocateSpace(ev->membership);
            if (alloc_sp == -1) {
                printf("Sorry %s, no space available for %s.\n", owner, v_num);

                 strcpy(ev->arr_date, ""); strcpy(ev->arr_time, "");
                 success = false;
            } else {
                ev->space_id = alloc_sp;
                printf("V# %s assigned space %d on %s @ %s.\n", v_num, alloc_sp, date_str, time_str);
            }
        }

    } else {
        printf("Registering new vehicle: %s (%s)\n", owner, v_num);
        Vehicle nv = {0};
        strcpy(nv.v_num, v_num); strcpy(nv.owner, owner);
        strcpy(nv.arr_date, date_str); strcpy(nv.arr_time, time_str);
        nv.membership = 0;
        int alloc_sp = allocateSpace(nv.membership);
         if (alloc_sp == -1) {
            printf("Sorry %s, no space available for new vehicle %s.\n", owner, v_num);
            success = false;
        } else {
            nv.space_id = alloc_sp;
            insertVehicle(nv); // Add the new vehicle to the tree
            printf("New V# %s registered, assigned space %d on %s @ %s.\n", v_num, alloc_sp, date_str, time_str);
        }
    }

}


void vehicleExit(const char *v_num) {
    time_t now; struct tm *local_tm;
    char dep_date_str[11]; char dep_time_str[5];
    bool proceed = true;

    time(&now); local_tm = localtime(&now);
    if (local_tm == NULL) {
        perror("localtime err");
        fprintf(stderr, "Err getting time for exit %s.\n", v_num);
        proceed = false;
    } else {
        strftime(dep_date_str, sizeof(dep_date_str), "%d%m%Y", local_tm);
        strftime(dep_time_str, sizeof(dep_time_str), "%H%M", local_tm);
    }

    Vehicle *v = NULL;
    if (proceed) {
        v = findVehicle(v_root, v_num);
        if (!v) {
            printf("Err: Vehicle %s not found.\n", v_num);
            proceed = false;
        }
    }

    if (proceed) {
        if (v->space_id <= 0 || strlen(v->dep_date) > 0) {
            printf("Err: Vehicle %s not parked.\n", v_num);
            proceed = false; // Corrected: Added proceed = false;
        }
    }


    if (proceed) {
        if (strlen(v->arr_date) == 0 || strlen(v->arr_time) == 0 || strcmp(v->arr_date, "-") == 0 || strcmp(v->arr_time, "-") == 0) {
             printf("Err: V# %s has bad arrival data (%s %s).\n", v_num, v->arr_date, v->arr_time);
			 proceed = false;

        }
    }

    if (proceed) {
        int sp_id = v->space_id;

        float sess_hrs = calcHours(v->arr_date, v->arr_time, dep_date_str, dep_time_str);
        if (sess_hrs < 0) {
            printf("Err calculating hours (<0). Check times.\n");
            proceed = false;
        }

        if (proceed) {
            float sess_pay = calcPay(sess_hrs, v->membership);
		    printf("V# %s exiting space %d on %s @ %s.\n", v_num, sp_id, dep_date_str, dep_time_str);
		    printf("  Arr: %s %s\n", v->arr_date, v->arr_time);
		    printf("  Session: %.2f hrs, Pay: %.2f\n", sess_hrs, sess_pay);

		    strcpy(v->dep_date, dep_date_str); strcpy(v->dep_time, dep_time_str);
		    v->total_hrs += sess_hrs; v->revenue += sess_pay; v->parks += 1;
		    v->space_id = 0;

		    checkMembership(v);

		    printf("  Updated Totals: %.2f hrs, %.2f rev, %d parks, membership: %d\n",
		           v->total_hrs, v->revenue, v->parks, v->membership);

		    ParkingSpace *sp = findSpace(s_root, sp_id);
		    if (sp) {
		        if (sp->status == 0) printf("Warn: Space %d was already free for V# %s exit.\n", sp_id, v_num);
		        sp->status = 0; sp->revenue += sess_pay; sp->hrs += sess_hrs;
		         printf("  Space %d freed. Updated Space: %.2f hrs, %.2f rev.\n", sp_id, sp->hrs, sp->revenue);
		    } else {
		        fprintf(stderr, "CRITICAL Err: Cannot find space %d to free!\n", sp_id);
		    }
        }
    }

}


int compareVByHrs(const void *a, const void *b) {
    int result = 0;
    float diff = ((Vehicle *)b)->total_hrs - ((Vehicle *)a)->total_hrs;
    if (diff > 1e-6) result = 1;
    else if (diff < -1e-6) result = -1;
    else result = strcmp(((Vehicle *)a)->v_num, ((Vehicle *)b)->v_num);
    return result;
}
int compareVByRev(const void *a, const void *b) {
    int result = 0;
    float diff = ((Vehicle *)b)->revenue - ((Vehicle *)a)->revenue;
     if (diff > 1e-6) result = 1;
     else if (diff < -1e-6) result = -1;
     else result = strcmp(((Vehicle *)a)->v_num, ((Vehicle *)b)->v_num);
     return result;
}
int compareSByHrs(const void *a, const void *b) {
    int result = 0;
    float diff = ((ParkingSpace *)b)->hrs - ((ParkingSpace *)a)->hrs;
     if (diff > 1e-6) result = 1;
     else if (diff < -1e-6) result = -1;
     else result = ((ParkingSpace *)a)->id - ((ParkingSpace *)b)->id;
     return result;
}
int compareSByRev(const void *a, const void *b) {
    int result = 0;
    float diff = ((ParkingSpace *)b)->revenue - ((ParkingSpace *)a)->revenue;
     if (diff > 1e-6) result = 1;
     else if (diff < -1e-6) result = -1;
     else result = ((ParkingSpace *)a)->id - ((ParkingSpace *)b)->id;
     return result;
}
int compareSpacesByID(const void *a, const void *b) {
    return ((ParkingSpace *)a)->id - ((ParkingSpace *)b)->id;
}

void collectVehicles(BPlusTreeNode *node, Vehicle *v_arr, int *cnt) {
    *cnt = 0;
    if (node) {
        BPlusTreeNode *curr = node;
        while (curr && !curr->leaf_flag) {
         if (curr->data.internal.child[0] == NULL && curr->nkeys >= 0) { return; }
        curr = curr->data.internal.child[0];
    }

        // Check if we successfully reached a leaf node
        if (curr && curr->leaf_flag) {
            bool collect_more = true;
            while (curr != NULL && collect_more) {
                for (int i = 0; i < curr->nkeys && collect_more; i++) {
                    if (*cnt < MAX_VEHICLES) {
                        v_arr[*cnt] = curr->data.leaf.leaf_v[i];
                        (*cnt)++;
                    } else {
               		    fprintf(stderr, "Warn: Exceeded MAX_VEHICLES.\n");
                        collect_more = false;
                    }
                }
                if(collect_more) {
                    curr = curr->data.leaf.next;
                }
            }
        }
    }

}


void collectSpaces(BPlusTreeNode *node, ParkingSpace *s_arr, int *cnt) {
     *cnt = 0;
    if (node) {
        BPlusTreeNode *curr = node;

        while (curr && !curr->leaf_flag) {
	        if (curr->data.internal.child[0] == NULL && curr->nkeys >= 0) { return; }
	        curr = curr->data.internal.child[0];
    	}

         if (curr && curr->leaf_flag) {
            bool collect_more = true;
            while (curr != NULL && collect_more) {
                for (int i = 0; i < curr->nkeys && collect_more; i++) {
                    if (*cnt < MAX_SPACES) {
                        s_arr[*cnt] = curr->data.leaf.leaf_s[i];
                        (*cnt)++;
                    } else {
               		    fprintf(stderr, "Warn: Exceeded MAX_SPACES.\n");
                        collect_more = false;
                    }
                }
                if(collect_more) {
                    curr = curr->data.leaf.next;
                }
            }
        }

    }

}


void displayVByHrs(BPlusTreeNode *node) {
    if (!node) {
        printf("No vehicles.\n");
    } else {
        Vehicle v_arr[MAX_VEHICLES]; int count = 0;
        collectVehicles(node, v_arr, &count);
        if (count == 0) {
            printf("No vehicles collected.\n");
        } else {
            quickSort(v_arr, count, sizeof(Vehicle), compareVByHrs);
            printf("\n--- Vehicles by Total Hours ---\n");
            printf("%-15s %-20s %-10s %-10s %-5s %-10s %-8s\n", "V#.","Owner","TotHrs","Revenue","Parks","Membership","SpaceID");
            printf("--------------------------------------------------------------------------------\n");
            for (int i = 0; i < count; i++) {
                printf("%-15s %-20.20s %-10.2f %-10.2f %-5d %-10d %-8s\n", // Adjusted width for membership
                       v_arr[i].v_num, v_arr[i].owner, v_arr[i].total_hrs, v_arr[i].revenue,
                       v_arr[i].parks, v_arr[i].membership,
                       (v_arr[i].space_id > 0 ? (char[4]){(v_arr[i].space_id/100)%10+'0', (v_arr[i].space_id/10)%10+'0', v_arr[i].space_id%10+'0', '\0'} : "N/A")); // Display space ID or N/A
            }
             printf("--------------------------------------------------------------------------------\n");
        }
    }
}

void displayVByRev(BPlusTreeNode *node) {
     if (!node) {
         printf("No vehicles.\n");
     } else {
        Vehicle v_arr[MAX_VEHICLES]; int count = 0;
        collectVehicles(node, v_arr, &count);
         if (count == 0) {
             printf("No vehicles collected .\n");
         } else {
            quickSort(v_arr, count, sizeof(Vehicle), compareVByRev);
            printf("\n--- Vehicles by Revenue ---\n");
            printf("%-15s %-20s %-10s %-10s %-5s %-10s %-8s\n", "V#.","Owner","Revenue","TotHrs","Parks","Membership","SpaceID");
            printf("--------------------------------------------------------------------------------\n");
            for (int i = 0; i < count; i++) {
                  printf("%-15s %-20.20s %-10.2f %-10.2f %-5d %-10d %-8s\n", // Adjusted width for membership
                       v_arr[i].v_num, v_arr[i].owner, v_arr[i].revenue, v_arr[i].total_hrs,
                       v_arr[i].parks, v_arr[i].membership,
                       (v_arr[i].space_id > 0 ? (char[4]){(v_arr[i].space_id/100)%10+'0', (v_arr[i].space_id/10)%10+'0', v_arr[i].space_id%10+'0', '\0'} : "N/A")); // Display space ID or N/A
            }
             printf("--------------------------------------------------------------------------------\n");
         }
     }
}

void displaySByHrs(BPlusTreeNode *node) {
    if (!node) {
         printf("No spaces.\n");
    } else {
        ParkingSpace s_arr[MAX_SPACES]; int count = 0;
        collectSpaces(node, s_arr, &count);
         if (count == 0) {
         	printf("No spaces collected.\n");
         } else {
            quickSort(s_arr, count, sizeof(ParkingSpace), compareSByHrs);
            printf("\n--- Spaces by Total Hours ---\n");
            printf("%-10s %-10s %-10s %-10s\n", "SpaceID", "Status", "Total Hrs", "Revenue");
            printf("--------------------------------------------\n");
            for (int i = 0; i < count; i++) {
                printf("%-10d %-10s %-10.2f %-10.2f\n",
                       s_arr[i].id, (s_arr[i].status == 1 ? "Occupied" : "Free"),
                       s_arr[i].hrs, s_arr[i].revenue);
            }
            printf("--------------------------------------------\n");
        }
    }
}

void displaySByRev(BPlusTreeNode *node) {
     if (!node) {
         printf("No spaces.\n");
     } else {
        ParkingSpace s_arr[MAX_SPACES]; int count = 0;
        collectSpaces(node, s_arr, &count);
         if (count == 0) {
             printf("No spaces collected.\n");
         } else {
            quickSort(s_arr, count, sizeof(ParkingSpace), compareSByRev);
            printf("\n--- Spaces by Total Revenue ---\n");
            printf("%-10s %-10s %-10s %-10s\n", "SpaceID", "Status", "Revenue", "Total Hrs");
            printf("--------------------------------------------\n");
            for (int i = 0; i < count; i++) {
                 printf("%-10d %-10s %-10.2f %-10.2f\n",
                       s_arr[i].id, (s_arr[i].status == 1 ? "Occupied" : "Free"),
                       s_arr[i].revenue, s_arr[i].hrs);
            }
             printf("--------------------------------------------\n");
         }
     }
}


void clear_input_buf() {
    int c; while ((c = getchar()) != '\n' && c != EOF);
}

void showMenu() {
    int choice; char v_num[20]; char owner[50];
    bool keep_running = true;

    while (keep_running) {
        printf("\n===== Parking System Menu =====\n");
        printf("1. Vehicle Entry\n");
        printf("2. Vehicle Exit\n");
        printf("3. List Vehicles by Hours\n");
        printf("4. List Vehicles by Revenue\n");
        printf("5. List Spaces by Hours\n");
        printf("6. List Spaces by Revenue\n");
        printf("7. Save and Exit\n");
        printf("===============================\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Enter number.\n");
            clear_input_buf();
        } else {
            clear_input_buf();

            switch (choice) {
                case 1:
                    printf("Enter vehicle number: ");
                    if(scanf("%19s", v_num) != 1) { fprintf(stderr,"Bad v_num input.\n"); clear_input_buf();}
                    else {
                        clear_input_buf();
                        printf("Enter owner name: ");
                        if(scanf(" %49[^\n]", owner) != 1) { fprintf(stderr,"Bad owner input.\n"); clear_input_buf();}
                        else {
                             clear_input_buf();
                             vehicleEntry(v_num, owner);
                        }
                    }
                    break;

                case 2:
                    printf("Enter vehicle number: ");
                     if(scanf("%19s", v_num) != 1) { fprintf(stderr,"Bad v_num input.\n"); clear_input_buf();}
                     else {
                        clear_input_buf();
                        vehicleExit(v_num);
                     }
                    break;

                case 3: displayVByHrs(v_root); break;
                case 4: displayVByRev(v_root); break;
                case 5: displaySByHrs(s_root); break;
                case 6: displaySByRev(s_root); break;
                case 7:
                    saveDataAndFree();
                    printf("Data saved. Exiting program.\n");
                    keep_running = false;
                    break;

                default:
                    printf("Invalid choice.\n");
            }

             if (keep_running) {
                 printf("\nPress Enter to continue...");
                 getchar();
             }
        }
    }
}



void freeTreeRecursive(BPlusTreeNode *n) {
    if (n != NULL) {
        if (!n->leaf_flag) {

            for (int i = 0; i <= n->nkeys; i++) {
                if (n->data.internal.child[i] != NULL) {
                    freeTreeRecursive(n->data.internal.child[i]);
                    n->data.internal.child[i] = NULL;
                }
            }
        }
        free(n);
    }
}


void saveVehiclesToFile(BPlusTreeNode *node, const char *fname) {
    FILE *fp = fopen(fname, "w");
    if (!fp) {
         perror("Err open vehicle file for write");
         fprintf(stderr, "Failed to open file: %s\n", fname);
    } else {
        Vehicle v_arr[MAX_VEHICLES]; int count = 0;
        collectVehicles(node, v_arr, &count);

        printf("Saving %d vehicles to %s...\n", count, fname);


        for (int i = 0; i < count; i++) {
            fprintf(fp, "%s %s %s %s %s %s %d %.2f %d %d %.2f\n",
                    v_arr[i].v_num, v_arr[i].owner,
                    v_arr[i].arr_date[0] ? v_arr[i].arr_date : "-",
                    v_arr[i].arr_time[0] ? v_arr[i].arr_time : "-",
                    v_arr[i].dep_date[0] ? v_arr[i].dep_date : "-",
                    v_arr[i].dep_time[0] ? v_arr[i].dep_time : "-",
                    v_arr[i].membership,
                    v_arr[i].total_hrs >= 0 ? v_arr[i].total_hrs : 0.0,
					v_arr[i].space_id, v_arr[i].parks,
                    v_arr[i].revenue >= 0 ? v_arr[i].revenue : 0.0);
        }
        fclose(fp);
        printf("Vehicle save done.\n");
    }
}

void saveSpacesToFile(BPlusTreeNode *node, const char *fname) {
    FILE *fp = fopen(fname, "w");
     if (!fp) {
         perror("Err open space file for write");
         fprintf(stderr, "Failed to open file: %s\n", fname);

     } else {
        ParkingSpace s_arr[MAX_SPACES]; int count = 0;
        collectSpaces(node, s_arr, &count);
        quickSort(s_arr, count, sizeof(ParkingSpace), compareSpacesByID);

        printf("Saving %d spaces to %s...\n", count, fname);

        for (int i = 0; i < count; i++) {
            fprintf(fp, "%d %d %.2f %.2f\n",
                    s_arr[i].id, s_arr[i].status,
                    s_arr[i].hrs >= 0 ? s_arr[i].hrs : 0.0,
                    s_arr[i].revenue >= 0 ? s_arr[i].revenue : 0.0);
        }
        fclose(fp);
        printf("Space save done.\n");
    }
}

void saveDataAndFree() {
    printf("\n--- Saving Data ---\n");
    saveVehiclesToFile(v_root, "bplus-vehicle-database.txt");
    saveSpacesToFile(s_root, "bplus-parking-lot-data.txt");

    printf("\n--- Freeing Memory ---\n");
    freeTreeRecursive(v_root); v_root = NULL;
    printf("Vehicle tree freed.\n");
    freeTreeRecursive(s_root); s_root = NULL;
    printf("Space tree freed.\n");
}

int main() {
    printf("--- Init Parking System ---\n");
    loadSpaces();
    loadVehicles();
    printf("--- Init Complete ---\n");

    showMenu();

    printf("Program end.\n");
    return 0;
}
