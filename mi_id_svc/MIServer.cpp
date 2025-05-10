#include "pch.h"
#include "MIServer.h"
//#include <atltime.h>

ST_RESPONSE* lv_pstRes = NULL;
#define LD_MAX_TRIAL_COUNT 100
int lv_nTrialCount = 50 * 2;
/*
void ClaHTTPServerWrapper::launch() {
	
	if (lv_pstRes == NULL) {
		lv_pstRes = (ST_RESPONSE*)malloc(sizeof(ST_RESPONSE));
		if (mil_read_license(lv_pstRes) == 0) {
			free(lv_pstRes);
			lv_pstRes = NULL;
		}
	}
	run();
}
*/

void MyRequestHandler::OnVersion(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");
	ostream& ostr = response.send();
	char szOut[MAX_PATH]; memset(szOut, 0, sizeof(szOut));
	sprintf_s(szOut, "Version : %s\nUpdate : %s", GD_ID_VERSION, GD_ID_UPDATE);
	ostr << szOut;
}

void MyRequestHandler::OnProcess(HTTPServerRequest& request, HTTPServerResponse& response)
{
// 	CTime timeCur = CTime::GetCurrentTime();
	// Get the current time point
	auto now = std::chrono::system_clock::now();

	// Convert the time point to a time_t object
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);

	if (lv_pstRes == NULL || lv_pstRes->m_lExpire < now_c) {
		if (lv_nTrialCount > LD_MAX_TRIAL_COUNT) {
			OnNoLicense(request, response); return;
		}
		lv_nTrialCount--;
		if (lv_nTrialCount < 0) {
			OnNoLicense(request, response); return;
		}
	}
	std::string strUri = GD_API_PROCESS_INNER;
	// Send the request to the target server
	HTTPClientSession session("127.0.0.1", 8080);
	HTTPRequest clientRequest(request.getMethod(), strUri, request.getVersion());
	clientRequest.setContentType(request.getContentType());
	clientRequest.setContentLength(request.getContentLength());
	session.sendRequest(clientRequest) << request.stream().rdbuf();

	// Receive the response from the target server
	HTTPResponse clientResponse;
	istream& clientResponseStream = session.receiveResponse(clientResponse);

	// Forward the response to the client
	response.setStatus(clientResponse.getStatus());
	response.setContentType(clientResponse.getContentType());
	response.setContentLength(clientResponse.getContentLength());
	response.send() << clientResponseStream.rdbuf();
}

void MyRequestHandler::OnUnknown(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");
	ostream& ostr = response.send();
	ostr << "Not found";
}

void MyRequestHandler::OnNoLicense(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");
	ostream& ostr = response.send();
	ostr << "Please input license.";
}