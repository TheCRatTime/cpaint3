/*
 * Copyright (C) 2026 TheCRatTime
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef u8 TokenType;

#define TOK_UB        0  /* Undefined behavior token */
#define TOK_STRING    1
#define TOK_CHAR      2
#define TOK_NUMBER    3
#define TOK_FLOAT     4
#define TOK_HEX       5
#define TOK_ENDLINE   6  /* ; */
#define TOK_PREPROC   7
#define TOK_LINE_CONT 8  /* \ */
#define TOK_STRINGIZE 9  /* # */
#define TOK_PASTE     10 /* ## */
#define TOK_PUNCT     11
#define TOK_COMMENT   12
#define TOK_VAR       13
#define TOK_KEYWORD   14
#define TOK_SPACE     15
#define TOK_EOF       16 /* Token EOF */
#define TOK_LPAREN    17 /* ( */
#define TOK_BIT_UN    18 /* ! */

/* C language token struct */
typedef struct Token {
  TokenType type;       /* Type of token */
  const char* text_ptr; /* Pointer to text */
  size_t len;           /* Length of token */
} Token;

/* CPP directives struct */
typedef struct CPPDirecrives {
  u8 is_if;
  u8 is_elif;
  u8 is_else;
  u8 is_ifdef;
  u8 is_undef;
  u8 is_endif;
  u8 is_error;
  u8 is_define;
  u8 is_ifndef;
  u8 is_pragma;
  u8 is_elifdef;
  u8 is_include;
  u8 is_warning;
  u8 is_elifndef;
} CPPDirectives;

static int IsPunctIgnore(const char chr) {
  if (chr == '_' || chr == '"' || chr == '\'') {
    return 1;
  }
  
  return 0;
}

/* Get token (length). */
static size_t GetWordLen(const char* text) {
  size_t len = 0;

  if (ispunct(*text) && !IsPunctIgnore(*text)) {
    while (ispunct(*text) && !IsPunctIgnore(*text)) {
      text++;
      len++;
    }

    return len;
  }

  if (isdigit(*text)) {
    while (isdigit(*text)) {
      text++;
      len++;
    }

    return len;
  }

  if (isspace(*text)) {
    while (isspace(*text)) {
      text++;
      len++;
    }

    return len;
  }
  
  while (isalnum(*text) || *text == '_') {
    text++;
    len++;
  }
  
  return (!len ? 1 : len);
}

static void TokLineCont(Token* tok, const char* text) {
  tok->len = 1;
  tok->type = (*(text+1) == '\n' ? TOK_LINE_CONT : TOK_UB);
}

static void TokEndLine(Token* tok) {
  tok->len = 1;
  tok->type = TOK_ENDLINE;
}

static void TokLParen(Token* tok) {
  tok->len = 1;
  tok->type = TOK_LPAREN;
}

static void TokBitUn(Token* tok) {
  tok->len = 1;
  tok->type = TOK_BIT_UN;
}

static void TokPunct(Token* tok, size_t len) {
  tok->type = TOK_PUNCT;
  tok->len = len;
}

static void TokString(Token* tok, const char* text) {
  size_t len = 1;
  
  tok->type = TOK_STRING;

  text++;

  while (*text) {
    if (*text == '"') {
      short slashes = 0;
      text--;
      while (*text == '\\') {
        text--;
        slashes++;
      }

      if ((slashes & 0x1) == 0) {
        len++;
        break;
      }

      text += slashes + 1;
    }
    
    text++;
    len++;
  }

  tok->len = len;
}

static void TokChar(Token* tok, const char* text) {
  size_t len = 1;
  
  tok->type = TOK_CHAR;

  text++;

  while (*text) {
    if (*text == '\'') {
      short slashes = 0;
      text--;
      while (*text == '\\') {
        text--;
        slashes++;
      }

      if ((slashes & 0x1) == 0) {
        len++;
        break;
      }

      text += slashes + 1;
    }

    text++;
    len++;
  }

  tok->len = len;
}

