/*
 * Copyright ©2024 Hannah C. Tang.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Autumn Quarter 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "./CrawlFileTree.h"
#include "./DocTable.h"
#include "./MemIndex.h"

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations, constants, etc
static void usage(void);
static void process_queries(DocTable* dt, MemIndex* mi);
static int get_next_line(FILE* f, char** ret_str);

// Helper function for cleaning up user queries
static void cleanup(char *user_query, char **query_tokens);


//////////////////////////////////////////////////////////////////////////////
// Main
int main(int argc, char** argv) {
  if (argc != 2) {
    usage();
  }

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - Crawl from a directory provided by argv[1] to produce and index
  //  - Prompt the user for a query and read the query from stdin, in a loop
  //  - Split a query into words (check out strtok_r)
  //  - Process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  //
  // Note that you should make sure the fomatting of your
  // searchshell output exactly matches our solution binaries
  // to get full points on this part.

  char *root_directory = argv[1];
  DocTable* document_table;
  MemIndex* memory_index;

  printf("Indexing '%s'\n", root_directory);

  if (!CrawlFileTree(root_directory, &document_table, &memory_index)) {
    usage();
  }

  process_queries(document_table, memory_index);

  DocTable_Free(document_table);
  MemIndex_Free(memory_index);

  return EXIT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
// Helper function definitions

static void usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

static void process_queries(DocTable* dt, MemIndex* mi) {
  int total_results;
  LinkedList *search_results;
  char *remaining_input;

  // Prompt the User for input and Perform Input Validation
  while (1) {
    char *user_query = (char*) malloc(sizeof(char) * 1024);
    char **query_tokens = (char**) malloc(sizeof(char*) * 1024);

    int end_of_query = get_next_line(stdin, &user_query);

    if (end_of_query != 0) {  // at end of query
      printf("shutting down...\n");
      free(user_query);
      free(query_tokens);
      return;
    }

    // Input Validation/Pre-Processing
    *(user_query + (strlen(user_query) - 1)) = '\0';
    for (int char_index = 0; char_index < strlen(user_query); char_index++) {
      *(user_query + char_index) = tolower(*(user_query + char_index));
    }

    // Split User Input into Query Tokens
    int token_count = 0;
    *(query_tokens) = strtok_r(user_query, " ", &remaining_input);
    while (1) {
      token_count += 1;
      *(query_tokens + token_count) = strtok_r(NULL, " ", &remaining_input);
      if (*(query_tokens + token_count) == NULL) {
        break;
      }
    }

    // Search Documents Via User Input
    search_results = MemIndex_Search(mi, query_tokens, token_count);

    // Did not find any documents
    if (search_results == NULL) {
      cleanup(user_query, query_tokens);
      continue;
    }

    // Iterate through documents to find occurrences of specified words
    SearchResult *search_result;
    LLIterator *list_iterator = LLIterator_Allocate(search_results);
    total_results = LinkedList_NumElements(search_results);

    for (int document_index = 0; document_index < total_results;
      document_index++) {
      LLIterator_Get(list_iterator, (LLPayload_t) &search_result);
      DocID_t document_id = search_result->doc_id;
      char *document_name = DocTable_GetDocName(dt, document_id);

      printf("  %s (%d)\n", document_name, search_result->rank);
      LLIterator_Next(list_iterator);
    }

    // Cleanup
    LinkedList_Free(search_results, (LLPayloadFreeFnPtr) free);
    LLIterator_Free(list_iterator);
    cleanup(user_query, query_tokens);
  }
}

static int get_next_line(FILE *f, char **ret_str) {
  printf("enter query:\n");

  if (fgets(*ret_str, 1024, f) == NULL) {
    return 1;  // End of File
  }

  return 0;  // Successful read
}

static void cleanup(char *user_query, char **query_tokens) {
    free(user_query);
    free(query_tokens);
}
