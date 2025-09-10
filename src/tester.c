#include <assert.h>
#include <stdio.h>
#include <string.h>

int  mini_init(void);
void *mini_malloc(long);
void mini_free(void *);
void *mini_realloc(void *, long);

int main(void){
    assert(mini_init() == 0);

    void *p = mini_malloc(24);
    assert(p); mini_free(p);

    void *a = mini_malloc(1);
    void *b = mini_malloc(7);
    assert(((unsigned long)a % 8) == 0);
    assert(((unsigned long)b % 8) == 0);
    
    void *r = mini_malloc(64);
    memset(r, 0xAB, 64);
    r = mini_realloc(r, 16);
    assert(r);

    void *g = mini_malloc(32);
    memset(g, 0xCD, 32);
    void *g2 = mini_realloc(g, 256);
    assert(g2);

    mini_free(a); mini_free(b); mini_free(r); mini_free(g2);

    puts("mini allocator test passed.");
    return 0;
}
