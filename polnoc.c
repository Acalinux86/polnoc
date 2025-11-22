#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define RPNI_INIT_CAP 5
#define RPNI_TRACE 0

char *rpni_read_file_into_memory(const char *rpni_file_path, size_t *rpni_file_size)
{
    FILE *fp = fopen(rpni_file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Could not read `%s`: `%s`\n", rpni_file_path, strerror(errno));
        return NULL;
    }

    int ret = fseek(fp, 0, SEEK_END);
    if (ret < 0) {
        fprintf(stderr, "[ERROR] Could not seek to end of `%s`: `%s`\n", rpni_file_path, strerror(errno));
        return NULL;
    }

    size_t size = ftell(fp);
    if (size <= 0) {
        fprintf(stderr, "[ERROR] File `%s` Empty\n", rpni_file_path);
        return NULL;
    }

    rewind(fp); // Rewind the file pointer to the beginning

    char *buffer = (char *)malloc(size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "[ERROR] Memory Allocation Failed\n");
        return NULL;
    }

    size_t bytes = fread(buffer, sizeof(*buffer), size, fp);
    if (bytes != (size_t)size) {
        fprintf(stderr, "fread() failed: Expected %ld bytes got %zu bytes\n", size, bytes);
        return NULL;
    }
    buffer[bytes] = '\0'; // Null Terminate

    *rpni_file_size = size;
    fclose(fp); // close file pointer
    return buffer;
}

const char *rpni_shift_args(int *argc, char ***argv)
{
    const char *result = **argv;
    (*argc)--;
    (*argv)++;
    return result;
}

typedef struct RPNI_String {
    char   *c_str;
    size_t len;
} RPNI_String;

typedef struct RPNI_RawList {
    RPNI_String *items;
    size_t count;
    size_t capacity;
} RPNI_RawList;

typedef enum RPNI_TokenType {
    RPNI_TOKEN_NUMBER,
    RPNI_TOKEN_PLUS,
    RPNI_TOKEN_MINUS,
    RPNI_TOKEN_DIV,
    RPNI_TOKEN_MULT,
} RPNI_TokenType;

const char *rpni_token_type_as_cstr(RPNI_TokenType type)
{
    switch (type) {
    case RPNI_TOKEN_NUMBER: return "RPNI_TOKEN_NUMBER";
    case RPNI_TOKEN_PLUS: return "RPNI_TOKEN_PLUS";
    case RPNI_TOKEN_MINUS: return "RPNI_TOKEN_MINUS";
    case RPNI_TOKEN_DIV: return "RPNI_TOKEN_DIV";
    case RPNI_TOKEN_MULT: return "RPNI_TOKEN_MULT";
    default:
        fprintf(stderr, "[PANIC] Unreachable TOKEN TYPE\n");
        return NULL;
    }
}

typedef union RPNI_TokenData {
    double number;
    char   *string;
} RPNI_TokenData;

typedef struct RPNI_Token {
    RPNI_TokenType type;
    RPNI_TokenData data;
} RPNI_Token;

typedef struct RPNI_Tokens {
    RPNI_Token *items;
    size_t count;
    size_t capacity;
} RPNI_Tokens;

typedef struct RPNI_Stack {
    RPNI_Token *items;
    size_t count;
    size_t capacity;
} RPNI_Stack;

#define rpni_array_push(array, item)                                    \
    do {                                                                \
        if ((array)->count >= (array)->capacity) {                      \
            (array)->capacity =                                         \
                (array)->capacity == 0 ?                                \
                RPNI_INIT_CAP :                                         \
                (array)->capacity * 2;                                  \
            (array)->items =                                            \
                realloc((array)->items,                                 \
                        (array)->capacity*(sizeof(*(array)->items)));   \
            if ((array)->items == NULL) {                               \
                fprintf(stderr, "[ERROR] Memory Reallocation for Array Failed\n"); \
                exit(EXIT_FAILURE);                                     \
            }                                                           \
        }                                                               \
        (array)->items[(array)->count++] = item;                        \
    } while (0)

RPNI_Token rpni_stack_pop(RPNI_Stack *stack)
{
    if (stack->count <= 0) {
        fprintf(stderr, "[ERROR] Attempting to Pop From Empty Stack\n");
        exit(EXIT_FAILURE);
    }
    return stack->items[--stack->count];
}

bool rpni_is_opcode(char x)
{
    return x == '+' || x == '*' || x == '/' || x == '-';
}

char *rpni_strdup(char *src, size_t src_len)
{
    char *dest = (char *)malloc(sizeof(char)*src_len+1);
    if (dest == NULL) {
        fprintf(stderr, "ERROR: Memory Allocation For String Duplication failed\n");
        return NULL;
    }
    strcpy(dest, src);
    dest[src_len] = '\0';
    return dest;
}

bool rpni_tokenize_source_string(RPNI_RawList *list, const char *source, const size_t size)
{
    if (!list)     return false;
    if (!source)   return false;
    if (size == 0) return false;

    const size_t OUT = 1;
    const size_t IN  = 1;
    size_t state = OUT;

    char buf[256] = {0};
    size_t cursor = 0;
    size_t n = 0;

    while (n <= size) {
        char c = source[n];
        if (isspace(c)) {
            state = OUT;
            if (cursor > 0) {
                rpni_array_push(list, ((RPNI_String){rpni_strdup(buf, cursor), cursor}));
            }
            cursor = 0;
        } else if (state == OUT) {
            state = IN;
            if (rpni_is_opcode(c)) {
                rpni_array_push(list, ((RPNI_String){rpni_strdup(&c, 1), 1}));
            } else {
                buf[cursor++] = c;
            }
        } else {
            ;
        }
        n++;
    }
    return true;
}

