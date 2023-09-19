#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#define SERVER_PORT  12345

#define TRUE             1
#define FALSE            0


// Fonction pour générer une réponse HTTP à partir d'un contenu HTML
std::string generateResponse(const std::string& htmlContent) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << htmlContent.length() << "\r\n";
    response << "\r\n";
    response << htmlContent;
    return response.str();
}

int main ()
{
   int    i, len, rc, on = 1;
   int    listen_sd, max_sd, new_sd;
   int    desc_ready, end_server = FALSE;
   int    close_conn;
   char   buffer[10024];
   sockaddr_in6   addr;
   timeval       timeout;
   fd_set rfds, wfds;
   FD_ZERO(&rfds);
   FD_ZERO(&wfds);
   listen_sd = socket(AF_INET, SOCK_STREAM, 0);
   rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                   (char *)&on, sizeof(on));
   rc = ioctl(listen_sd, FIONBIO, (char *)&on);
   memset(&addr, 0, sizeof(addr));
   addr.sin6_family      = AF_INET;
   memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
   addr.sin6_port        = htons(SERVER_PORT);
   rc = bind(listen_sd,(struct sockaddr *)&addr, sizeof(addr));
   rc = listen(listen_sd, 32);
   max_sd = listen_sd;
   std::cout << "listen sd = " << listen_sd << std::endl;
   FD_SET(listen_sd, &rfds);
   timeout.tv_sec  = 3 * 60;
   timeout.tv_usec = 0;
   do
   {
      printf("Waiting on select()...\n");
      rc = select(max_sd + 1, &rfds, &wfds, NULL, &timeout);
      if (rc < 0) {
            perror("select() failed");
            exit(EXIT_FAILURE);
        }
        close_conn = FALSE;
      desc_ready = rc;
      for (i=0; i <= max_sd ; ++i)
      {
        std::cout << i << std::endl;
            if (FD_ISSET(i, &rfds) && i == listen_sd)
            {
                desc_ready -= 1;
                if (i == listen_sd)
                {
                printf("  Listening socket is readable\n");
                new_sd = accept(listen_sd, NULL, NULL);
                if (new_sd < 0)
                {
                    if (errno != EWOULDBLOCK)
                    {
                        perror("  accept() failed");
                        end_server = TRUE;
                    }
                    break;
                }
                printf("  New incoming connection - %d\n", new_sd);
                FD_SET(new_sd, &rfds);
                if (new_sd > max_sd)
                    max_sd = new_sd;
                }
            }
            else if (FD_ISSET(i, &rfds))
            {
                close_conn = FALSE;
                desc_ready -= 1;
                rc = recv(i, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
                std::cout << "Reading : " << rc << std::endl;
                if (rc < 0)
                    break;
                if (rc == 0)
                {
                    close_conn = TRUE;
                    break;
                }
                len = rc;
                printf("  %d bytes received\n", len);
                std::cout << buffer << std::endl;
                FD_CLR(i, &rfds);
                FD_SET(i, &wfds);
            }
            else if (FD_ISSET(i, &wfds))
            {
                close_conn = FALSE;
                desc_ready -= 1;
                std::cout << "Writing" << std::endl;
                std::string htmlFilePath = "index.html";
                std::ifstream htmlFile(htmlFilePath);
                std::stringstream htmlContentStream;
                htmlContentStream << htmlFile.rdbuf();
                std::string htmlContent = htmlContentStream.str();
                std::string response = generateResponse(htmlContent);
                // std::cout << response << std::endl;
                rc = send(i, response.c_str(), response.length(), MSG_DONTWAIT);
                if (rc < 0){
                    close_conn = TRUE;
                    break;
                }
                FD_CLR(i, &wfds);
                FD_SET(i, &rfds);
            }
            if (close_conn)
            {
                std::cout << "closing" << std::endl;
                close(i);
                FD_CLR(i, &rfds);
                if (i == max_sd)
                {
                    while (FD_ISSET(max_sd, &rfds) == FALSE)
                        max_sd -= 1;
                }
            }
        }
    } while (end_server == FALSE);
    for (i = 0; i <= max_sd; ++i)
    {
        if (FD_ISSET(i, &rfds))
            close(i);
    }
}