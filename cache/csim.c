#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "cachelab.h"
#include "getopt.h"

#define OPTIONS "hvs:E:b:t:"
#define LINELEN 80
#define N_ADDR_BITS 64

typedef struct {
	unsigned int s, b;
	unsigned int n_sets, n_lines, n_blocks;
} cache_size;

typedef struct {
	unsigned int valid;
	unsigned long long tag;
	//unsigned long long block;
} cache_line;

typedef struct {
	unsigned int hit_count, miss_count, eviction_count;
} counter;

typedef struct {
	char type;
	unsigned long long address;
	unsigned int n_bytes;
} operation;

void print_help_info()
{
	printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
	printf("Options:\n");
	printf("%4s %-7s %s\n", "-h", "", "Print this help message.");
	printf("%4s %-7s %s\n", "-v", "", "Optional verbose flage.");
	printf("%4s %-7s %s\n", "-s", "<num>", "Number of set index bits.");
	printf("%4s %-7s %s\n", "-E", "<num>", "Number of lines per set.");
	printf("%4s %-7s %s\n", "-b", "<num>", "Number of block offet bits.");
	printf("%4s %-7s %s\n", "-t", "<file>", "Trace file.");
	printf("\nExamples:\n");
	printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
	printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void swap(cache_line *p1, cache_line *p2)
{
	unsigned long long temp;
	temp = p1->tag;
	p1->tag = p2->tag;
	p2->tag = temp;
}

void mem_op(operation *op, cache_size *size, cache_line **cache, unsigned int *cache_LRU, counter *summary, int verbose)
{
	char info[LINELEN] = "";
	//unsigned int block_offset; 
	unsigned int set_index;
	unsigned long long tag;
	
	//block_offset = op->address << (N_ADDR_BITS - size->b) >> (N_ADDR_BITS - size->b);
	set_index = op->address << (N_ADDR_BITS - size->s - size->b) >> (N_ADDR_BITS - size->s);
	tag = op->address >> (size->s + size->b);

	int hit = 0;
	cache_line *cp;
	int i;
	for (i = 0; i < size->n_lines; i++)
	{

		cp = &cache[set_index][i];
		if (!cp->valid)
			break;
		if (cp->tag == tag)
		{
			hit = 1;
			break;	
		}
	}

	if (hit)
	{
		unsigned int next;
		summary->hit_count++;
		strncat(info, "hit ", LINELEN);

		next = (i + 1) % size->n_lines;
		while (cache[set_index][next].valid && next != cache_LRU[set_index])
		{
			//swap
			swap(&cache[set_index][next], &cache[set_index][i]);
			i = next;
			next = (next + 1) % size->n_lines;
		}
	}
	else
	{
		summary->miss_count++;
		strncat(info, "miss ", LINELEN);

		if (i == size->n_lines) // cache line is full
		{	
			summary->eviction_count++;
			strncat(info, "eviction ", LINELEN);

			i = cache_LRU[set_index];
			cache_LRU[set_index]++;
			cache_LRU[set_index] %= size->n_lines;
		}

		cache[set_index][i].valid = 1;
		cache[set_index][i].tag = tag;
	}

	if (op->type == 'M')
	{	
		summary->hit_count++;
		strncat(info, "hit ", LINELEN);
	}

	if (verbose)
		printf("%s\n", info);

}

void run(FILE* fp, cache_size *size, cache_line **cache, unsigned int *cache_LRU, counter *summary, int verbose)
{
	char line[LINELEN];
	operation op;

	while (fgets(line, LINELEN, fp) != NULL)
	{
		if (line[0] == 'I')
			continue;
		sscanf(line, " %c %llx, %u", &op.type, &op.address, &op.n_bytes);
		mem_op(&op, size, cache, cache_LRU, summary, verbose);	
	}
}

int main(int argc, char *argv[])
{
	int opt;
	int h = 0, v = 0, s = 0, E = 0, b = 0;
	char t[LINELEN];
	int n_sets = 0, n_lines = 0, n_blocks = 0;
	FILE *trace_file = NULL;

	while ( (opt = getopt(argc, argv, OPTIONS)) != -1 ) 
	{
		//optarg
		switch (opt)
		{
			case 'h':
				h = 1;
				break;
			case 'v':
				v = 1;
				break;
			case 's':
				s = atoi(optarg);
				n_sets = 1 << s;
				break;
			case 'E':
				n_lines = E = atoi(optarg);
				printf("%d\n", n_lines);
				break;
			case 'b':
				b = atoi(optarg);
				n_blocks = 1 << b;
				break;
			case 't':
				strncpy(t, optarg, LINELEN);
				trace_file = fopen(t, "r");
				break;
			default:
				h = 1;
				break;
		}
	}

	if (h) 
		print_help_info();

	if ( n_sets == 0 || n_lines == 0 || n_blocks == 0 || trace_file == NULL)
	{
		printf("Invalid arguments.\n");
		if (!h)
			print_help_info();

		return 1;
	}	

	cache_size size = {s, b, n_sets, n_lines, n_blocks}; 
	counter summary = {0, 0, 0};
	cache_line **cache = malloc(n_sets * sizeof(cache_line*));
	unsigned int *cache_LRU = calloc(n_sets, sizeof(unsigned int)); 

	if (cache == NULL)
	{
		printf("Invalid memory allocation.\n");
		return 1;
	}

	for (int i = 0; i < n_sets; i++)
	{
		cache[i] = calloc(n_lines, sizeof(cache_line));
		if (cache[i] == NULL)
		{
			printf("Invalid memory allocation.\n");
			return 1;
		}
	}

	run(trace_file, &size, cache, cache_LRU, &summary, v);
	
	free(cache);
	free(cache_LRU);
	fclose(trace_file);

	printSummary(summary.hit_count, summary.miss_count, summary.eviction_count);
	return 0;
}
