#ifndef WEBSERV_H
# define WEBSERV_H

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unordered_map>

#include "Request.hpp"

class Request;

class Webserv
{
    public:
        Webserv();
        Webserv(Webserv const &copy);
        ~Webserv();
        Webserv &operator=(Webserv const &rhs);
        bool init();
        bool process();
        void generateResponse(Request &request);

    private:
        int _listenSd;
        int _maxSd;
        int _returnCode;
        int _serverPort;
        int _newSd;
        int _descReady;
        int _endServ;
        int _closeConn;
        int _run;
        fd_set _masterSet, _readableSet;
        fd_set _writableSet;
        timeval _timeOut;

        Request _request;
        void newConnHandling();
        void existingConnHandling(int currSd);
        void closeConn(int currSd);
        int receiveRequest(int currSd);
        
        int sendResponse(int currSd);
        std::string formating(std::string content);

        int handlingErrorConn();
        int handlingErrorInit(std::string function);


        int _statusCode;
        int _contentLength;
        std::string _pathToContent;
        std::string _rawContent;
        std::string _statusText;
        std::string _contentType;
        std::string _response;
        std::string _protocol;
        std::string _modifExtension;
        void addingFormat(Request &request);
        void configHeader(Request &request);
        void configBody(Request &request);
        void buildingFormattedResponse();
        void getContent();
        void getPathToContent(Request &request);
};

#endif