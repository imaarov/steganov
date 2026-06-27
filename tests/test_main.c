#include <stdio.h>

void test_lz77_token_stream();
void test_deflated_lz77_token_stream();

int main() {
    printf("--lz77 test:\n");
    test_lz77_token_stream();
    printf("--deflated lz77 test:\n");
    test_deflated_lz77_token_stream();
    printf("all tests passed\n");
    return 0;
}
