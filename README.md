## Building

To build the following libraries are required:

**Available by devkitPro:**

libctru<br>
citro3d<br>
libpng<br>
libcurl<br>
flite<br>
jansson<br>

**Build from source:**

citro2d    - https://github.com/piepie62/citro2d/tree/layoutText <br>
libsndfile - https://github.com/libsndfile/libsndfile

#### **TLS**

A CA certificate in PEM format is required by default: https://curl.se/docs/caextract.html <br>
To include, copy ```cacert.pem``` to ```rsrc\romfs```<br>
<br>
TLS can be disabled by defining: ```CURL_NO_CERT```


#### **API-Key**

It is possible to include you API-Key in the ROMFS:<br>
Provide your API-Key in a JSON format: `api_key.json`<br>
```json
{
    "api_key": "sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
}
```

Copy to: ```api_key.json``` to ```rsrc\romfs```