static int NoneDirective(const CPPDirectives* const cpp_dir) {
  if (!cpp_dir->is_if && !cpp_dir->is_elif && !cpp_dir->is_else &&
      !cpp_dir->is_ifdef && !cpp_dir->is_undef && !cpp_dir->is_endif &&
      !cpp_dir->is_define && !cpp_dir->is_ifndef && !cpp_dir->is_pragma &&
      !cpp_dir->is_elifdef && !cpp_dir->is_include &&
      !cpp_dir->is_elifndef && !cpp_dir->is_error &&
      !cpp_dir->is_warning) {
    return 1;
  }

  return 0;
}

#define CPP_IF_LEN 3
#define CPP_ELIF_LEN 5
#define CPP_ELSE_LEN 5
#define CPP_IFDEF_LEN 6
#define CPP_UNDEF_LEN 6
#define CPP_ENDIF_LEN 6
#define CPP_ERROR_LEN 6
#define CPP_DEFINE_LEN 7
#define CPP_IFNDEF_LEN 7
#define CPP_PRAGMA_LEN 7
#define CPP_ELIFDEF_LEN 8
#define CPP_INCLUDE_LEN 8
#define CPP_WARNING_LEN 8
#define CPP_ELIFNDEF_LEN 9

#define CPP_TOTAL_LENGTHS 14

static void SetTokLen(const CPPDirectives* const cpp_dir, Token* const tok) {
  const size_t lens[CPP_TOTAL_LENGTHS] = {
    CPP_IFDEF_LEN,    CPP_IFNDEF_LEN,  CPP_ELIFDEF_LEN,
    CPP_ELIFNDEF_LEN, CPP_IF_LEN,      CPP_ELIF_LEN,   
    CPP_ELSE_LEN,     CPP_INCLUDE_LEN, CPP_WARNING_LEN,
    CPP_PRAGMA_LEN,   CPP_UNDEF_LEN,   CPP_ENDIF_LEN,  
    CPP_ERROR_LEN,    CPP_DEFINE_LEN, 
  };
  u8 cur = 0;
  const char* as_char_array = (const char*)cpp_dir;
  
  for (; cur < CPP_TOTAL_LENGTHS; cur++) {
    if (as_char_array[cur]) {
      tok->len = lens[cur];
      break;
    }
  }
}

static int CPPNoNeedContinue(const CPPDirectives* const cpp_dir) {
  if (!cpp_dir->is_include && !cpp_dir->is_warning && !cpp_dir->is_error) {
    return 1;
  }
  
  return 0;
}

static void TokHash(Token* tok, const char* text) {
  CPPDirectives cpp_dir;
  cpp_dir.is_if       = !strncmp("#if", text, CPP_IF_LEN);
  cpp_dir.is_elif     = !strncmp("#elif", text, CPP_ELIF_LEN);
  cpp_dir.is_else     = !strncmp("#else", text, CPP_ELSE_LEN);
  cpp_dir.is_ifdef    = !strncmp("#ifdef", text, CPP_IFDEF_LEN);
  cpp_dir.is_undef    = !strncmp("#undef", text, CPP_UNDEF_LEN);
  cpp_dir.is_endif    = !strncmp("#endif", text, CPP_ENDIF_LEN);
  cpp_dir.is_error    = !strncmp("#error", text, CPP_ERROR_LEN);
  cpp_dir.is_define   = !strncmp("#define", text, CPP_DEFINE_LEN);
  cpp_dir.is_ifndef   = !strncmp("#ifndef", text, CPP_IFNDEF_LEN);
  cpp_dir.is_pragma   = !strncmp("#pragma", text, CPP_PRAGMA_LEN);
  cpp_dir.is_elifdef  = !strncmp("#elifdef", text, CPP_ELIFDEF_LEN);
  cpp_dir.is_include  = !strncmp("#include", text, CPP_INCLUDE_LEN);
  cpp_dir.is_warning  = !strncmp("#warning", text, CPP_WARNING_LEN);
  cpp_dir.is_elifndef = !strncmp("#elifndef", text, CPP_ELIFNDEF_LEN);

  if (NoneDirective(&cpp_dir)) {
    if (*(text+1) == '#') {
      tok->len = 2;
      tok->type = TOK_PASTE;
    } else {
      tok->len = 1;
      tok->type = TOK_STRINGIZE;
    }
    
    return;
  }

  tok->type = TOK_PREPROC;

  SetTokLen(&cpp_dir, tok);

  if (CPPNoNeedContinue(&cpp_dir)) {
    return;
  }

  text += tok->len;
  while (*text) {
    if (*text == '\n') {
      if (*(text-1) != '\\') {
        tok->len++;
        break;
      }
    }

    text++;
    tok->len++;
  }
}

