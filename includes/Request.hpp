#ifndef REQUEST_HPP
#define REQUEST_HPP
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

class Request {
	public:
		Request();
		Request(const std::string& s_request);
		~Request();
		Request&	operator=(const Request& rhs);
		Request(const Request& cpy);
		
		std::map<std::string, std::string> data;

		int		getStatus() const;
		
	private:
		void	display();
		void	parseRequest();
		void	parseFirstLine(const std::string& line);
		void	parseHeader(const std::string& line);
		void	parseRawBody();

		int _status;
		
};

#endif