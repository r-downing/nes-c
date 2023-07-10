#include <stdio.h>
#include <emscripten.h>

extern int x;

#ifndef TEST_DEFINE
#warning "TEST_DEFINE not defined"
#endif

int main(void)
{

    int j = j; // making sure other warnings work
    x = j;      // making sure other file built and linked

    printf("built %s @ %s\n", __DATE__, __TIME__);
}

