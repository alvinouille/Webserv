#include "Webserv.hpp"

Webserv::Webserv() : _endServ(false), _serverPort(8080), _run(1)
{
    _timeOut.tv_sec = 3 * 60;
    _timeOut.tv_usec = 0;
}

Webserv::~Webserv() {}

Webserv::Webserv(Webserv const &copy) {}

Webserv &Webserv::operator=(Webserv const &rhs)
{
    if (this == &rhs)
        return (*this);
    return (*this);
}

int Webserv::handlingErrorInit(std::string function)
{
    std::cerr << function << "() failed" << std::endl;
    if (_listenSd > 0)
        close(_listenSd);
    return (0);
}

bool Webserv::init()
{
    sockaddr_in6 addr;
    int on = 1;

   _listenSd = socket(AF_INET6, SOCK_STREAM, 0);
    if (_listenSd < 0)
        return (handlingErrorInit("listen"));
    _returnCode = setsockopt(_listenSd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
    if (_returnCode < 0)
        return (handlingErrorInit("setsockopt"));
    _returnCode = ioctl(_listenSd, FIONBIO, (char*)&on);
    if (_returnCode < 0)
        return (handlingErrorInit("ioctl"));

    std::memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
    addr.sin6_port = htons(_serverPort);
    _returnCode = bind(_listenSd, (struct sockaddr *)&addr, sizeof(addr));
    if (_returnCode < 0)
        return (handlingErrorInit("bind"));

    _returnCode = listen(_listenSd, 32);
    if (_returnCode < 0)
        return (handlingErrorInit("listen"));

    FD_ZERO(&_masterSet);
    _maxSd = _listenSd;
    FD_SET(_listenSd, &_masterSet);
    return (1);
}

bool Webserv::process()
{
    while (!_endServ)
    {
        memcpy(&_readableSet, &_masterSet, sizeof(_masterSet));
        FD_ZERO(&_writableSet);
        std::cout << "Waiting on select()..." << std::endl;
        _returnCode = select(_maxSd + 1, &_readableSet, &_writableSet, NULL, &_timeOut);
        if (_returnCode <= 0){
            std::cerr << (_returnCode < 0 ? "select() failed" : "select() timed out. End program") << std::endl;
            perror("select");
            break;
        }
        _descReady = _returnCode;
        for (int i = 0 ; i <= _maxSd && _descReady > 0 ; ++i)
        {
            if (FD_ISSET(i, &_workingSet))
            {
                _descReady -= 1;
                if (i == _listenSd)
                    newConnHandling();
                else
                    existingConnHandling(i);
            }
        }  
    }
    for (int i = 0 ; i <= _maxSd ; i++)
    {
        if (FD_ISSET(i, &_masterSet))
            close(i);
    }
    return (1);
}

void Webserv::newConnHandling()
{
    std::cout << "Listening socket is readable" << std::endl;
    _newSd = 0;
    while (_newSd != -1)
    {
        _newSd = accept(_listenSd, NULL, NULL);
        if (_newSd < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                std::cerr << "accept() failed" << std::endl;
                _endServ = true;
            }
            break ;
        }
        std::cout << "New incoming connection " << _newSd << std::endl;
        FD_SET(_newSd, &_masterSet);
        if (_newSd > _maxSd)
            _maxSd = _newSd;
    }
}
void Webserv::closeConn(int currSd)
{
    std::cout << "closing ! " << std::endl;
    close(currSd);
    FD_CLR(currSd, &_masterSet);
    if (currSd == _maxSd)
    {
        while (FD_ISSET(_maxSd, &_masterSet) == false)
            _maxSd -= 1;
    }
}

void Webserv::existingConnHandling(int currSd)
{
    std::cout << "Descriptor " << currSd << " is readable" << std::endl;
    _closeConn = false;
    while (true)
    {
        if (receiveRequest(currSd) == 0)
            break;
        if (sendResponse(currSd) == 0)
            break;
        if (_closeConn)
            closeConn(currSd);
    }
}