static void TokNumber(Token* tok, const char* text) {
  u8 is_hex   = 0;
  u8 is_float = 0;
  size_t len = 0;
  tok->type = TOK_UB;

  while (*text) {
    u8 lower = (u8)tolower(*text);
    
    if (lower == 'x') {
      if (tok->type == TOK_UB) {
        tok->type = TOK_HEX;
      }
      
      is_hex = 1;
    } else if (lower == 'f' || lower == '.') {
      if (tok->type == TOK_UB) {
        tok->type = TOK_FLOAT;
      }
      
      is_float = 1;
    } else if (lower != 'u' && lower != 'l' && !isdigit(lower)) {
      break;
    }

    len++;
    text++;
  }

  tok->len = len;

  if (!is_hex && !is_float) {
    tok->type = TOK_NUMBER;
  }

  if (is_hex && is_float) {
    tok->type = TOK_UB;
  }
}

static void TokUB(Token* tok) {
  tok->type = TOK_UB;
  tok->len = 1;
}

static void TokVar(Token* tok, size_t len) {
  tok->type = TOK_VAR;
  tok->len = len;
}

static void TokComment(Token* tok, const char* text) {
  tok->type = TOK_COMMENT;
  tok->len = 0;
  
  if (*(text+1) == '/') {
    while (*text && *text != '\n') {
      text++;
      tok->len++;
    }

    tok->len++;
  } else if (*(text+1) == '*') {
    const char* end = strstr(text, "*/");
    if (end) {
      ptrdiff_t diff = end - text;
      tok->len = (size_t)(diff + 2);
    } else {
      while (*text) {
        text++;
        tok->len++;
      } 
    }
  } else {
    tok->type = TOK_PUNCT;
    tok->len++;
  }
}

static void TokSpace(Token* tok, size_t len) {
  tok->type = TOK_SPACE;
  tok->len = len;
}

#define TOTAL_KEYWORDS 63
#define TOTAL_OFFSETS  16
#define OUT_OF_BOUNDS_KW_LEN 14

static int LikeKeyword(Token* tok) {
  u8 cur = 0;
  
  const struct {
    char* name;
    u8 len;
  } Keywords[TOTAL_KEYWORDS] = {
    {"do", 2},             {"if", 2},             {"int", 3},
    {"for", 3},            {"asm", 3},            {"auto", 4},
    {"enum", 4},           {"void", 4},           {"char", 4},
    {"long", 4},           {"goto", 4},           {"case", 4},
    {"FILE", 4},           {"else", 4},           {"bool", 4},
    {"true", 4},           {"union", 5},          {"const", 5},
    {"_Bool", 5},          {"short", 5},          {"while", 5},
    {"false", 5},          {"break", 5},          {"float", 5},
    {"struct", 6},         {"switch", 6},         {"typeof", 6},
    {"double", 6},         {"static", 6},         {"extern", 6},
    {"size_t", 6},         {"sizeof", 6},         {"signed", 6},
    {"inline", 6},         {"return", 6},         {"typedef", 7},
    {"default", 7},        {"defined", 7},        {"alignas", 7},
    {"alignof", 7},        {"nullptr", 7},        {"_Atomic", 7},
    {"_BitInt", 7},        {"volatile", 8},       {"unsigned", 8},
    {"continue", 8},       {"restrict", 8},       {"register", 8},
    {"_Alignas", 8},       {"_Alignof", 8},       {"_Complex", 8},
    {"_Generic", 8},       {"constexpr", 9},      {"_Noreturn", 9},
    {"_Imaginary", 10},    {"_Decimal32", 10},    {"_Decimal64", 10},
    {"_Decimal128", 11},   {"thread_local", 12},  {"_Thread_local", 13},
    {"typeof_unqual", 13}, {"static_assert", 13}, {"_Static_assert", 14},
  };

  const u8 offsets[TOTAL_OFFSETS] = {
     0,  0,  0,  2,  5, 16, 24, 35,
    43, 52, 54, 57, 58, 59, 62, 63,
  };

  if (!tok || tok->len > OUT_OF_BOUNDS_KW_LEN || tok->len < 2) {
    return 0;
  }

  cur = offsets[tok->len];

  for (; cur < offsets[tok->len+1]; cur++) {
    if (!strncmp(tok->text_ptr, Keywords[cur].name, Keywords[cur].len)) {
      tok->type = TOK_KEYWORD;
      tok->len = Keywords[cur].len;
      return 1;
    }
  }
  
  return 0;
}

