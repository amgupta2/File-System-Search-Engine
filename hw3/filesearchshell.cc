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

#include <cstdlib>    // for EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>   // for std::cout, std::cerr, etc.
#include <cctype>     // for tolower
#include <sstream>    // for std::stringstream
#include <list>
#include <vector>
#include <string>
#include "./QueryProcessor.h"  // for QueryProcessor class

using namespace hw3;
using std::cerr;
using std::endl;

// Function declarations

// Error usage message for the client to see
// Arguments:
// - prog_name: Name of the program
static void Usage(char* prog_name);

// Converts a string to lowercase via per character changes
// Arguments:
// - input: String to convert to lowercase
static void ToLowerCase(std::string *input);

// Prints out the result of a query
// Arguments:
// - query_result: The result of the query containing subfields
static void PrintResult(const QueryProcessor::QueryResult &query_result);

// Handles the main interactive shell loop for processing user queries
// Arguments:
// - processor: A reference to the QueryProcessor object for handling queries
static void RunInteractiveShell(const QueryProcessor &processor);

// Reads and returns a query from the user
// Arguments: None
// Returns: A string contianing the user's query input
static std::string GetUserQueryInput();

// Parses a user query string into a vector of words
// Arguments:
// - user_query: A string representing the user's query
// Returns: A vector of string representing individual query words
static std::vector<std::string> ParseQuery(const std::string &user_query);

// Processes the parsed query and prints the results
// Arguments:
// - processor: A reference to the QueryProcessor object for handling queries
// - query: A vector of strings representing the parsed query words
static void ProcessAndDisplayQueryResults(const QueryProcessor &processor,
  const std::vector<std::string> &query);

// Main function handling program setup and the main loop
int main(int argc, char** argv) {
  if (argc < 2) {
    Usage(argv[0]);
  }

  // Load index files into a list
  std::list<std::string> index_files;
  for (int i = 1; i < argc; i++) {
    index_files.push_back(argv[i]);
  }

  // Instantiate QueryProcessor
  QueryProcessor processor(index_files);

  // Run the main interactive shell loop
  RunInteractiveShell(processor);

  return EXIT_SUCCESS;
}

static void RunInteractiveShell(const QueryProcessor &processor) {
  while (true) {
    std::string user_query = GetUserQueryInput();
    if (user_query.empty() && std::cin.eof()) {
      break;  // Handle EOF to exit the loop
    }

    std::vector<std::string> query = ParseQuery(user_query);
    ProcessAndDisplayQueryResults(processor, query);
  }
}

static std::string GetUserQueryInput() {
  std::string user_query;
  std::cout << "Enter query:" << std::endl;
  std::getline(std::cin, user_query);
  ToLowerCase(&user_query);
  return user_query;
}

static std::vector<std::string> ParseQuery(const std::string &user_query) {
  std::vector<std::string> query;
  std::stringstream ss(user_query);
  std::string query_word;
  while (ss >> query_word) {
    query.push_back(query_word);
  }
  return query;
}

static void ProcessAndDisplayQueryResults(const QueryProcessor &processor,
  const std::vector<std::string> &query) {
  std::vector<QueryProcessor::QueryResult> query_res =
    processor.ProcessQuery(query);

  if (!query_res.empty()) {
    for (const auto &item : query_res) {
      PrintResult(item);
    }
  } else {
    std::cout << " [no results]" << std::endl;
  }
}

static void PrintResult(const QueryProcessor::QueryResult &query_result) {
  std::cout << " " << query_result.document_name << " (" << query_result.rank
            << ")" << std::endl;
}

static void ToLowerCase(std::string *input) {
  for (std::string::size_type i = 0; i < input->length(); i++) {
    (*input)[i] = tolower((*input)[i]);
  }
}

static void Usage(char* prog_name) {
  cerr << "Usage: " << prog_name << " [index files+]" << endl;
  exit(EXIT_FAILURE);
}
