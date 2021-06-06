#include <stdio.h>
#include <stdbool.h>
#include <string.h> // strcmp()
#include <stdlib.h> // exit(); #define EXIT_SUCCESS 0, #define EXIT_FAILURE 1
#include <stdint.h> // uint32_t

// macro to calculate the size of a member in a specific struct
#define size_of_member(Type, Member) sizeof(((Type*)0)->Member)

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100 // arbitrary limit of pages we can allocate

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID
} PrepareResult;

typedef  enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

typedef enum {
    STATEMENT_SELECT,
    STATEMENT_INSERT
} StatementType;

// row structure
typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1]; //  C strings are supposed to end with a null character
    char email[COLUMN_EMAIL_SIZE + 1]; // so we should allocate one additional byte for username & email
} Row;

// user input statement
typedef struct {
    StatementType type;
    Row row_to_insert; // store the parameters in the insert statement
} Statement;

// keeps track of how many rows there are and pointers to each page
typedef struct {
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
} Table;

// store user inputs
typedef struct {
    char* buffer;
    size_t buffer_length; // unsigned
    ssize_t input_length; // signed
} InputBuffer;

// define the size of the members in the Row struct
const uint32_t ID_SIZE = size_of_member(Row, id);
const uint32_t USERNAME_SIZE = size_of_member(Row, username);
const uint32_t EMAIL_SIZE = size_of_member(Row, email);
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

// define the address offset of the members in the Row struct
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

// define constants related to the size and number of pages in memory
const uint32_t PAGE_SIZE = 4096; // 4KB, consistent with the page size of virtual memory in os
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = TABLE_MAX_PAGES * ROWS_PER_PAGE;

// refer to sqlite's prompt
void print_prompt() {
    printf("db > ");
}

// print row data
void print_row(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

// create a Table and allocate memory
Table* new_table() {
    Table* table = malloc(sizeof(Table));

    table->num_rows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; ++i) {
        table->pages[i] = NULL; // remember to initialize pointer
    }

    return table;
}

// release memory allocated to Table
void free_table(Table* table) {
    // It's tricky here, because according to the implementation in row_slot,
    // pages are assigned according to the number of rows, and the loop exits
    // when the first empty page is encountered("table->pages[i] == NULL")
    for (uint32_t i = 0; table->pages[i]; ++i) {
        free(table->pages[i]);
    }
    free(table);
}

// In this compact representation,
// (1) the row number and ROWS_PER_PAGE can be used to locate which page in the Table the row is on,
// (2) and furthermore, the position of the row in the page can be known (actually, it is a remainder),
// (3) and each member can be accessed in memory by the offset of the members in the row.
// This function only completes the first & second steps, which is to return the memory address of a row.
void* row_slot(Table* table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE; // complete step(1)

    void* page = table->pages[page_num];
    if (page == NULL) {
        page = table->pages[page_num] = malloc(PAGE_SIZE); // Allocate memory if the page doesn't exist yet
    }

    uint32_t row_offset = row_num % ROWS_PER_PAGE; // complete step(2), offset on row numbers
    uint32_t byte_offset = row_offset * ROW_SIZE; // converts offset on row numbers to offset on memory bytes

    return page + byte_offset; // row start address = page start address + page offset
}

// Compactly store each member of Row-struct according to the offset
void serialize_row(Row* source, void* destination) {
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

// Convert compact representation to a Row-struct instance
void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

// initialize an InputBuffer instance
InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));

    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

// free the memory allocated for the instance and element of the respective structure
void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer); // getline() allocates memory in read_input()
    free(input_buffer); // new_input_buffer() allocates memory
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

// a wrapper for processing meta command
MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        exit(EXIT_SUCCESS); // not return META_COMMAND_SUCCESS(0), but should terminate the program
    }
    else {
        return META_COMMAND_UNRECOGNIZED;
    }
}

// Since input_buffer can be large enough, but the size of the array in Row-struct is limited,
// so after reading user input into input_buffer, the string is first split into four substrings,
// and only when the substrings username and email are of legal length can be written to Row-struct
PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
    statement->type = STATEMENT_INSERT;

    char* keyword = strtok(input_buffer->buffer, " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " "); // scan stops if the terminating null character is found

    int id = atoi(id_string); // can not use uint_32(unsigned)
    if (id < 0) {
        return PREPARE_NEGATIVE_ID;
    }
    if (!id_string || !username || !email) {
        return PREPARE_SYNTAX_ERROR;
    }
    if (strlen(username) > COLUMN_USERNAME_SIZE || strlen(email) > COLUMN_EMAIL_SIZE) {
        return PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}

// hacky version of the "SQL Compiler"
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    // In SimpleDB, "select" is used to print all rows, so there is no need to use strncmp
    if (strcmp(input_buffer->buffer, "select") == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    // In SimpleDB, "insert x y z" takes 3 parameters(id, username, email), so we need to use strncmp
    else if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        return prepare_insert(input_buffer, statement);
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// execution of a "select" statement
ExecuteResult execute_select(Statement* statement, Table* table) {
    Row row;
    for (uint32_t i = 0; i < table->num_rows; ++i) {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
}

// execution of a "insert x y z" statement
ExecuteResult execute_insert(Statement* statement, Table* table) {
    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row* row = &(statement->row_to_insert);
    serialize_row(row, row_slot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

// hacky version of the virtual machine
ExecuteResult execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case (STATEMENT_SELECT) :
            return execute_select(statement, table);
        case (STATEMENT_INSERT) :
            return execute_insert(statement, table);
    }
}

int main(int argc, char* argv[]) {
    Table* table = new_table();
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
            case (PREPARE_SYNTAX_ERROR) :
                printf("Syntax error, can't parse statement: '%s'.\n", input_buffer->buffer);
                continue;
            case (PREPARE_STRING_TOO_LONG) :
                printf("String is too long.\n");
                continue;
            case (PREPARE_NEGATIVE_ID) :
                printf("ID must be positive.\n");
                continue;
        }

        // Executing SQL statement
        switch (execute_statement(&statement, table)) {
            case (EXECUTE_SUCCESS) :
                printf("Executed.\n");
                break;
            case (EXECUTE_TABLE_FULL) :
                printf("Error: Table full.\n");
                break;
        }
    }
}
