#include <stdio.h>
#include <stdbool.h>
#include <string.h> // strcmp()
#include <stdlib.h> // exit(); #define EXIT_SUCCESS 0, #define EXIT_FAILURE 1

typedef struct {
    char* buffer;
    size_t buffer_length; // unsigned
    ssize_t input_length; // signed
} InputBuffer;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum {
    STATEMENT_SELECT,
    STATEMENT_INSERT
} StatementType;

typedef struct {
    StatementType type;
} Statement;

// initialize an InputBuffer instance
InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));

    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

// refer to sqlite's prompt
void print_prompt() {
    printf("db > ");
}

// Get the user input statements and store them in a custom memory buffer
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

// a wrapper for processing meta command
MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        exit(EXIT_SUCCESS); // not return META_COMMAND_SUCCESS(0), but should terminate the program
    }
    else {
        return META_COMMAND_UNRECOGNIZED;
    }
}

// hacky version of the "SQL Compiler"
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if (strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    else if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// hacky version of the virtual machine
void execute_statement(Statement* statement) {
    switch (statement->type) {
        case (STATEMENT_SELECT) :
            printf("TODO: this is where we would do a select.\n");
            break;
        case (STATEMENT_INSERT) :
            printf("TODO: this is where we would do a insert.\n");
            break;
    }
}

int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();

    while (true) {
        print_prompt(); // 打印提示符
        read_input(input_buffer);

        // Processing meta-commands, i.e., non-SQL statements, starting with a dot
        if (input_buffer->buffer[0] == '.') {
            switch (do_meta_command(input_buffer)) {
                case (META_COMMAND_SUCCESS) :
                    printf("META_COMMAND_SUCCESS\n");
                    continue;
                case (META_COMMAND_UNRECOGNIZED) :
                    printf("Error: unknown meta-command: '%s'.\n", input_buffer->buffer);
                    continue;
            }
        }

        // Parsing SQL statements, converting input rows to internal representation
        Statement statement;
        switch (prepare_statement(input_buffer, &statement)) {
            case (PREPARE_SUCCESS) :
                break;
            case (PREPARE_UNRECOGNIZED_STATEMENT) :
                printf("Error: unknown statement: '%s'.\n", input_buffer->buffer);
                continue;
        }

        // Executing SQL statement
        execute_statement(&statement);
        printf("Executed.\n");
    }
}
