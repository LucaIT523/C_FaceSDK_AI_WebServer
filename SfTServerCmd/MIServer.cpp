#include "MIServer.h"
#include "MiParserJson.h"
//#include <atltime.h>


ST_RESPONSE* lv_pstRes = NULL;
#define LD_MAX_TRIAL_COUNT 100
int lv_nTrialCount = 50 * 2;

std::string replaceAll(std::string original, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = original.find(search, pos)) != std::string::npos) {
		original.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return original;
}

HTTPResponse::HTTPStatus sendPostRequest(const std::string& url, const std::string& payload, std::string& p_strRsp)
{
	URI uri(url);
	HTTPClientSession session(uri.getHost(), uri.getPort());

	HTTPRequest request(HTTPRequest::HTTP_POST, uri.getPathAndQuery(), HTTPMessage::HTTP_1_1);
	request.setContentType("application/json");
	request.setContentLength(payload.length());

	std::ostream& os = session.sendRequest(request);
	os << payload;

	HTTPResponse response;
	std::istream& rs = session.receiveResponse(response);

	std::string responseData;
	StreamCopier::copyToString(rs, responseData);
	p_strRsp = responseData;

	return response.getStatus();

}

unsigned int TF_READ_LIC(void*) {
	INT64 nSts = 0;
	ST_RESPONSE* p = (ST_RESPONSE*)malloc(sizeof(ST_RESPONSE));

	while (TRUE)
	{
		Sleep(10 * 1000);

		memset(p, 0, sizeof(ST_RESPONSE));
		if ((nSts = mil_read_license(p)) <= 0) {
			if (lv_pstRes != NULL) free(lv_pstRes);
			lv_pstRes = NULL;
		}
		else {
			if (lv_pstRes == NULL) {
				lv_pstRes = (ST_RESPONSE*)malloc(sizeof(ST_RESPONSE));
			}
			memcpy(lv_pstRes, p, sizeof(ST_RESPONSE));
		}
	}
	free(p); // Clean up allocated memory
}

void ClaHTTPServerWrapper::launch() {
	
	if (lv_pstRes == NULL) {
		lv_pstRes = (ST_RESPONSE*)malloc(sizeof(ST_RESPONSE));
		if (mil_read_license(lv_pstRes) <= 0) {
			free(lv_pstRes);
			lv_pstRes = NULL;
		}
	}

	DWORD dwTID = 0;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TF_READ_LIC, NULL, 0, &dwTID);
	run();
}

void MyRequestHandler::OnVersion(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	char szOut[MAX_PATH]; memset(szOut, 0, sizeof(szOut));
	sprintf_s(szOut, "Version : %s\nUpdate : %s", GD_ID_VERSION, GD_ID_UPDATE);
	ostr << szOut;
}
void MyRequestHandler::OnQualityProcessProc(HTTPServerRequest& request, HTTPServerResponse& response, Poco::Dynamic::Var procName, Poco::Dynamic::Var baseURL, int base64)
{
	// Get the current time point
	auto now = std::chrono::system_clock::now();

	// Convert the time point to a time_t object
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
#ifdef NDEBUG
	if (lv_pstRes == NULL || lv_pstRes->m_lExpire < now_c) {
		OnNoLicense(request, response);
		return;
	}
#endif
	std::string encodedImage;
	if (base64 == 0) {
		MyPartHandler hPart;
		Poco::Net::HTMLForm form(request, request.stream(), hPart);

		Object::Ptr jsonResponse = new Poco::JSON::Object();
		encodedImage = hPart.base64Data();
		encodedImage = replaceAll(encodedImage, "\r\n", "");
	}
	else {
		std::istream& input = request.stream();
		std::ostringstream ss;
		StreamCopier::copyStream(input, ss);
		std::string data = ss.str();

		Parser parser;
		auto result = parser.parse(data);
		Object::Ptr object = result.extract<Object::Ptr>();
		encodedImage = object->getValue<std::string>("image");
	}

	// JSON payload to send in the POST request
	Object::Ptr processParam = new Object;
	Poco::JSON::Object::Ptr outputImageParams = new Poco::JSON::Object;
	Poco::JSON::Object::Ptr crop = new Poco::JSON::Object;
	Poco::JSON::Array::Ptr size = new Poco::JSON::Array;
	// Process Parameters
	processParam->set("onlyCentralFace", false);
	processParam->set("scenario", "QualityFull");
	processParam->set("outputImageParams", outputImageParams);
	// Output Image Parameters
	outputImageParams->set("crop", crop);
	// Crop Data
	crop->set("type", 1);
	size->add(106);
	size->add(134);
	crop->set("size", size);


	Object::Ptr root = new Object;
	root->set("processParam", processParam);
	root->set("image", encodedImage);

	std::ostringstream oss;
	Stringifier::stringify(root, oss);
	std::string payload = oss.str();

	try
	{
		// Send POST request
		std::string strRsp;
		HTTPServerResponse::HTTPStatus status = sendPostRequest(baseURL, payload, strRsp);
		if (status == HTTPResponse::HTTP_OK) {

			std::string out;
			out = FaceQualityTransformJSON(strRsp);
			out = replaceAll(out, "\r\n", "");
			out = replaceAll(out, "\n", "");

			response.setStatus(status);
			response.setContentType("application/json");
			response.setContentLength(out.length());

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.send() << out.c_str();
		}
		else {
			response.setStatus(status);
			response.setContentType("application/json");

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.setContentLength(strRsp.length());
			response.send() << strRsp.c_str();
		}
	}
	catch (const Exception& ex)
	{
		response.setStatus(HTTPResponse::HTTP_CONFLICT);
		response.setContentType("application/json");

		response.set("Access-Control-Allow-Origin", "*");
		response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
		response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

		response.setContentLength(ex.displayText().length());
		response.send() << ex.displayText();
	}
}

