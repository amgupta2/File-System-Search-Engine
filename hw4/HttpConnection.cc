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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;

namespace hw4 {

static const char *kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;
static const int kBufferSize = 1024;

bool HttpConnection::GetNextRequest(HttpRequest *const request) {
  // Use WrappedRead from HttpUtils.cc to read bytes from the files into
  // private buffer_ variable. Keep reading until:
  // 1. The connection drops
  // 2. You see a "\r\n\r\n" indicating the end of the request header.
  //
  // Hint: Try and read in a large amount of bytes each time you call
  // WrappedRead.
  //
  // After reading complete request header, use ParseRequest() to parse into
  // an HttpRequest and save to the output parameter request.
  //
  // Important note: Clients may send back-to-back requests on the same socket.
  // This means WrappedRead may also end up reading more than one request.
  // Make sure to save anything you read after "\r\n\r\n" in buffer_ for the
  // next time the caller invokes GetNextRequest()!

  // STEP 1:
  while (true) {
    size_t header_end_pos = buffer_.find(kHeaderEnd);

    // If we are at end of request header, process and return
    if (header_end_pos != std::string::npos) {
      std::string header = buffer_.substr(0, header_end_pos + kHeaderEndLen);
      *request = ParseRequest(header);
      buffer_ = buffer_.substr(header_end_pos + kHeaderEndLen);
      return true;
    }

    // Read more data into the buffer
    std::vector<char> read_buffer(kBufferSize);
    int bytes_read = WrappedRead(fd_,
      reinterpret_cast<unsigned char*>(read_buffer.data()), kBufferSize);

    // End of file or error reading bytes
    if (bytes_read <= 0) {
      return false;
    }

    buffer_.append(read_buffer.data(), bytes_read);
  }
}

bool HttpConnection::WriteResponse(const HttpResponse &response) const {
  string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         reinterpret_cast<const unsigned char*>(str.c_str()),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(const string &request) const {
  HttpRequest req("/");  // by default, get "/".

  // Plan for STEP 2:
  // 1. Split the request into different lines (split on "\r\n").
  // 2. Extract the URI from the first line and store it in req.URI.
  // 3. For the rest of the lines in the request, track the header name and
  //    value and store them in req.headers_ (e.g. HttpRequest::AddHeader).
  //
  // Hint: Take a look at HttpRequest.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for:
  // - Splitting a string into lines on a "\r\n" delimiter
  // - Trimming whitespace from the end of a string
  // - Converting a string to lowercase.
  //
  // Note: If a header is malformed, skip that line.

  // STEP 2:

  // Split the request into lines based on delimiter described above
  std::vector<std::string> lines;
  boost::algorithm::split(lines, request, boost::is_any_of("\r\n"));

  // Check if the first line exists
  if (!lines.empty()) {
    // Split the first line into method, URI, and HTTP version
    std::vector<std::string> request_line;
    boost::algorithm::split(request_line, lines[0], boost::is_any_of(" "));

    if (request_line.size() >= 2) {
      // Extract and set the URI.
      req.set_uri(request_line[1]);
    }
  }

  // Process the remaining lines as headers.
  for (size_t i = 1; i < lines.size(); ++i) {
    const std::string &line = lines[i];
    if (line.empty()) {
      continue;  // Skip empty lines
    }

    // Split the line into a header name and value
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0 ||
      colon_pos == line.length() - 1) {
      continue;  // Header format is not correct --> skip
    }

    std::string header_name = line.substr(0, colon_pos);
    std::string header_value = line.substr(colon_pos + 1);

    // Trim whitespace and process header name to lowercase.
    boost::algorithm::trim(header_name);
    boost::algorithm::trim(header_value);
    boost::algorithm::to_lower(header_name);

    // Add the header to the request object
    req.AddHeader(header_name, header_value);
  }

  return req;
}

}  // namespace hw4
