/* Name: Daeho Kim */
/* longin ID: kdh0324 */

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

typedef enum { HIT,
               COLD_MISS } Op;

typedef struct {
    int v, tag;
    size_t lru;
} line_t;

typedef struct {
    line_t* lines;
} set_t;

typedef struct {
    set_t* sets;
    size_t E;
} cache_t;

cache_t cache = {};
int hit_count = 0, miss_count = 0, eviction_count = 0;
bool verbose = false;

/* Get LRU of selected set. */
line_t* getLRU(set_t* set) {
    size_t line_num = cache.E;
    for (size_t i = 0; i < line_num; i++) {
        line_t* line = &set->lines[i];
        if (line->v && line->lru == 0)
            return line;
    }
    return NULL;
}

/* Update lru of each blocks. */
void update(set_t* set, line_t* line) {
    size_t line_num = cache.E;
    size_t lru = line->lru;
    for (size_t i = 0; i < line_num; i++) {
        line_t* tmp = &set->lines[i];
        if (tmp->v && tmp->lru > lru)
            tmp->lru--;
    }
    line->lru = line_num - 1;
}

/* Write allocate from memory. */
void write_allocate(set_t* set, line_t* line, int tag) {
    line->v = 1;
    line->tag = tag;

    update(set, line);
}

bool hit_condition(line_t line, int tag) {
    return !line.v || line.tag != tag;
}

bool cold_miss_condition(line_t line, int tag) {
    return line.v;
}

line_t* check(set_t* set, int tag, Op op) {
    size_t line_num = cache.E;
    bool (*condition)();
    if (op == HIT)
        condition = hit_condition;
    else
        condition = cold_miss_condition;
        
    for (size_t i = 0; i < line_num; i++) {
        line_t* line = &set->lines[i];
        if (condition(*line, tag)) continue;

        if (op == HIT) {
            hit_count++;
            if (verbose)
                printf("hit ");
        } else
            line->lru = 0;
        return line;
    }
    return NULL;
}

/* Find LRU and write allocate. */
line_t* evict(set_t* set, int tag) {
    eviction_count++;
    if (verbose)
        printf("eviction ");
    return getLRU(set);
}

/* Load data. Store data.*/
void load_store(int set_index, int tag) {
    set_t* set = &cache.sets[set_index];
    line_t* line = check(set, tag, HIT);
    if (line == NULL) {
        miss_count++;
        if (verbose)
            printf("miss ");
        line = check(set, tag, COLD_MISS);
    }
    if (line == NULL)
        line = evict(set, tag);
    write_allocate(set, line, tag);
}

int main(int argc, char* argv[]) {
    FILE* fp;
    int set_bits, block_bits;
    int set_num = 0;

    int opt;
    bool isMissed = true;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        isMissed = false;
        switch (opt) {
            /* Optional verbose flag that displays trace info */
            case 'v':
                verbose = true;
                break;

            /* Number of set index bits (S = 2^s is the number of sets) */
            case 's':
                set_bits = atoi(optarg);
                set_num = 1 << (set_bits + 1);
                break;

            /* Associativity (number of lines per set) */
            case 'E':
                cache.E = atoi(optarg);
                break;

            /* Number of block bits (B = 2^b is the block size) */
            case 'b':
                block_bits = atoi(optarg);
                break;

            /* Name of the valgrind trace to replay */
            case 't':
                if (!(fp = fopen(optarg, "r"))) return 1;
                break;

            /* Optional help flag that prints usage info */
            case 'h':
                execl("csim-ref", "csim-ref", "-h", NULL);
                break;

            /* Unknown parameter */
            default:
                printf("./csim: invalid option -- \'%c\'\n", opt);
                execl("csim-ref", "csim-ref", "-h", NULL);
                return 1;
        }
    }

    if (isMissed) {
        printf("./csim: Missing required command line argument\n");
        execl("csim-ref", "csim-ref", "-h", NULL);
        return 1;
    }

    if (!(set_bits && block_bits)) return 1;

    int line_num = cache.E;
    cache.sets = malloc(sizeof(set_t) * set_num);
    for (int i = 0; i < set_num; i++)
        cache.sets[i].lines = calloc(sizeof(line_t), line_num);

    char op;
    int address, size;
    while (fscanf(fp, "%c %x,%d", &op, &address, &size) != EOF) {
        size_t set_index = (address >> block_bits) & (0x7fffffff >> (31 - set_bits));
        int tag = address >> (block_bits + set_bits);
        switch (op) {
            case 'I':
                continue;
            /* Modify data (i.e., a data load followed by a data store). */
            case 'M':
                load_store(set_index, tag);
            case 'L':
            case 'S':
                load_store(set_index, tag);
                break;
            default:
                continue;
        }
        if (verbose)
            printf("\n");
    }

    printSummary(hit_count, miss_count, eviction_count);

    fclose(fp);
    for (int i = 0; i < set_num; i++)
        free(cache.sets[i].lines);
    free(cache.sets);
    return 0;
}