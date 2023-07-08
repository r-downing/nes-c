#include <stdio.h>
#include <emscripten.h>

int main(void) {
    printf("built %s @ %s\n", __DATE__, __TIME__);
}
