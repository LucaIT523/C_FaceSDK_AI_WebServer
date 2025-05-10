# 

<div align="center">
   <h1>C++_FaceSDK_AI_WebServer (Windows, Linux, Docker)</h1>
</div>



### **1. Core Architecture**

This code implements a **License-Managed Facial Recognition HTTP Server** using Poco C++ Libraries. It acts as middleware handling license validation and request routing for three core services:

1. **Face Quality Assessment** (`OnQualityProcessProc`)
2. **Face Detection & Attributes Analysis** (`OnDetectProcessProc`)
3. **Face Matching** (`OnMatchProcessProc`)

------

### **2. Key Components**

#### **2.1 License Management**

```

unsigned int TF_READ_LIC(void*) {
    mil_read_license(p); // License check every 10s
    memcpy(lv_pstRes, p, ...); // Update global license status
}

// Runtime validation in handlers
#ifdef NDEBUG
if (lv_pstRes->m_lExpire < now_c) // Expiration check
    OnNoLicense(...);
#endif
```

#### **2.2 HTTP Server Infrastructure**

| Component              | Functionality                         |
| ---------------------- | ------------------------------------- |
| `ClaHTTPServerWrapper` | Main server lifecycle management      |
| `MyRequestHandler`     | Endpoint routing & request processing |
| `sendPostRequest`      | Backend service proxy implementation  |

------

### **3. Service Workflows**

#### **3.1 Face Quality Processing**

```
1. Base64 Image Decoding → 
2. JSON Payload Construction (106x134 crop) → 
3. Backend Service Proxy → 
4. Response Transformation (FaceQualityTransformJSON) → 
5. CORS-enabled Response
```

#### **3.2 Face Detection Pipeline**

```
"Age", "EyeRight", "EyeLeft", "Emotion", "Smile", 
"Glasses", "HeadCovering", "MedicalMask", "Occlusion"
```

#### **3.3 Face Matching Logic**

```
// Dual image handling:
image1->set("data", encodedImage1); // Index 0
image2->set("data", encodedImage2); // Index 1

```

------

### **4. Technical Features**

#### **4.1 Security Implementation**

- **License Enforcement**:

  - Periodic license check (10s interval)
  - Expiration timestamp validation (UNIX time)
  - Production build restrictions (`#ifdef NDEBUG`)

- **CORS Policy**:

  ```
  cppCopyresponse.set("Access-Control-Allow-Origin", "*");
  response.set("Access-Control-Allow-Methods", "GET, POST...");
  ```

#### **4.2 Image Handling**

- **Multi-part Form Support**:

  ```
  cppCopyMyPartHandler hPart;
  Poco::Net::HTMLForm form(...); // For file uploads
  ```

- **Base64 Processing**:

  ```
  encodedImage = replaceAll(..., "\r\n", ""); // Sanitization
  ```

#### **4.3 Error Handling**

| Scenario              | Response                          |
| --------------------- | --------------------------------- |
| License Invalid       | HTTP 200 + "Please input license" |
| Backend Service Error | HTTP 409 Conflict                 |
| Malformed Request     | HTTP 400 Bad Request              |

------

### **5. FaceSDK API Details**

POST http://127.0.0.1:8083/api/face_detect Face Detection, Face Attributes API

POST http://127.0.0.1:8083/api/face_detect_base64 Face Detection, Face Attributes API

POST http://127.0.0.1:8083/api/face_match Face Matching API

POST http://127.0.0.1:8083/api/face_match_base64 Face Matching API

Request
URL: http://127.0.0.1:8083/api/face_detect

Method: POST

Form Data:

image: The image file (PNG, JPG, etc.) to be analyzed. This should be provided as a file upload.



Request

URL: http://127.0.0.1:8083/api/face_detect_base64
Method: POST
Raw Data:
JSON Format: { "image": "--base64 image data here--" }
Response
The API returns a JSON object with the recognized details from the input Face image. Here is an example response:



<div align="center">
   <img src=https://github.com/LucaIT523/C_FaceSDK_AI_WebServer/blob/main/images/1.png>
</div>


<div align="center">
   <img src=https://github.com/LucaIT523/C_FaceSDK_AI_WebServer/blob/main/images/2.png>
</div>






### **Contact Us**

For any inquiries or questions, please contact us.

telegram : @topdev1012

email :  skymorning523@gmail.com

Teams :  https://teams.live.com/l/invite/FEA2FDDFSy11sfuegI