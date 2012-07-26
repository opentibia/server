//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include "otpch.h"

#include "http_request.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <sstream>

using namespace HTTP;

Request::Request() : requestMethod(GET), respCode(0)
{
}

Request& Request::url(const std::string& url)
{
	requestUrl = url;
	return *this;
}

Request& Request::method(Method method)
{
	requestMethod = method;
	return *this;
}

Request& Request::header(const std::string& name, const std::string& value)
{
	requestHeaders[name] = value;
	return *this;
}

Request& Request::header(const Headers& headers)
{
	for(Headers::const_iterator it = headers.begin(); it != headers.end(); ++it)
		requestHeaders[it->first] = it->second;
	return *this;
}

Request& Request::data(const std::string& data)
{
	requestData = data;
	return *this;
}

// Get response
int Request::responseCode() const
{
	return respCode;
}

std::string Request::responseMessage() const
{
	return respCodeStr;
}

std::string Request::responseData() const
{
	return response;
}

// The fun stuff!
Request& Request::fetch()
{
	respCode = 0;
	response = "";
	respHeaders = Headers();

	// Check protocol
	if(requestUrl.substr(0, 7) != "http://"){
		internalError = response = "Only the HTTP protocol is supported";
		respCode = -1;
		return *this;
	}

	// The data after splitting the URI
	std::string domain = "";
	std::string locator = "/";
	std::string port = "80";

	// Find the domain and port
	std::string::size_type domainStart = 7;
	std::string::size_type domainEnd = requestUrl.find('/', domainStart);
	if(domainEnd == std::string::npos){
		// Plain domain, http://example.com
		domain = requestUrl.substr(domainStart);
	}
	else{
		// Path has a file locator http://example.com/data.html for example
		domain = requestUrl.substr(domainStart, domainEnd - domainStart);

		//
		locator = requestUrl.substr(domainEnd);
	}

	// Split the port from the domain
	int portStart = domain.find(':');
	if(portStart != std::string::npos){
		std::istringstream portConverter(domain.substr(portStart + 1));

		portConverter >> port;
		domain = domain.substr(0, portStart);
	}

	// Do the request
	std::stringstream httpResponse;

	try{
		using boost::asio::ip::tcp;
		boost::asio::io_service httpService;

		tcp::resolver resolver(httpService);
		tcp::resolver::query query(domain,  port);
		tcp::resolver::iterator endpointIterator = resolver.resolve(query);
		tcp::resolver::iterator endpointEnd;

		tcp::socket socket(httpService);
		boost::system::error_code error = boost::asio::error::host_not_found;

		while(error && endpointIterator != endpointEnd){
			socket.close();
			socket.connect(*endpointIterator++, error);
		}

		if(error){
			boost::system::system_error err(error);
			internalError = response = err.what();
			respCode = -2;
			return *this;
		}

		std::ostringstream request;
		request <<
			(requestMethod == GET ? "GET" : "POST") << " " << locator << " HTTP/1.0\r\n" <<
			"Host: " << domain << ":" << port << "\r\n" <<
			"\r\n"
		;

		socket.write_some(boost::asio::buffer(request.str()), error);
		if(error == boost::asio::error::eof){
			internalError = response = "Connection closed by remote server.";
			respCode = -2;
			return *this;
		}

		for(;;){
			boost::array<char, 128> buf;
			boost::system::error_code error;

			size_t len = socket.read_some(boost::asio::buffer(buf), error);

			if(error == boost::asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if(error)
				throw boost::system::system_error(error); // Some other error.

			httpResponse.write(buf.data(), len);
		}
	}
	catch(std::exception& e){
		std::cerr << e.what() << std::endl;
	}

	// Got response, now to parse it
	std::string ln;
	int lineNumber = 1;
	bool output = false;
	while(!httpResponse.eof()){
		std::getline(httpResponse, ln);

		// First check if we should just output data
		if(output){
			response += ln;
		}
		
		// Maybe it's the first response line?
		else if(lineNumber == 1){
			// Find space after "HTTP/1.1"
			std::string::size_type firstSpace = ln.find(' ');
			if (firstSpace == std::string::npos){
				respCode = -3;
				internalError = response = "Invalid HTTP response.";
				return *this;
			}
			std::string::size_type secondSpace = ln.find(' ', firstSpace+1);
			if (firstSpace == secondSpace || secondSpace == std::string::npos){
				respCode = -3;
				internalError = response = "Invalid HTTP response.";
				return *this;
			}

			respCodeStr = ln.substr(firstSpace);
			if (respCodeStr.size() > 0)
				// Strip space at the beginning
				respCodeStr = respCodeStr.substr(1);

			std::istringstream code(ln.substr(firstSpace, secondSpace - firstSpace));
			if (code.peek() == ' ')
				code.get();
			if (!code.eof())
				code >> respCode;
		}

		// Have the headers ended?
		else if(ln == "\r"){
			// data comes after this
			output = true;
			continue;
		}

		// Normal header then
		else if(ln.find(": ") != std::string::npos){
			std::string header = ln.substr(0, ln.find(": "));
			std::string value = ln.substr(ln.find(": ") + 2);
			respHeaders[header] = value;
		}

		// Error!
		else{
			respCode = -3;
			internalError = response = "Invalid HTTP response. Unexpected line in header.";
			return *this;
		}

		++lineNumber;
	}

	return *this;
}
