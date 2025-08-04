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

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

using std::string;

constexpr size_t kBufferSize = 1024;

namespace hw4 {

// Helper function to retrieve information for a socket given its fd
static bool GetSocketInfo(int fd, std::string *ip_addr, std::string *dns_name);

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int *const listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // STEP 1:
  if (ai_family != AF_INET && ai_family != AF_INET6 && ai_family != AF_UNSPEC) {
    return false;
  }

  // Populate addrinfo structure to use with getaddrinfo() call
  struct addrinfo hints;

  hints.ai_family = ai_family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;

  std::string port_str = std::to_string(port_);

  // Call getaddrinfo() for a list of address structs into output param
  struct addrinfo *result;

  int get_addr_info_res =
    getaddrinfo(nullptr, port_str.c_str(), &hints, &result);

  if (get_addr_info_res != 0) {
    return false;
  }

  int ret_fd = -1;

  // Iterate through addr structs to create a socket and bind
  for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
    ret_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    // Socket could not be created
    if (ret_fd == -1) {
      continue;
    }

    // Configure the socket if we found one that can be created
    int optval = 1;
    int set_sockopt_res =
      setsockopt(ret_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (set_sockopt_res != 0) {
      close(ret_fd);
      ret_fd = -1;
      continue;
    }

    // Attempt to bind to socket and if successful, exit iterations
    int bind_res = bind(ret_fd, rp->ai_addr, rp->ai_addrlen);
    if (bind_res == 0) {
      sock_family_ = rp->ai_family;
      break;
    }

    // Bind failed so cleanup and continue iterating
    close(ret_fd);
    ret_fd = -1;
  }

  freeaddrinfo(result);

  // Failed to bind after all iterations so should not listen
  if (ret_fd == -1) {
    return false;
  }

  int listen_res = listen(ret_fd, SOMAXCONN);

  if (listen_res != 0) {
    close(ret_fd);
    return false;
  }

  // Successfully binded to socket so use return params
  listen_sock_fd_ = ret_fd;
  *listen_fd = ret_fd;

  return true;
}

bool ServerSocket::Accept(int *const accepted_fd,
                          string *const client_addr,
                          uint16_t *const client_port,
                          string *const client_dns_name,
                          string *const server_addr,
                          string *const server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // STEP 2:
  struct sockaddr_storage client_storage;
  socklen_t client_len = sizeof(client_storage);
  struct sockaddr *client_addr_ptr =
    reinterpret_cast<struct sockaddr *>(&client_storage);

  // Create a client connection or error out
  int client_fd = -1;
  while (true) {
    client_fd = accept(listen_sock_fd_, client_addr_ptr, &client_len);
    if (client_fd >= 0) break;
    if (errno == EAGAIN || errno == EINTR) continue;
    return false;  // Other errors
  }

  // Connection accepted
  *accepted_fd = client_fd;

  // Depending on IP Family, setup client addr and client port
  if (client_addr_ptr->sa_family == AF_INET) {
    // IPv4
    char ip[INET_ADDRSTRLEN];
    auto *addr_in = reinterpret_cast<struct sockaddr_in *>(client_addr_ptr);
    inet_ntop(AF_INET, &addr_in->sin_addr, ip, sizeof(ip));
    *client_addr = std::string(ip);
    *client_port = ntohs(addr_in->sin_port);
  } else if (client_addr_ptr->sa_family == AF_INET6) {
    // IPv6
    char ip[INET6_ADDRSTRLEN];
    auto *addr_in6 = reinterpret_cast<struct sockaddr_in6 *>(client_addr_ptr);
    inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip, sizeof(ip));
    *client_addr = std::string(ip);
    *client_port = ntohs(addr_in6->sin6_port);
  } else {
    close(client_fd);
    return false;  // Unsupported address family
  }

  // Get client dns name
  char client_hostname[kBufferSize];

  if (getnameinfo(client_addr_ptr, client_len, client_hostname,
    sizeof(client_hostname), nullptr, 0, 0) == 0) {
    *client_dns_name = std::string(client_hostname);
  } else {
    *client_dns_name = "<unknown>";
  }

  // If the socket info cannot be retrieved return false due to invalid socket
  if (!GetSocketInfo(client_fd, server_addr, server_dns_name)) {
    close(client_fd);
    return false;
  }

  return true;
}

static bool GetSocketInfo(int fd, std::string *ip_addr, std::string *dns_name) {
  struct sockaddr_storage server_storage;
  socklen_t server_len = sizeof(server_storage);
  struct sockaddr *server_addr_ptr =
    reinterpret_cast<struct sockaddr*>(&server_storage);

  if (getsockname(fd, server_addr_ptr, &server_len) != 0) {
    return false;
  }

  char ip_buffer[INET6_ADDRSTRLEN];
  char hostname[kBufferSize];

  if (server_addr_ptr->sa_family == AF_INET) {
    // IPv4
    auto *addr_in = reinterpret_cast<struct sockaddr_in *>(server_addr_ptr);
    inet_ntop(AF_INET, &addr_in->sin_addr, ip_buffer, sizeof(ip_buffer));
  } else if (server_addr_ptr->sa_family == AF_INET6) {
    // IPv6
    auto *addr_in6 = reinterpret_cast<struct sockaddr_in6 *>(server_addr_ptr);
    inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_buffer, sizeof(ip_buffer));
  } else {
    return false;  // Unsupported address family
  }

  *ip_addr = std::string(ip_buffer);

  if (getnameinfo(server_addr_ptr, server_len, hostname,
    sizeof(hostname), nullptr, 0, 0) == 0) {
    *dns_name = std::string(hostname);
  } else {
    *dns_name = "<unknown>";
  }

  return true;
}

}  // namespace hw4
