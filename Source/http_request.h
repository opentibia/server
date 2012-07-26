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

#ifndef __OTSERV_HTTPREQUEST_H__
#define __OTSERV_HTTPREQUEST_H__

// Extremely trivial implementation of the HTTP 1.1 protocol
// Synchrounous, so DO NOT use it while the server is running as it will hang

#include <map>
#include <string>

namespace HTTP
{
	enum Method
	{
		GET,
		POST
	};

	typedef std::map<std::string, std::string> Headers;

	class Request
	{
	public:
		Request();
		
		Request& url(const std::string& url);
		Request& method(Method method);
		Request& header(const std::string& name, const std::string& value);
		Request& header(const Headers& headers);
		Request& data(const std::string& data);

		// This call will connect to the server with the specfied params and fetch the response
		Request& fetch();

		// Used to access response
		std::string error() const;
		int responseCode() const;
		std::string responseMessage() const;
		std::string responseData() const;
		Headers responseHeaders() const;

	protected:
		// Request data
		std::string requestUrl;
		std::string requestData;
		Method requestMethod;
		Headers requestHeaders;
		std::string payload;
		
		// Return data
		int respCode;
		std::string respCodeStr;
		std::string internalError;
		std::string response;
		Headers respHeaders;
	};
};

#endif
