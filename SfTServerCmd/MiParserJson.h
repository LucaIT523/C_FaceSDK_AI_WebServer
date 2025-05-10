#pragma once

#include <string>

std::string process_json_full(std::string p_strJson);
std::string process_json_full_process(std::string p_strJson);
std::string process_json_credit_card(std::string p_strJson);
std::string process_json_mrz_barcode(std::string p_strJson);
std::string FaceDetectionTransformJSON(const std::string& inputJSON);
std::string FaceMatchTransformJSON(const std::string& inputJSON);
std::string FaceQualityTransformJSON(const std::string& inputJSON);

