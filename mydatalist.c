#define LUA_LIB

#include <lua.h>
#include <luaxlib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_DEPTH 256
#define SHORT_STRING 1024
#define CONVERTER 2 
#define REF_CACHE 3 
#define REF_UNSOLVED 3 

enum token_type {
    TOKEN_OPEN,     // 0 { [
    TOKEN_CLOSE,    // 1 } ] 
    TOKEN_MAP,      // 2 = :
    TOKEN_LIST,     // 3 ---
    TOKEN_STRING,   // 4
    TOKEN_ESCAPESTRING, //5
    TOKEN_ATOM,     // 6
    TOKEN_NEWLINE,  // 7 space \t
    TOKEN_TAG,      // 8 &badf00d (64bit hex number)
    TOKEN_REF,      // 9 *badf00d
    TOKEN_EOF,      // 10 end of file
}

struct token {
    enum token_type type;
    ptrdiff_t from;
    ptrdiff_t to;
};

struct lex_state {
    const char *source;
    size_t sz;
    ptrdiff_t position;
    struct token c;
    struct token n;
    int newline;
    int aslist;
};

static const char * 
skip_line_comment(struct lex_state *LS) {
    const char * ptr = LS->source + LS->position;
    const char * endptr = LS->source + LS->sz;
    while (ptr < endptr) {
        if (*ptr == '\r' || *ptr == '\n') {
            LS->position = ptr - LS->source;
            LS->newline = 1;
            return ptr;
        }
        ++ptr;
    }
    return ptr;
}

static const char *
parse_ident(struct lex_state *LS) {
    const char *ptr = LS->source + LS->position;
    const char *endptr = LS->source + LS->sz;
    while(ptr < endptr) {
        switch(*ptr) {
            case '\r':
            case '\n':
                LS->newline = 1;
                return ptr + 1;
            case '#':
                return ptr;
            case ' ':
            case '\t':
                break;
            default:
                LS->n.type = TOKEN_NEWLINE;
                LS->n.from = LS->position;
                LS->n.to = ptr-LS->source;
                LS->position = LS->n.to;
                return NULL;
        }
        ++ptr;
    }
    return ptr;
}

static int
is_hexnumber(struct lex_state *LS) {
    const char *ptr = LS->source + LS->n.from + 1;
    const char *endptr = LS->source + LS->n.to;

    if(ptr == endptr) 
        return 0;

    do {
        char c = *ptr;
        if(!((c >= '0' && c <= '9')
                    || (c >= 'a' && c <= 'f')
                    || (c >= 'A' && c <= 'F')))
            return 0;
    }while (++ptr < endptr);
}