void MyRequestHandler::OnDetectProcessProc(HTTPServerRequest& request, HTTPServerResponse& response, Poco::Dynamic::Var procName, Poco::Dynamic::Var baseURL, int base64)
{
	// Get the current time point
	auto now = std::chrono::system_clock::now();

	// Convert the time point to a time_t object
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
#ifdef NDEBUG
	if (lv_pstRes == NULL || lv_pstRes->m_lExpire < now_c) {
		OnNoLicense(request, response); 
		return;
	}
#endif
	std::string encodedImage;
	if (base64 == 0) {
		MyPartHandler hPart;
		Poco::Net::HTMLForm form(request, request.stream(), hPart);

		Object::Ptr jsonResponse = new Poco::JSON::Object();
		encodedImage = hPart.base64Data();
		encodedImage = replaceAll(encodedImage, "\r\n", "");
	}
	else {
		std::istream& input = request.stream();
		std::ostringstream ss;
		StreamCopier::copyStream(input, ss);
		std::string data = ss.str();

		Parser parser;
		auto result = parser.parse(data);
		Object::Ptr object = result.extract<Object::Ptr>();
		encodedImage = object->getValue<std::string>("image");
	}

	// JSON payload to send in the POST request
	Object::Ptr processParam = new Object;
	Poco::JSON::Object::Ptr outputImageParams = new Poco::JSON::Object;
	Poco::JSON::Object::Ptr crop = new Poco::JSON::Object;
	Poco::JSON::Array::Ptr size = new Poco::JSON::Array;
	Poco::JSON::Object::Ptr attributes = new Poco::JSON::Object;
	Poco::JSON::Array::Ptr configArray = new Poco::JSON::Array;
	// Process Parameters
	processParam->set("onlyCentralFace", false);
	processParam->set("outputImageParams", outputImageParams);
	processParam->set("attributes", attributes);
	// Output Image Parameters
	outputImageParams->set("crop", crop);
	// Crop Data
	crop->set("type", 1);
	size->add(106);
	size->add(134);
	crop->set("size", size);
	// Attributes Config
	Poco::JSON::Object::Ptr attributeConfig = new Poco::JSON::Object;
	std::vector<std::string> attributeNames = {
		"Age", "EyeRight", "EyeLeft", "Emotion", "Smile", "Glasses",
		"HeadCovering", "ForeheadCovering", "Mouth", "MedicalMask",
		"Occlusion", "StrongMakeup", "Headphones"
	};

	for (const auto& name : attributeNames) {
		Poco::JSON::Object::Ptr attribute = new Poco::JSON::Object;
		attribute->set("name", name);
		configArray->add(attribute);
	}
	attributes->set("config", configArray);

	Object::Ptr root = new Object;
	root->set("processParam", processParam);
	root->set("image", encodedImage);

	std::ostringstream oss;
	Stringifier::stringify(root, oss);
	std::string payload = oss.str();

	try
	{
		// Send POST request
		std::string strRsp;
		HTTPServerResponse::HTTPStatus status = sendPostRequest(baseURL, payload, strRsp);
		if (status == HTTPResponse::HTTP_OK) {

			std::string out;
			out = FaceDetectionTransformJSON(strRsp);
			out = replaceAll(out, "\r\n", "");
			out = replaceAll(out, "\n", "");

			response.setStatus(status);
			response.setContentType("application/json");
			response.setContentLength(out.length());

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.send() << out.c_str();
		}
		else {
			response.setStatus(status);
			response.setContentType("application/json");

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.setContentLength(strRsp.length());
			response.send() << strRsp.c_str();
		}
	}
	catch (const Exception& ex)
	{
		response.setStatus(HTTPResponse::HTTP_CONFLICT);
		response.setContentType("application/json");

		response.set("Access-Control-Allow-Origin", "*");
		response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
		response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

		response.setContentLength(ex.displayText().length());
		response.send() << ex.displayText();
	}
}

