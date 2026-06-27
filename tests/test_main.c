#include <stdio.h>

void test_lz77_token_stream();
void test_deflated_lz77_token_stream();
void test_huffman_code_table();

int main() {
    printf("--lz77 test:\n");
    test_lz77_token_stream();
    printf("--deflated lz77 test:\n");
    test_deflated_lz77_token_stream();
    printf("--huffman code test\n");
    test_huffman_code_table();
    printf("all tests passed\n");
    return 0;
}
