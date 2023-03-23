#include <stdio.h>

extern const char *const build_hash;

int main(void) {
    printf("hello world\n");
    printf("build_hash = %s\n", build_hash);
}