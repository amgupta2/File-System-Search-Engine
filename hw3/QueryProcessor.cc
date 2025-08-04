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

#include "./QueryProcessor.h"

#include <iostream>
#include <algorithm>
#include <list>
#include <string>
#include <vector>

extern "C" {
  #include "./libhw1/CSE333.h"
}

using std::list;
using std::sort;
using std::string;
using std::vector;

namespace hw3 {

// Returns the all DocIDElementHeaders in the intersection of two lists
static list<DocIDElementHeader> Intersection(
  const list<DocIDElementHeader> &list1,
  const list<DocIDElementHeader> &list2);

// Returns a list of documents that contain all query terms
static list<DocIDElementHeader> MultiTermIntersection(
  const vector<string> &query, IndexTableReader* index_reader);

// Processes a list of mathcing documents and adds them to the results vector
static void AddMatchingDocuments(const list<DocIDElementHeader> &matching_docs,
  DocTableReader* doc_reader, vector<QueryProcessor::QueryResult> *results);

QueryProcessor::QueryProcessor(const list<string>& index_list, bool validate) {
  // Stash away a copy of the index list.
  index_list_ = index_list;
  array_len_ = index_list_.size();
  Verify333(array_len_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader* [array_len_];
  itr_array_ = new IndexTableReader* [array_len_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::const_iterator idx_iterator = index_list_.begin();
  for (int i = 0; i < array_len_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = fir.NewDocTableReader();
    itr_array_[i] = fir.NewIndexTableReader();
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (int i = 0; i < array_len_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

// This structure is used to store a index-file-specific query result.
typedef struct {
  DocID_t doc_id;  // The document ID within the index file.
  int     rank;    // The rank of the result so far.
} IdxQueryResult;

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string>& query) const {
  Verify333(query.size() > 0);

  // STEP 1.
  // (the only step in this file)
  vector<QueryProcessor::QueryResult> final_result;

  for (int i = 0; i < array_len_; i++) {
    IndexTableReader* index_reader = itr_array_[i];
    DocTableReader* doc_reader = dtr_array_[i];

    list<DocIDElementHeader> matching_docs =
      MultiTermIntersection(query, index_reader);

    AddMatchingDocuments(matching_docs, doc_reader, &final_result);
  }

  // Sort the final results.
  sort(final_result.begin(), final_result.end());
  return final_result;
}

static list<DocIDElementHeader> Intersection(
  const list<DocIDElementHeader>& list1,
  const list<DocIDElementHeader>& list2) {
    list<DocIDElementHeader> result;
    list<DocIDElementHeader>::const_iterator it1;
    list<DocIDElementHeader>::const_iterator it2;

    // Iterate through the first list using the predefined iterator
    for (it1 = list1.begin(); it1 != list1.end(); it1++) {
      // Iterate through the second list using the predefined iterator
      for (it2 = list2.begin(); it2 != list2.end(); it2++) {
        // Check for matching `doc_id`s
        if (it1->doc_id == it2->doc_id) {
          // Combine `num_positions` and add to the result
          DocID_t matching_doc_id = it1->doc_id;
          int32_t total_positions = it1->num_positions + it2->num_positions;
          result.push_back(DocIDElementHeader(matching_doc_id,
            total_positions));
          break;  // Break to avoid duplicate matches for `it1`
        }
      }
    }

    return result;
}

static list<DocIDElementHeader> MultiTermIntersection(
  const vector<string> &query, IndexTableReader* index_reader) {
    // Find first query term
    DocIDTableReader* first_term = index_reader->LookupWord(query[0]);
    if (!first_term) {
      return {};
    }

    // Get document list associated with first term
    list<DocIDElementHeader> matching_docs = first_term->GetDocIDList();
    delete(first_term);

    // Iterate over remaining query terms
    for (size_t j = 1; j < query.size(); j++) {
      // Find the current query term in the index
      DocIDTableReader* curr_term = index_reader->LookupWord(query[j]);
      if (!curr_term) {
        matching_docs.clear();
        break;
      }

      // Get document list for current term
      list<DocIDElementHeader> next_docs = curr_term->GetDocIDList();

      // Intersect with running match list
      matching_docs = Intersection(next_docs, matching_docs);
      delete(curr_term);

      // Exit early if no documents remain in intersection
      if (matching_docs.empty()) {
        break;
      }
    }

    return matching_docs;
  }

static void AddMatchingDocuments(const list<DocIDElementHeader> &matching_docs,
  DocTableReader* doc_reader, vector<QueryProcessor::QueryResult> *results) {
    if (!results) {
      return;
    }

    // Iterate over each matching document
    for (const auto& doc : matching_docs) {
      QueryProcessor::QueryResult result;
      // Lookup document name uisng ID and add rank to results
      if (doc_reader->LookupDocID(doc.doc_id, &result.document_name)) {
        result.rank = doc.num_positions;
        results->push_back(result);
      }
    }
}

}  // namespace hw3
