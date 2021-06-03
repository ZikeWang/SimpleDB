#include <stdio.h>
#include <stdbool.h>
#include <string.h> // strcmp()
#include <stdlib.h> // exit(); #define EXIT_SUCCESS 0, #define EXIT_FAILURE 1

typedef struct {
    char* buffer;
    size_t buffer_length; // unsigned
    ssize_t input_length; // signed
} InputBuffer;

// initialize an InputBuffer instance
InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));

    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

void print_prompt() {
    printf("db > "); // reference to sqlite's prompt
}

void read_input(InputBuffer* input_buffer) {
    // If the first parameter is set to NULL, it is mallocatted by getline()
    // and should thus be freed by the user, even if the command fails.
    // As seen below, We tell getline() to store the read line in input_buffer->buffer
    // and the size of the allocated buffer in input_buffer->buffer_length.
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0) {
        printf("Read Error\n");
        exit(EXIT_FAILURE);
    }

    // if read succeeded, ignore trailing newline
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0; // value 0 should be used, not character '0'
}

// free the memory allocated for the instance and element of the respective structure
void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer); // getline() allocates memory in read_input()
    free(input_buffer); // new_input_buffer() allocates memory
}

int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();

    while (true) {
        print_prompt(); // 打印提示符
        read_input(input_buffer);

        if (strcmp(input_buffer->buffer, ".exit") == 0) {
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        } else { // reference to sqlite
            printf("Error: unknown command or invalid arguments: '%s'.\n", input_buffer->buffer);
        }
    }
}
