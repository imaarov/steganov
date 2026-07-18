#include <stdio.h>

void test_lz77_token_stream();
void test_deflated_lz77_token_stream();
void test_huffman_code_table();
void test_deflate_no_compression();
void test_deflate_dynamic_huffman();

int main() {
    printf("--lz77 test:\n");
    test_lz77_token_stream();
    printf("--deflated lz77 test:\n");
    test_deflated_lz77_token_stream();
    printf("--huffman code test\n");
    test_huffman_code_table();
    printf("--deflate no compression test\n");
    test_deflate_no_compression();
    printf("--deflate dynamic huffman compression\n");
    test_deflate_dynamic_huffman();
    printf("all tests passed\n");
    return 0;
}