/* Parse text to token */
static void ParseToToken(Token* tok, const char* text) {
  size_t word_len = GetWordLen(text);

  if (!tok) {
    return;
  }

  tok->text_ptr = text;

  if (word_len == 0) {
    TokUB(tok);
    return;
  }

  switch (*text) {
    case '\\': TokLineCont(tok, text);  return;
    case '/':  TokComment(tok, text);   return;
    case ';':  TokEndLine(tok);         return;
    case '"':  TokString(tok, text);    return;
    case '\'': TokChar(tok, text);      return;
    case '#':  TokHash(tok, text);      return;
    case ' ':  TokSpace(tok, word_len); return;
    case '(':  TokLParen(tok);          return;
    case '!':  TokBitUn(tok);           return;
    default:                             break;
  }

  if (isdigit(*text)) {
    TokNumber(tok, text);
  } else if (ispunct(*text) && !IsPunctIgnore(*text)) {
    TokPunct(tok, word_len);
  } else if (isalpha(*text) || *text == '_') {
    TokVar(tok, word_len);
    LikeKeyword(tok);
  } else {
    TokUB(tok);
  }
}

/* Count tokens */
static size_t CountNeededTokens(const char* code) {
  size_t needed = 0;
  Token tmp_tok = {TOK_UB, NULL, 0};
  const char* end = code + strlen(code);

  while (code < end) {
    ParseToToken(&tmp_tok, code);
    code += tmp_tok.len;
    needed++;
  }
  
  return needed;
}

static void ParseCode(Token* array, const char* code) {
  Token* cur_tok = array;
  const char* end = code + strlen(code);

  while (code < end) {
    ParseToToken(cur_tok, code);
    code += cur_tok->len;
    cur_tok++;
  }
}

static void PrintColor(const Token* const tok, const char* const color) {
  (void)fputs(color, stdout);
  (void)fwrite(tok->text_ptr, 1, tok->len, stdout);
  (void)fputs("\033[0m", stdout);
}

static int IsOnlyCaps(const char* var, size_t len) {
  size_t cur = 0;

  for (; cur < len; cur++) {
    if (!isupper(var[cur]) && var[cur] != '_') {
      return 0;
    }
  }

  return 1;
}

static void PrintVar(const Token* tok) {
  if ((tok+1)->type == TOK_LPAREN) {
    PrintColor(tok, "\033[33m");
  } else if ((tok+1)->type == TOK_SPACE) {
    if ((tok+2)->type == TOK_VAR) {
      PrintColor(tok, "\033[36m");
    } else {
      (void)fwrite(tok->text_ptr, 1, tok->len, stdout);
    }
  } else if (IsOnlyCaps(tok->text_ptr, tok->len)) {
    PrintColor(tok, "\033[34m");
  } else {
    (void)fwrite(tok->text_ptr, 1, tok->len, stdout);
  }
}

#define INCLUDE_CPP_LEN 8

static void PrintKeyword(const Token* tok) {
  if (!strncmp(tok->text_ptr, "#include", INCLUDE_CPP_LEN)) {
    (void)fputs("\033[31m#include\033[34m", stdout);
    if (tok->len > INCLUDE_CPP_LEN) {
      (void)fwrite(tok->text_ptr+INCLUDE_CPP_LEN, 1,
                   tok->len-INCLUDE_CPP_LEN, stdout);
    }
    
    (void)fputs("\033[0m", stdout);
  } else {
    PrintColor(tok, "\033[31m");
  }
}

