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
#define TOK_LPAREN    16 /* ( */
#define TOK_BIT_UN    17 /* ! */

/* C language token struct */
typedef struct Token {
  TokenType type;       /* Type of token */
  const char* text_ptr; /* Pointer to text */
  size_t len;           /* Length of token */
} Token;

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
  } else if (isdigit(*text)) {
    while (isdigit(*text)) {
      text++;
      len++;
    }

    return len;
  } else if (isspace(*text)) {
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

static void TokHash(Token* tok, const char* text) {
  u8 is_if       = !strncmp("#if", text, 3);
  u8 is_elif     = !strncmp("#elif", text, 5);
  u8 is_else     = !strncmp("#else", text, 5);
  u8 is_ifdef    = !strncmp("#ifdef", text, 6);
  u8 is_undef    = !strncmp("#undef", text, 6);
  u8 is_endif    = !strncmp("#endif", text, 6);
  u8 is_error    = !strncmp("#error", text, 6);
  u8 is_define   = !strncmp("#define", text, 7);
  u8 is_ifndef   = !strncmp("#ifndef", text, 7);
  u8 is_pragma   = !strncmp("#pragma", text, 7);
  u8 is_elifdef  = !strncmp("#elifdef", text, 8);
  u8 is_include  = !strncmp("#include", text, 8);
  u8 is_warning  = !strncmp("#warning", text, 8);
  u8 is_elifndef = !strncmp("#elifndef", text, 9);

  if (!is_if && !is_elif && !is_else && !is_ifdef && !is_undef &&
      !is_endif && !is_define && !is_ifndef && !is_pragma &&
      !is_elifdef && !is_include && !is_elifndef && !is_error &&
      !is_warning) {
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

  if (is_ifdef)         { tok->len = 6; }
  else if (is_ifndef)   { tok->len = 7; }
  else if (is_elifdef)  { tok->len = 8; }
  else if (is_elifndef) { tok->len = 9; }
  else if (is_if)       { tok->len = 3; }
  else if (is_elif)     { tok->len = 5; }
  else if (is_else)     { tok->len = 5; }
  else if (is_include)  { tok->len = 8; }
  else if (is_warning)  { tok->len = 8; }
  else if (is_pragma)   { tok->len = 7; }
  else if (is_undef)    { tok->len = 6; }
  else if (is_endif)    { tok->len = 6; }
  else if (is_error)    { tok->len = 6; }
  else if (is_define)   { tok->len = 7; }

  if (!is_include && !is_warning && !is_error) {
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
    char* end = strstr(text, "*/");
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

#define TOTAL_KEYWORDS 38
#define TOTAL_OFFSETS  10

static int LikeKeyword(Token* tok) {
  u8 i = 0;
  
  const struct {
    char* name;
    u8 len;
  } Keywords[TOTAL_KEYWORDS] = {
    {"do", 2},       {"if", 2},       {"int", 3},
    {"for", 3},      {"void", 4},     {"char", 4},
    {"long", 4},     {"goto", 4},     {"case", 4},
    {"FILE", 4},     {"else", 4},     {"bool", 4},
    {"true", 4},     {"union", 5},    {"const", 5},
    {"short", 5},    {"while", 5},    {"false", 5},
    {"break", 5},    {"float", 5},    {"struct", 6},
    {"switch", 6},   {"double", 6},   {"static", 6},
    {"extern", 6},   {"size_t", 6},   {"sizeof", 6},
    {"signed", 6},   {"inline", 6},   {"return", 6},
    {"typedef", 7},  {"default", 7},  {"defined", 7},
    {"volatile", 8}, {"unsigned", 8}, {"continue", 8},
    {"restrict", 8}, {"register", 8},
  };

  const u8 offsets[TOTAL_OFFSETS] = {
    0, 0, 0, 2, 4, 13, 20, 30, 33, 38
  };

  if (!tok || tok->len > 8 || tok->len < 2) {
    return 0;
  }

  i = offsets[tok->len];

  for (; i < offsets[tok->len+1]; i++) {
    if (!strncmp(tok->text_ptr, Keywords[i].name, Keywords[i].len)) {
      tok->type = TOK_KEYWORD;
      tok->len = Keywords[i].len;
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
  } else if (ispunct(*text)) {
    TokPunct(tok, word_len);
  } else if (isalpha(*text)) {
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

static void PrintColor(Token* tok, const char* color) {
  fputs(color, stdout);
  fwrite(tok->text_ptr, 1, tok->len, stdout);
  fputs("\033[0m", stdout);
}

static int IsOnlyCaps(const char* var, size_t len) {
  size_t i = 0;

  for (; i < len; i++) {
    if (!isupper(var[i]) && var[i] != '_') {
      return 0;
    }
  }

  return 1;
}

static void PrintVar(Token* tok) {
  if ((tok+1)->type == TOK_LPAREN) {
    PrintColor(tok, "\033[33m");
  } else if ((tok+1)->type == TOK_SPACE) {
    if ((tok+2)->type == TOK_VAR) {
      PrintColor(tok, "\033[36m");
    } else {
      fwrite(tok->text_ptr, 1, tok->len, stdout);
    }
  } else if (IsOnlyCaps(tok->text_ptr, tok->len)) {
    PrintColor(tok, "\033[34m");
  } else {
    fwrite(tok->text_ptr, 1, tok->len, stdout);
  }
}

static void PrintKeyword(Token* tok) {
  if (!strncmp(tok->text_ptr, "#include", 8)) {
    fputs("\033[31m#include\033[34m", stdout);
    if (tok->len > 8) {
      fwrite(tok->text_ptr+8, 1, tok->len-8, stdout);
    }
    
    fputs("\033[0m", stdout);
  } else {
    PrintColor(tok, "\033[31m");
  }
}

static void PrintToken(Token* tok) {
  switch (tok->type) {
    case TOK_UB:
    default:
      fwrite(tok->text_ptr, 1, tok->len, stdout);
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

static void PrintParsedTokens(Token* array, Token* end) {
  Token* cur_tok = array;
  
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
    fputs("cpaint3: cannot allocate memory\n", stdout);
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
  int i = 0;
  int j = 0;

  if (len1 < len2) {
    return 0;
  }

  i = (int)len1-1;
  j = (int)len2-1;
  
  for (; i >= 0 && j >= 0; i--, j--) {
    if (str1[i] != str2[j]) {
      return 0;
    }
  }
  
  return 1;
}

/* Main function. */
int main(int argc, char** argv) {
  char* buffer = NULL;
  FILE* file = NULL;
  size_t size = 0;

  if (argc < 2) {
    printf("Usage: %s FILE\n", argv[0]);
    return 1;
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

  fseek(file, 0, SEEK_END);
  size = (size_t)ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size <= 0) {
    printf("%s: file size %lu ignored\n", argv[0], (unsigned long)size);
    fclose(file);
    return 1;
  }

  buffer = (char*)malloc(size+1);
  if (buffer == NULL) {
    perror(argv[0]);
    fclose(file);
    return 1;
  }

  if (fread(buffer, 1, size, file) != size) {
    perror(argv[0]);
    fclose(file);
    free(buffer);
    return 1;
  }

  buffer[size] = '\0';

  fclose(file);

  SourceHighlight(buffer);

  free(buffer);
  return 0;
}
