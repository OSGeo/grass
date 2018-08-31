#ifndef MEMORY_H
#define MEMORY_H
void alloc_memory(unsigned int n);
void free_memory();
struct edge *get_edge();
void free_edge(struct edge *e);
void alloc_sites(unsigned int n);
void realloc_sites(unsigned int n);
void alloc_edges(unsigned int n);
#endif