static void PrintToken(const Token* tok) {
  switch (tok->type) {
    case TOK_UB:
    default:
      (void)fwrite(tok->text_ptr, 1, tok->len, stdout);
      break;
    case TOK_STRING:
    case TOK_CHAR:
      PrintColor(tok, "\033[32m");
      break;
    case TOK_NUMBER:
    case TOK_FLOAT:
    case TOK_HEX:
      PrintColor(tok, "\033[35m");
      break;
    case TOK_PREPROC:
    case TOK_KEYWORD:
    case TOK_BIT_UN:
      PrintKeyword(tok);
      break;
    case TOK_LINE_CONT:
    case TOK_COMMENT:
      PrintColor(tok, "\033[2m");
      break;
    case TOK_STRINGIZE:
    case TOK_PASTE:
    case TOK_PUNCT:
    case TOK_LPAREN:
    case TOK_ENDLINE:
      PrintColor(tok, "\033[34m");
      break;
    case TOK_VAR:
      PrintVar(tok);
      break;
  }
}

static void PrintParsedTokens(const Token* array, const Token* const end) {
  const Token* cur_tok = array;
  
  while (cur_tok != end) {
    PrintToken(cur_tok);
    cur_tok++;
  }
}

/* Source highlight function */
static void SourceHighlight(const char* code) {
  size_t needed = CountNeededTokens(code);
  Token* tokens = NULL;

  if (needed == 0) {
    return;
  }

  tokens = (Token*)calloc(needed+1, sizeof(Token));
  if (!tokens) {
    (void)fputs("cpaint3: cannot allocate memory\n", stdout);
    return;
  }

  tokens[needed].type = TOK_EOF;

  ParseCode(tokens, code);

  PrintParsedTokens(tokens, &tokens[needed]);

  free(tokens);
}

/* Find at end function. */
static int HasAtEnd(const char* str1, const char* str2) {
  size_t len1 = strlen(str1);
  size_t len2 = strlen(str2);
  int cur1 = 0;
  int cur2 = 0;

  if (len1 < len2) {
    return 0;
  }

  cur1 = (int)len1-1;
  cur2 = (int)len2-1;
  
  for (; cur1 >= 0 && cur2 >= 0; cur1--, cur2--) {
    if (str1[cur1] != str2[cur2]) {
      return 0;
    }
  }
  
  return 1;
}

static int HasVersionFlag(int argc, char** argv) {
  int arg = 1;
  for (; arg < argc; arg++) {
    if (!strcmp(argv[arg], "--version") || !strcmp(argv[arg], "-V")) {
      return 1;
    }
  }

  return 0;
}

/* Main function. */
int main(int argc, char** argv) {
  char* buffer = NULL;
  FILE* file = NULL;
  long size = 0;

  if (argc < 2) {
    printf("Usage:\n    %s FILE\n    %s -V|--version\n", argv[0], argv[0]);
    return 1;
  }

  if (HasVersionFlag(argc, argv)) {
    (void)fputs("cpaint3 v1.0.0 - big code update\n"
                "Copyright (C) TheCRatTime\n", stdout);
    return 0;
  }

  if (!HasAtEnd(argv[1], ".c") && !HasAtEnd(argv[1], ".h")) {
    printf("%s: %s: isn't a header or source\n", argv[0], argv[1]);
    return 1;
  }

  file = fopen(argv[1], "rb");
  if (file == NULL) {
    perror(argv[0]);
    return 1;
  }

  (void)fseek(file, 0, SEEK_END);
  size = ftell(file);
  (void)fseek(file, 0, SEEK_SET);

  if (size <= 0) {
    printf("%s: file size %lu ignored\n", argv[0], (unsigned long)size);
    (void)fclose(file);
    return 1;
  }

  buffer = (char*)malloc((size_t)size+1);
  if (buffer == NULL) {
    perror(argv[0]);
    (void)fclose(file);
    return 1;
  }

  if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
    perror(argv[0]);
    (void)fclose(file);
    free(buffer);
    return 1;
  }

  buffer[size] = '\0';

  (void)fclose(file);

  SourceHighlight(buffer);

  free(buffer);
  return 0;
}
