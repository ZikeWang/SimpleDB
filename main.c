#include <stdio.h>
#include <stdbool.h>
#include <string.h> // strcmp()
#include <stdlib.h> // exit()

typedef struct {

} InputBuffer;

InputBuffer* new_input_buffer() {

}

void print_prompt() {

}

void read_input(InputBuffer* input_buffer) {

}

void close_input_buffer(InputBuffer* input_buffer) {

}

int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();

    while (true) {
        print_prompt(); // 打印提示符
        read_input(input_buffer);

        if (strcmp(input_buffer->buffer, ".exit") == 0) {
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        } else {
            printf("Unrecognized command '%s'.\n", input_buffer->buffer);
        }
    }
}