int Webserv::handlingErrorConn()
{
    if (_returnCode < 0)
    {
        if (errno != EWOULDBLOCK)
        {
            std::cerr << "recv() or send() failed" << std::endl;
            _closeConn = true;
        }
        return 0;
    }
    if (_returnCode == 0)
    {
        std::cerr << "Connection closed" << std::endl;
        _closeConn = true;
        return 0;
    }
    return 0;
}

int Webserv::receiveRequest(int currSd)
{
    char bf[BUFFER_SIZE];
    _returnCode = recv(currSd, bf, sizeof( bf), 0);
    if (_returnCode <= 0)
        return (handlingErrorConn());
    _request.requestTreatment(bf);
    return 1;
}


int Webserv::sendResponse(int currSd)
{
    generateResponse(this->_request);
    int rc = send(currSd, this->_response.c_str(), this->_response.length(), 0);
    std::cout << "SENT" << std::endl;
    if (rc < 0)
        return (handlingErrorConn());
    return 1;
}

void Webserv::generateResponse(Request &request)
{
    getPathToContent(request);
    getContent();
    addingFormat(request);

    //print data from request and response built
    std::cout << "EXTENSION = " << request.getExtension() << std::endl;
    std::cout << "ressource = " << request.getRessource() << std::endl;
    std::cout << "PATH = " << this->_pathToContent << std::endl;
    std::cout << this->_response << std::endl;
}

void Webserv::configHeader(Request &request)
{
    this->_statusCode = 200;
    this->_statusText = "OK";
    this->_protocol = request.getProtocol();
}

void Webserv::configBody(Request &request)
{
    this->_modifExtension = (request.getExtension() == "js" ? "javascript" : (request.getExtension() == "jpg" ? "jpeg" : request.getExtension()));
    std::unordered_map<std::string, std::string> map{
        {"webp", "image"},
        {"jpeg", "image"},
        {"html", "text"},
        {"javascript", "application"},
        {"css", "text"}
    };
    this->_contentType = map[this->_modifExtension];
    this->_contentLength = this->_rawContent.length();
}

void Webserv::buildingFormattedResponse()
{
    std::ostringstream response;
    response << this->_protocol << " " << this->_statusCode << " " << this->_statusText << "\r\n";
    response << "Content-Type: " << this->_contentType << "/" << this->_modifExtension << "\r\n";
    response << "Content-Length: " << this->_contentLength << "\r\n";
    response << "\r\n" << this->_rawContent;
    this->_response = response.str();
}

void Webserv::addingFormat(Request &request)
{
    configHeader(request);
    configBody(request);
    buildingFormattedResponse();
}

void Webserv::getPathToContent(Request &request)
{
    this->_pathToContent = "../html/";
    if (request.getExtension() == "webp" || request.getExtension() == "jpg")
        this->_pathToContent += "image/";
    else
        this->_pathToContent += "data/";
    this->_pathToContent += request.getRessource();
}

void Webserv::getContent()
{
    std::ifstream ifs(this->_pathToContent.c_str());
    std::stringstream iss;
    iss << ifs.rdbuf();
    this->_rawContent = iss.str();
}


//      for (int i = 0; i <= _max_fd; ++i) {
        //     if (FD_ISSET(i, &working_set_recv) && i == 0) {
        //         if (userExit())
        //             return(false);
        //     }
        //     else if (FD_ISSET(i, &working_set_recv) && isServerSocket(i)) 
        //         acceptNewCnx(i);
        //     else if (FD_ISSET(i, &working_set_recv) && _requests.count(i))
        //         readRequest(i, *_requests[i]);
        //     else if (FD_ISSET(i, &working_set_write) && _requests.count(i))
        //         sendResponse(i, *_requests[i]); ->to analyse
        // }