bool rpni_dump_raw_list(const RPNI_RawList *list)
{
    if (!list) return false;

    for (size_t i = 0; i < list->count; ++i) {
        printf("[INFO] Token: %s, Size: %zu\n", list->items[i].c_str, list->items[i].len);
    }
    return true;
}

double rpni_parse_number(char *number_string)
{
    char *endptr, *str;
    double number = 0;

    errno = 0;
    str = number_string;
    number = strtold(str, &endptr);
    if (errno == ERANGE) {
        perror("strtold");
        exit(EXIT_FAILURE);
    }

    if (endptr == str) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }
    return number;
}

bool rpni_is_string_number(RPNI_String *str)
{
    for (size_t i = 0; i < str->len; ++i) {
        char c = str->c_str[i];
        if (!(isdigit(c) || '.')) {
            return false;
        }
    }
    return true;
}

bool rpni_tokenize_raw_list(const RPNI_RawList *list, RPNI_Tokens *tokens)
{
    if (!list)   return false;
    if (!tokens) return false;

    for (size_t i = 0; i < list->count; ++i) {
        RPNI_Token token = {0};
        if (isdigit(list->items[i].c_str[0])) { // if digit occurred , parse number
            if (!rpni_is_string_number(&list->items[i])) {
                fprintf(stderr, "[ERROR] Error Occurred While Parsing Number: %s\n", list->items[i].c_str);
                return false;
            }
            token.data.number = rpni_parse_number(list->items[i].c_str);
            token.type = RPNI_TOKEN_NUMBER;
            rpni_array_push(tokens, token);
        } else if (rpni_is_opcode(list->items[i].c_str[0])) {
            // parse opcode
            char opcode = list->items[i].c_str[0];
            switch (opcode) {
            case '+': {
                token.data.string = list->items[i].c_str;
                token.type = RPNI_TOKEN_PLUS;
                rpni_array_push(tokens, token);
            } break;

            case '/': {
                token.data.string = list->items[i].c_str;
                token.type = RPNI_TOKEN_DIV;
                rpni_array_push(tokens, token);
            } break;

            case '-': {
                token.data.string = list->items[i].c_str;
                token.type = RPNI_TOKEN_MINUS;
                rpni_array_push(tokens, token);
            } break;

            case '*': {
                token.data.string = list->items[i].c_str;
                token.type = RPNI_TOKEN_MULT;
                rpni_array_push(tokens, token);
            } break;

            default:
                fprintf(stderr, "[ERROR] Unknown Opcode Encounter: %c\n", opcode);
                return false;
            }
        }
        free(list->items[i].c_str);
    }

    return true;
}

bool rpni_eval_exprs(const RPNI_Tokens *tokens, RPNI_Stack *stack)
{
    if (!stack)  return false;
    if (!tokens) return false;

    for (size_t i = 0; i < tokens->count; ++i) {
        RPNI_Token token = tokens->items[i];
        switch (token.type) {
        case RPNI_TOKEN_NUMBER: {
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_PLUS: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left + right;
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_MINUS: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left - right;
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_DIV: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left / right;
            rpni_array_push(stack, token);
        } break;

        case RPNI_TOKEN_MULT: {
            double right = rpni_stack_pop(stack).data.number;
            double left  = rpni_stack_pop(stack).data.number;

            RPNI_Token token = {0};
            token.type = RPNI_TOKEN_NUMBER;
            token.data.number = left * right;
            rpni_array_push(stack, token);
        } break;

        default:
            fprintf(stderr, "[PANIC] Unreachable TOKEN TYPE\n");
            return false;
        }
    }

    return true;
}

bool rpni_dump_tokens(const RPNI_Tokens *tokens)
{
    for (size_t i = 0; i < tokens->count; ++i) {
        RPNI_Token token = tokens->items[i];
        switch (token.type) {
        case RPNI_TOKEN_NUMBER: {
            printf("[INFO] Type: %s, Token: %lf\n", rpni_token_type_as_cstr(token.type), token.data.number);
        } break;

        case RPNI_TOKEN_DIV:
        case RPNI_TOKEN_MULT:
        case RPNI_TOKEN_MINUS:
        case RPNI_TOKEN_PLUS:
            printf("[INFO] Type: %s, Token: %s\n", rpni_token_type_as_cstr(token.type), token.data.string);
            break;
        default:
            fprintf(stderr, "[PANIC] Unreachable TOKEN TYPE\n");
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    const char *program = rpni_shift_args(&argc, &argv);
    if (argc < 1) {
        fprintf(stderr, "[USAGE] %s <input_file>\n", program);
        return 1;
    }

    const char *source = rpni_shift_args(&argc, &argv);
    size_t size = 0;
    char *result = rpni_read_file_into_memory(source, &size);
    if (result == NULL) return 1;

    RPNI_RawList list = {0};
    if (!rpni_tokenize_source_string(&list, result, size)) return 1;

#if RPNI_TRACE
    if (!rpni_dump_raw_list(&list)) return 1;
#endif

    RPNI_Tokens tokens = {0};
    if (!rpni_tokenize_raw_list(&list, &tokens)) return 1;

#if RPNI_TRACE
    if (!rpni_dump_tokens(&tokens)) return 1;
#endif

    RPNI_Stack stack = {0};
    if (!rpni_eval_exprs(&tokens, &stack)) return 1; // Reverse Polish Notation Algorithm

#if RPNI_TRACE
    if (!rpni_dump_tokens((RPNI_Tokens*)&stack)) return 1;
#endif

    // Clean up
    free(result);
    free(stack.items);
    free(list.items);
    free(tokens.items);
    return 0;
}
