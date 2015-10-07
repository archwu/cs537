#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_SIZE 514

char *words[INPUT_SIZE];
int is_batch = 0;
int is_redirection = 0;
char *preToken, *postToken;

char prompt_message[6] = "mysh #";
char error_message[30] = "An error has occurred\n";

typedef struct circular_buffer
{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *tail;       // pointer to tail
} circular_buffer;

void cb_init(circular_buffer *cb, size_t capacity, size_t sz)
{
    cb->buffer = malloc(capacity * sz);
    if(cb->buffer == NULL){
      fprintf(stderr, "mysh: buffer allocation failed\n");
    }
        // handle error
    cb->buffer_end = (char *)cb->buffer + capacity * sz;
    cb->capacity = capacity;
    cb->count = 0;
    cb->sz = sz;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
}

void cb_free(circular_buffer *cb)
{
    free(cb->buffer);
    // clear out other fields too, just to be safe
}

void cb_push_back(circular_buffer *cb, const void *item)
{
    if(cb->count == cb->capacity)
        // handle error
    memcpy(cb->head, item, cb->sz);
    cb->head = (char*)cb->head + cb->sz;
    if(cb->head == cb->buffer_end)
        cb->head = cb->buffer;
    cb->count++;
}

void cb_pop_front(circular_buffer *cb, void *item)
{
    if(cb->count == 0)
        // handle error
    memcpy(item, cb->tail, cb->sz);
    cb->tail = (char*)cb->tail + cb->sz;
    if(cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
}


void prompt() {
    if (!is_batch)
        write(STDOUT_FILENO, prompt_message, strlen(prompt_message));
}

void error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int split(char *line, char *words[]) {
    int count = 0;
    char *input = strdup(line);
    while (1) {
        while (isspace(*input))
            input++;
        if (*input == '\0')
            return count;
        words[count++] = input;
        while (!isspace(*input) && *input != '\0')
            input++;
    }
}

int main (int argc, char *argv[]) {
    // Variables
    FILE *inFile = NULL;
    int outFile, outFile_1;
    char input[INPUT_SIZE];
    int count = 0, i;

    // Interactive or batch mode
    if (argc == 1)
        inFile =  stdin;
    else if (argc == 2) {
        inFile = fopen(argv[1], "r");
        if (inFile == NULL) {
            error();
            exit(1);
        }
        is_batch = 1;
    }
    else {
        error();
        exit(1);
    }

    prompt();
    // Handle input
    while (fgets(input, INPUT_SIZE, inFile)) {

        is_redirection = 0;
        preToken = NULL;
        postToken = NULL;

        prompt();

        // Input too long
        if (strlen(input) > 513) {
            if (is_batch)
                write(STDOUT_FILENO, input, strlen(input));
            error();
            continue;
        }

        // Empty command line
        if (strlen(input) == 1) {
            continue;
        }

        if (is_batch)
            write(STDOUT_FILENO, input, strlen(input));

        // Search for redirection
        preToken = strtok(input, ">");
        if (strlen(preToken) != strlen(input)) {
            is_redirection = 1;
            postToken = strtok(NULL, ">");
            // Too many redirections
            if (strtok(NULL, ">") != NULL) {
                error();
                continue;
            }
        }

        // Handle redirection
        if (!is_redirection)
            count = split(input, words);
        else {
            count = split(preToken, words);
            i = split(postToken, &words[count]);
            count += i;
            outFile_1 = dup(1);
            outFile = open(words[count-1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
            if (outFile < 0) {
                error();
                continue;
            }
            else if (dup2(outFile, 1) < 0) {
                error();
                continue;
            }
            close(outFile);
        }

        // Exit
        if (!strcmp("exit", words[0])) {
            if (count != 1) {
                error();
                continue;
            }
            else
                exit(0);
        }
    }

    return 0;
}