void MyRequestHandler::OnMatchProcessProc(HTTPServerRequest& request, HTTPServerResponse& response, Poco::Dynamic::Var procName, Poco::Dynamic::Var baseURL, int base64)
{
	// Get the current time point
	auto now = std::chrono::system_clock::now();

	// Convert the time point to a time_t object
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
#ifdef NDEBUG
	if (lv_pstRes == NULL || lv_pstRes->m_lExpire < now_c) {
		OnNoLicense(request, response);
		return;
	}
#endif
	std::string encodedImage1;
	std::string encodedImage2;
	if (base64 == 0) {
		MyMutiPartHandler hPart;
		Poco::Net::HTMLForm form(request, request.stream(), hPart);
		const std::vector<std::string>& encodedImages = hPart.getBase64DataList();
		encodedImage1 = encodedImages[0];
		encodedImage2 = encodedImages[1];

		encodedImage1 = replaceAll(encodedImage1, "\r\n", "");
		encodedImage2 = replaceAll(encodedImage2, "\r\n", "");

	}
	else {
		std::istream& input = request.stream();
		std::ostringstream ss;
		StreamCopier::copyStream(input, ss);
		std::string data = ss.str();

		Parser parser;
		auto result = parser.parse(data);
		Object::Ptr object = result.extract<Object::Ptr>();
		encodedImage1 = object->getValue<std::string>("image1");
		encodedImage2 = object->getValue<std::string>("image2");
	}

	// JSON payload to send in the POST request
	Object::Ptr root = new Object;
	Object::Ptr processParam = new Object;
	Poco::JSON::Object::Ptr outputImageParams = new Poco::JSON::Object;
	Poco::JSON::Object::Ptr crop = new Poco::JSON::Object;
	Poco::JSON::Array::Ptr size = new Poco::JSON::Array;
	Poco::JSON::Array::Ptr images = new Poco::JSON::Array;

	// Setting up root object
	root->set("outputImageParams", outputImageParams);
	root->set("images", images);

	// Setting up outputImageParams with crop
	outputImageParams->set("crop", crop);
	crop->set("type", 1);
	size->add(106);
	size->add(134);
	crop->set("size", size);

	// Adding images array
	Poco::JSON::Object::Ptr image1 = new Poco::JSON::Object;
	Poco::JSON::Object::Ptr image2 = new Poco::JSON::Object;

	image1->set("data", encodedImage1);
	image1->set("index", 0);
	image1->set("detectAll", true);
	image1->set("type", 3);

	image2->set("data", encodedImage2);
	image2->set("index", 1);
	image2->set("detectAll", true);
	image2->set("type", 3);

	images->add(image1);
	images->add(image2);

	std::ostringstream oss;
	Stringifier::stringify(root, oss);
	std::string payload = oss.str();

	try
	{
		// Send POST request
		std::string strRsp;
		HTTPServerResponse::HTTPStatus status = sendPostRequest(baseURL, payload, strRsp);
		if (status == HTTPResponse::HTTP_OK) {

			std::string out;
			out = FaceMatchTransformJSON(strRsp);
			out = replaceAll(out, "\r\n", "");
			out = replaceAll(out, "\n", "");

			response.setStatus(status);
			response.setContentType("application/json");
			response.setContentLength(out.length());

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.send() << out.c_str();
		}
		else {
			response.setStatus(status);
			response.setContentType("application/json");

			response.set("Access-Control-Allow-Origin", "*");
			response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
			response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

			response.setContentLength(strRsp.length());
			response.send() << strRsp.c_str();
		}
	}
	catch (const Exception& ex)
	{
		response.setStatus(HTTPResponse::HTTP_CONFLICT);
		response.setContentType("application/json");

		response.set("Access-Control-Allow-Origin", "*");
		response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
		response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

		response.setContentLength(ex.displayText().length());
		response.send() << ex.displayText();
	}
}

void MyRequestHandler::OnUnknown(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	ostr << "Not found";
}

void MyRequestHandler::OnNoLicense(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	ostr << "Please input license.";
}

void MyRequestHandler::OnStatus(HTTPServerRequest& request, HTTPServerResponse& response)
{
	response.setStatus(HTTPResponse::HTTP_OK);
	response.setContentType("text/plain");

	response.set("Access-Control-Allow-Origin", "*");
	response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	response.set("Access-Control-Allow-Headers", "Content-Type, Authorization");

	ostream& ostr = response.send();
	

	if (lv_pstRes == NULL) {
		//. no license
		ostr << "License not found";
	}
	else {
		time_t t = lv_pstRes->m_lExpire;
		struct tm timeinfo;
		localtime_s(&timeinfo, &t);
		char szTime[260]; memset(szTime, 0, sizeof(szTime));
		if (lv_pstRes->m_lExpire < 32503622400) {
			strftime(szTime, 260, "License valid : %Y-%m-%d", &timeinfo);
		}
		else {
			sprintf_s(szTime, 260, "License valid : NO LIMIT");
		}
		ostr << szTime;
	}
}