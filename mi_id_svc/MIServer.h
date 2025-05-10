#pragma once

#include "MiConf.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
//#include "Poco/Util/ServerApplication.h"
#include "Poco/Net/HTTPClientSession.h"
// #include "Poco/Net/HTTPClientRequest.h"
// #include "Poco/Net/HTTPClientResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"
#include <iostream>
#include "..\cmn\MiKeyMgr.h"

using namespace Poco::Net;
//using namespace Poco::Util;
using namespace std;

// Define a request handler to handle incoming HTTP requests
class MyRequestHandler : public HTTPRequestHandler {
public:
	void OnVersion(HTTPServerRequest& request, HTTPServerResponse& response);
	void OnProcess(HTTPServerRequest& request, HTTPServerResponse& response);
	void OnUnknown(HTTPServerRequest& request, HTTPServerResponse& response);
	void OnNoLicense(HTTPServerRequest& request, HTTPServerResponse& response);
public:
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
		try {
			OutputDebugStringA(request.getURI().c_str());
			if (request.getURI().compare(GD_API_VERSION) == 0) {
				OnVersion(request, response);
				return;
			}
			if (request.getURI().compare(GD_API_PROCESS) == 0) {
				OnProcess(request, response);
				return;
			}

			OnUnknown(request, response);
		}
		catch (Poco::Exception& ex) {
			response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
			response.send() << "Error: " << ex.displayText();
		}
/*
		// Set the status code for redirection (302 - Found)
		response.setStatus(HTTPResponse::HTTP_FOUND);

		// Set the Location header to the target URL
		response.add("Location", "http://127.0.0.1:8080");

		// Send an empty response body
		response.setContentLength(0);
		response.send();
*/

/*
		response.setStatus(HTTPResponse::HTTP_OK);
		response.setContentType("text/plain");
		ostream& ostr = response.send();
		ostr << "Hello, World!";
*/
	}
};

// Define a request handler factory to create instances of MyRequestHandler
class MyRequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest&) override {
		return new MyRequestHandler;
	}
};

/*
// Define the main application class
class ClaHTTPServerWrapper : public ServerApplication {
public:
	void launch();
protected:
	int main(const vector<string>&) override {
		// Set up server parameters
		HTTPServerParams* params = new HTTPServerParams;
		params->setMaxQueued(100);
		params->setMaxThreads(16);

		// Create a new HTTPServer instance
		HTTPServer server(new MyRequestHandlerFactory, ServerSocket(8082), params);

		// Start the server
		server.start();
		cout << "Server started on port 8082." << endl;

		// Wait for CTRL-C or termination signal
		waitForTerminationRequest();

		// Stop the server
		server.stop();
		cout << "Server stopped." << endl;

		return Application::EXIT_OK;
	}
};
*/