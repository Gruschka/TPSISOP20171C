/*
 * sockets.c
 *
 *  Created on: Apr 22, 2017
 *      Author: utnso
 */

#include "sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#include <commons/log.h>

extern t_log *logger;

#define MAXEVENTS 64

int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1) { return -1; }

  return 0;
}

int create_and_bind (char *port) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (NULL, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
	  int iSetOption = 1;
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,
              sizeof(iSetOption));
      if (sfd == -1)
        continue;

      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }

      close (sfd);
    }

  if (rp == NULL)
    {
      log_error(logger, "Could not bind");
      return -1;
    }

  freeaddrinfo (result);

  return sfd;
}

int createServer(char *port, EpollConnectionEventHandler newConnectionHandler, EpollIncomingDataEventHandler incomingDataHandler, EpollDisconnectionEventHandler disconnectionHandler) {
  int sfd, s;
  int efd;
  struct epoll_event event;
  struct epoll_event *events;

  sfd = create_and_bind (port);
  if (sfd == -1) return -1;

  s = make_socket_non_blocking (sfd);
  if (s == -1) return -1;

  s = listen (sfd, SOMAXCONN);
  if (s == -1) { return -1; }

  efd = epoll_create1 (0);
  if (efd == -1)
    {
      perror ("epoll_create");
      abort ();
    }

  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;
  s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
  if (s == -1)
    {
      perror ("epoll_ctl");
      abort ();
    }

  /* Buffer where events are returned */
  events = calloc (MAXEVENTS, sizeof event);

  /* The event loop */
  while (1)
    {
      int n, i;

      n = epoll_wait (efd, events, MAXEVENTS, -1);
      for (i = 0; i < n; i++)
	{
	  if ((events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN)))
	    {
              /* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
	      fprintf (stderr, "epoll error\n");
	      close (events[i].data.fd);
	      continue;
	    }

	  else if (sfd == events[i].data.fd)
	    {
              /* We have a notification on the listening socket, which
                 means one or more incoming connections. */
              while (1)
                {
                  struct sockaddr in_addr;
                  socklen_t in_len;
                  int infd;
                  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                  in_len = sizeof in_addr;
                  infd = accept (sfd, &in_addr, &in_len);
                  if (infd == -1)
                    {
                      if ((errno == EAGAIN) ||
                          (errno == EWOULDBLOCK))
                        {
                          /* We have processed all incoming
                             connections. */
                          break;
                        }
                      else
                        {
                          perror ("accept");
                          break;
                        }
                    }

                  s = getnameinfo (&in_addr, in_len,
                                   hbuf, sizeof hbuf,
                                   sbuf, sizeof sbuf,
                                   NI_NUMERICHOST | NI_NUMERICSERV);
                  if (s == 0)
                    {
                	  log_debug(logger, "New connection. IP: %s", hbuf);
                    }

                  /* Make the incoming socket non-blocking and add it to the
                     list of fds to monitor. */
                  s = make_socket_non_blocking (infd);
                  if (s == -1)
                    abort ();

                  event.data.fd = infd;
                  event.events = EPOLLIN | EPOLLET;
                  s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
                  newConnectionHandler(infd);
                  if (s == -1)
                    {
                      perror ("epoll_ctl");
                      abort ();
                    }
                }
              continue;
            }
          else
            {
              /* We have data on the fd waiting to be read. Read and
                 display it. We must read whatever data is available
                 completely, as we are running in edge-triggered mode
                 and won't get a notification again for the same
                 data. */
              void *buffer = malloc(sizeof(ipc_header));
              ssize_t count;
              count = recv(events[i].data.fd, buffer, sizeof(ipc_header), MSG_PEEK);

              if (count == sizeof(ipc_header)) {
            	  ipc_header *header = malloc(sizeof(ipc_header));
            	  memcpy(header, buffer, sizeof(ipc_header));
            	 // free(buffer);
            	  incomingDataHandler(events[i].data.fd, *header);
            	  free(header);
              } else if (count == 0) {
            	  disconnectionHandler(events[i].data.fd);
              } else {
            	  // Manejar error
              }

              free(buffer);

//              while (1)
//                {
//                  ssize_t count;
//                  char buf[512];
//
//                  count = read (events[i].data.fd, buf, sizeof buf);
//                  if (count == -1)
//                    {
//                      /* If errno == EAGAIN, that means we have read all
//                         data. So go back to the main loop. */
//                      if (errno != EAGAIN)
//                        {
//                          perror ("read");
//                          done = 1;
//                        }
//                      break;
//                    }
//                  else if (count == 0)
//                    {
//                      /* End of file. The remote has closed the
//                         connection. */
//                	  disconnectionHandler(events[i].data.fd);
//                      done = 1;
//                      break;
//                    }
//
//
//                  /* Write the buffer to standard output */
//                  s = write (1, buf, count);
//                  if (s == -1)
//                    {
//                      perror ("write");
//                      abort ();
//                    }
//                }
//
//              if (done)
//                {
//                  printf ("Closed connection on descriptor %d\n",
//                          events[i].data.fd);
//
//                  /* Closing the descriptor will make epoll remove it
//                     from the set of descriptors which are monitored. */
//                  close (events[i].data.fd);
//                  disconnectionHandler(events[i].data.fd);
//                }
            }
        }
    }

  free (events);

  close (sfd);

  return EXIT_SUCCESS;
}
