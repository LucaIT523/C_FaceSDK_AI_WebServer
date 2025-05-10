#include "MiParserJson.h"

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Exception.h>

using namespace Poco;
using namespace Poco::JSON;

typedef struct tagFieldName
{
	const char* m_szOrgName;
	const char* m_szName;
}ST_FIELD_NAME;


ST_FIELD_NAME lv_fieldFullProcess[] = {
//	Original Name					Show Name
	{"DocumentName",				"Document Name"},
	{"Surname And Given Names",		"Full Name"},
	{"Bank Card Valid Thru",		"Bank Card Validity"},
	{"MRZ Strings",					""},	//this field will be hide
	{NULL, NULL}
};

ST_FIELD_NAME lv_fieldMRZ[] = {
//	Original Name					Show Name
	{"Date of Birth",				"Date of Birth"},
	{"Date of Expiry",				"Date of Expiry"},
	{"Document Class Code",			"Document Class Code"},
	{"Document Number",				"Document Number"},
	{"Surname And Given Names",		"Full Name"},
	{"Issuing State Code",			"Issuing State Code"},
	{"Issuing State Name",			"Issuing State Name"},
	{"Personal Number",				"Personal Number"},
	{"Sex",							"Sex"},
	{"Surname",						"Surname"},
	{"MRZ Type",					"MRZ Type"},
	{"MRZ Strings",					"MRZ Strings"},
	{NULL, NULL}
};

ST_FIELD_NAME lv_fieldImage[] = {
//	Original Name					Show Name
	{"Document front side",			"Document"},
	{"Portrait",					"Portrait111"},
	{NULL, NULL}
};


const char* _getFieldName(const char* p_szName, ST_FIELD_NAME* p_pFieldList) {
	for (int i = 0; p_pFieldList[i].m_szOrgName != NULL; i++) {
		if (strcmp(p_szName, p_pFieldList[i].m_szOrgName) == 0) {
			return p_pFieldList[i].m_szName;
		}
	}
	return NULL;
}

const char* _getImageFieldName(const char* p_szName) {
	for (int i = 0; lv_fieldImage[i].m_szOrgName != NULL; i++) {
		if (strcmp(p_szName, lv_fieldImage[i].m_szOrgName) == 0) {
			return lv_fieldImage[i].m_szName;
		}
	}
	return p_szName;
}

std::string _convertFTtoCM(std::string p_strData) {
	int w_Pos = p_strData.find("cm");
	if (w_Pos <= 0) {
		int ft = 0, in = 0;
		sscanf_s(p_strData.c_str(), "%d ft %d in", &ft, &in);
		if (ft > 10) ft = 0;
		if (in > 10 || in < 0) in = 0;

		int cm = (((ft * 12) + in) * 2.54);
		char szRet[10]; memset(szRet, 0, sizeof(szRet));
		sprintf_s(szRet, 10, "%dcm", cm);
		return szRet;
	}
	else {
		return p_strData;
	}
}

Object::Ptr _getObject(Object::Ptr& p_pObj, const char* p_szKey) {
	Object::Ptr ret = NULL;
	try {
		ret = p_pObj->getObject(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return NULL;
	}
}

Array::Ptr _getArray(Object::Ptr& p_pObj, const char* p_szKey) {
	Array::Ptr ret = NULL;
	try {
		ret = p_pObj->getArray(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return NULL;
	}
}

std::string _getString(Object::Ptr& p_pObj, const char* p_szKey) {
	std::string ret = "";
	try {
		ret = p_pObj->getValue<std::string>(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return "";
	}
}

int _getNumber(Object::Ptr& p_pObj, const char* p_szKey) {
	int ret = 0;
	try {
		ret = p_pObj->getValue<int>(p_szKey);
		return ret;
	}
	catch (Exception e) {
		return ret;
	}
}

void _addJsonTextItem(
	Object::Ptr& p_jsonRet,
	const char* p_szName, const char* p_szValue, const char* p_szLicName,
	ST_FIELD_NAME* p_pFieldNames, bool p_bOnlyInclude)
{
	const char* pszName = _getFieldName(p_szName, p_pFieldNames);
	if (pszName == NULL && p_bOnlyInclude) return;
	if (pszName != NULL && strcmp(pszName, "") == 0) return;
	std::string strNewName;
	if (pszName == NULL) {
		strNewName = p_szName;
	}
	else {
		strNewName = pszName;
	}

	if (p_szLicName[0] != 0) {
		strNewName = strNewName + "-" + p_szLicName;
	}
	std::string strValue = p_szValue;
	p_jsonRet->set(strNewName, strValue);
}

void extract_text_block(Object::Ptr& p_jsonMain, Object::Ptr& p_jsonRet, ST_FIELD_NAME* p_pFieldNames, bool p_bOnlyInclude)
{
	Object::Ptr containerList = p_jsonMain->getObject("ContainerList");
	Array::Ptr listArray = containerList->getArray("List");
	int nTotalSize = listArray->size();

	for (int i = 0; i < nTotalSize; i++) {
		Object::Ptr obj = listArray->getObject(i);

		//.	for text result
		Object::Ptr objOneCandidate = _getObject(obj, "OneCandidate");
		Object::Ptr objText = _getObject(obj, "Text");

		if (objOneCandidate != nullptr) {
			std::string str_name;
			std::string str_value;

			std::string value = _getString(objOneCandidate, "DocumentName");
			if (value != "") {
				_addJsonTextItem(p_jsonRet, "DocumentName", value.c_str(), "", p_pFieldNames, p_bOnlyInclude);
			}
		}
		else if (objText != nullptr) {
			Array::Ptr fieldList = _getArray(objText, "fieldList");
			if (fieldList == nullptr)
				continue;
			for (int j = 0; j < fieldList->size(); j++) {
				Object::Ptr field = fieldList->getObject(j);
				std::string fieldName = _getString(field, "fieldName");
				std::string lcidName = _getString(field, "lcidName");
				std::string value = _getString(field, "value");

				std::string value_org = "";

				Array::Ptr val_list = _getArray(field, "valueList");
				if (val_list != nullptr) {
					Object::Ptr vals = val_list->getObject(0);
					if (vals != nullptr) {
						value_org = _getString(vals, "originalValue");
					}
				}

				std::string str_name;
				if (lcidName == "")
					str_name = fieldName;
				else
					str_name = fieldName + "-" + lcidName;

				std::string str_value;
				if (fieldName == "") continue;
				if (value_org == "")
					str_value = value;
				else
					str_value = value_org;

				_addJsonTextItem(p_jsonRet, fieldName.c_str(), str_value.c_str(), lcidName.c_str(), p_pFieldNames, p_bOnlyInclude);
			}
		}
	}
}

void extract_image_block(Object::Ptr& p_jsonMain, Object::Ptr& p_jsonRet)
{
	Object::Ptr containerList = p_jsonMain->getObject("ContainerList");
	Array::Ptr listArray = containerList->getArray("List");
	int nTotalSize = listArray->size();

	for (int i = 0; i < nTotalSize; i++) {
		Object::Ptr obj = listArray->getObject(i);

		//.	for text result
		Object::Ptr objImages = _getObject(obj, "Images");

		if (objImages != nullptr) {
			Array::Ptr fieldList = _getArray(objImages, "fieldList");
			if (fieldList == nullptr)
				continue;

			Object::Ptr imagesObject = new Object();

			for (int j = 0; j < fieldList->size(); j++) {

				Object::Ptr field_image = new Object();

				Object::Ptr field = fieldList->getObject(j);
				std::string fieldName = _getString(field, "fieldName");
				Array::Ptr valueList = _getArray(field, "valueList");
				Object::Ptr valueListItem = valueList->getObject(0);
				Object::Ptr fieldRect = valueListItem->getObject("fieldRect");
				std::string strImage;

				if (fieldRect != nullptr) {
					int Portrait_x1 = _getNumber(fieldRect, "left");
					int Portrait_x2 = _getNumber(fieldRect, "right");
					int Portrait_y1 = _getNumber(fieldRect, "top");
					int Portrait_y2 = _getNumber(fieldRect, "bottom");

					Object::Ptr portraitPositionObject = new Object();
					portraitPositionObject->set("x1", Portrait_x1);
					portraitPositionObject->set("x2", Portrait_x2);
					portraitPositionObject->set("y1", Portrait_y1);
					portraitPositionObject->set("y2", Portrait_y2);
					field_image->set("Position", portraitPositionObject);
				}
				strImage = _getString(valueListItem, "value");
				field_image->set("image", strImage);

				fieldName = _getImageFieldName(fieldName.c_str());
				p_jsonRet->set(fieldName, field_image);
			}
		}
	}
}

std::string process_json_full_process(std::string p_strJson)
{
	Parser parser;
	auto result = parser.parse(p_strJson);
	Object::Ptr json_obj = result.extract<Object::Ptr>();
	Object::Ptr json_ret = new Object();
	Object::Ptr json_mrz = new Object();
	Object::Ptr json_img = new Object();

	extract_text_block(json_obj, json_ret, lv_fieldFullProcess, false);
	extract_text_block(json_obj, json_mrz, lv_fieldMRZ, true);
	extract_image_block(json_obj, json_img);

	json_ret->set("Images", json_img);

	std::string strMrz = _getString(json_mrz, "MRZ Strings");

	if (strMrz != "")
		json_ret->set("MRZ", json_mrz);

	json_ret->set("Status", "Ok");
	//. 
	std::stringstream out_temp;
 	json_ret->stringify(out_temp, 2);
 	std::string ret = out_temp.str();
	return ret;
}

std::string process_json_credit_card(std::string p_strJson)
{
	Parser parser;
	auto result = parser.parse(p_strJson);
	Object::Ptr json_obj = result.extract<Object::Ptr>();
	Object::Ptr json_ret = new Object();
	Object::Ptr json_img = new Object();

	extract_text_block(json_obj, json_ret, lv_fieldFullProcess, false);
	extract_image_block(json_obj, json_img);

	json_ret->set("Images", json_img);

	json_ret->set("Status", "Ok");
	//. 
	std::stringstream out_temp;
	json_ret->stringify(out_temp, 2);
	std::string ret = out_temp.str();
	return ret;
}

std::string process_json_mrz_barcode(std::string p_strJson)
{
	Parser parser;
	auto result = parser.parse(p_strJson);
	Object::Ptr json_obj = result.extract<Object::Ptr>();
	Object::Ptr json_ret = new Object();
	Object::Ptr json_mrz = new Object();
	Object::Ptr json_img = new Object();

	extract_text_block(json_obj, json_mrz, lv_fieldMRZ, true);
	extract_image_block(json_obj, json_img);

	json_ret->set("Images", json_img);
	std::string strMrz = _getString(json_mrz, "MRZ Strings");

	if (strMrz != "")
		json_ret->set("MRZ", json_mrz);
	json_ret->set("Status", "Ok");
	//. 
	std::stringstream out_temp;
	json_ret->stringify(out_temp, 2);
	std::string ret = out_temp.str();
	return ret;
}

std::string FaceDetectionTransformJSON(const std::string& inputJSON) {

	try {

		Poco::JSON::Parser parser;
		Poco::Dynamic::Var result = parser.parse(inputJSON);
		Poco::JSON::Object::Ptr jsonObject = result.extract<Poco::JSON::Object::Ptr>();

		Poco::JSON::Object newJson;
		Poco::JSON::Object::Ptr detections = new Poco::JSON::Object;
		// Access the nested fields for "crop", "landmarks", "roi", and "rotationAngle"
		Poco::JSON::Object::Ptr results = jsonObject->getObject("results");
		Poco::JSON::Array::Ptr originalDetections = results->getArray("detections");
		for (size_t i = 0; i < originalDetections->size(); ++i) {

			Poco::JSON::Object::Ptr detection = originalDetections->getObject(i);
			Poco::JSON::Object::Ptr transformedJSON = new Poco::JSON::Object();
			// Copy "crop", "landmarks", "roi", and "rotationAngle" directly
			transformedJSON->set("face", detection->get("crop"));
			transformedJSON->set("landmarks", detection->get("landmarks"));
			transformedJSON->set("position", detection->get("roi"));
			transformedJSON->set("rotationAngle", detection->get("rotationAngle"));
			// Transform "attributes"
			Poco::JSON::Object::Ptr attributes = detection->getObject("attributes");
			Poco::JSON::Array::Ptr details = attributes->getArray("details");
			Poco::JSON::Object::Ptr transformedAttributes = new Poco::JSON::Object();

			// Filter required attributes
			for (size_t i = 0; i < details->size(); ++i) {
				Poco::JSON::Object::Ptr detail = details->getObject(i);
				std::string name = detail->getValue<std::string>("name");

				if (name == "Age" || name == "EyeLeft" || name == "EyeRight" ||
					name == "Emotion" || name == "Smile" || name == "Glasses" ||
					name == "HeadCovering" || name == "ForeheadCovering" || name == "MedicalMask" ||
					name == "Occlusion" || name == "StrongMakeup" || name == "Headphones") {
					transformedAttributes->set(name, detail->get("value"));
				}
			}
			transformedJSON->set("attributes", transformedAttributes);

			detections->set(std::to_string(i), transformedJSON);
		}
		newJson.set("detections", detections);
		// Convert the final object to JSON string
		std::ostringstream oss;
		newJson.stringify(oss, 2);  // Indent by 2 for readability
		return oss.str();
	}
	catch (const Poco::Exception& ex) {
		std::string		w_sErr = "Please input the correct face image.";
		return w_sErr;
	}

}
std::string FaceQualityTransformJSON(const std::string& inputJSON) {

	try {
		// Parse the JSON string
		Poco::JSON::Parser parser;
		Poco::Dynamic::Var result = parser.parse(inputJSON);
		Poco::JSON::Object::Ptr jsonObject = result.extract<Poco::JSON::Object::Ptr>();

		// Extract the "results" object
		Poco::JSON::Object::Ptr resultsObject = jsonObject->getObject("results");
		// Extract "detections" array
		Poco::JSON::Array::Ptr detectionsArray = resultsObject->getArray("detections");
		// Assuming we want to transform the first detection
		Poco::JSON::Object::Ptr detectionObject = detectionsArray->getObject(0);
		// Create the new JSON structure
		Poco::JSON::Object::Ptr newJsonObject = new Poco::JSON::Object;

		// Extracting the required fields
		newJsonObject->set("face", detectionObject->get("crop"));
		newJsonObject->set("landmarks", detectionObject->get("landmarks"));

		// Extract the quality details
		Poco::JSON::Object::Ptr qualityObject = detectionObject->getObject("quality");
		Poco::JSON::Array::Ptr details = qualityObject->getArray("details");

		// Create a new quality object
		Poco::JSON::Object::Ptr newQualityObject = new Poco::JSON::Object();

		for (size_t i = 0; i < details->size(); ++i) {
			Poco::JSON::Object::Ptr detail = details->getObject(i);
			std::string name = detail->getValue<std::string>("name");
			double value = detail->getValue<double>("value");
			newQualityObject->set(name, value);
		}
		// Set the new quality object into the new JSON object
		newJsonObject->set("quality", newQualityObject);

		// Convert the final object to JSON string
		std::ostringstream oss;
		newJsonObject->stringify(oss, 2);  // Indent by 2 for readability
		return oss.str();
	}
	catch (const Poco::Exception& ex) {
		std::string		w_sErr = "Please input the correct face image.";
		return w_sErr;
	}
}
std::string FaceMatchTransformJSON(const std::string& inputJSON)
{
	try {
		// Parse the original JSON
		Poco::JSON::Parser parser;
		Poco::Dynamic::Var parsedJson = parser.parse(inputJSON);
		Poco::JSON::Object::Ptr jsonObject = parsedJson.extract<Poco::JSON::Object::Ptr>();

		// Create new JSON structure
		Poco::JSON::Object newJson;
		Poco::JSON::Array::Ptr detections = new Poco::JSON::Array;
		Poco::JSON::Array::Ptr matches = new Poco::JSON::Array;

		// Process detections
		Poco::JSON::Array::Ptr originalDetections = jsonObject->getArray("detections");
		for (size_t i = 0; i < originalDetections->size(); ++i) {
			Poco::JSON::Object::Ptr detection = originalDetections->getObject(i);
			Poco::JSON::Array::Ptr faces = detection->getArray("faces");
			int imageIndex = detection->getValue<int>("imageIndex");

			for (size_t j = 0; j < faces->size(); ++j) {
				Poco::JSON::Object::Ptr face = faces->getObject(j);
				Poco::JSON::Object::Ptr newDetection = new Poco::JSON::Object;

				newDetection->set("face", face->get("crop"));
				newDetection->set("landmarks", face->getArray("landmarks"));
				newDetection->set("roi", face->getArray("roi"));
				newDetection->set("rotationAngle", face->get("rotationAngle"));
				if (imageIndex == 0) {
					newDetection->set("firstFaceIndex", face->get("faceIndex"));
				}
				else {
					newDetection->set("secondFaceIndex", face->get("faceIndex"));
				}

				detections->add(newDetection);
			}
		}

		// Process match results
		Poco::JSON::Array::Ptr results = jsonObject->getArray("results");
		for (size_t k = 0; k < results->size(); ++k) {
			Poco::JSON::Object::Ptr result = results->getObject(k);
			Poco::JSON::Object::Ptr newMatch = new Poco::JSON::Object;

			newMatch->set("firstFaceIndex", result->get("firstFaceIndex"));
			newMatch->set("secondFaceIndex", result->get("secondFaceIndex"));
			newMatch->set("similarity", result->get("similarity"));

			matches->add(newMatch);
		}

		// Add arrays to new JSON object
		newJson.set("detections", detections);
		newJson.set("match", matches);

		// Output the new JSON
		std::ostringstream oss;
		newJson.stringify(oss, 2);  // Pretty print with 2 spaces
		return oss.str();
	}
	catch (const Poco::Exception& ex) {
		std::string		w_sErr = "Please input the correct face image.";
		return w_sErr;
	}
}
