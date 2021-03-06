/**
 * @author : xiaozhuai
 * @date   : 17/1/3
 */

#ifndef CXXURL_REQUEST_H
#define CXXURL_REQUEST_H

#include <curl/curl.h>
#include <string>
#include <iostream>
#include <map>
#include <cstdlib>
#include "Version.h"
#include "Form.h"
#include "SimpleForm.h"
#include "MultipartForm.h"
#include "RawForm.h"
#include "Header.h"
#include "StringUtils.h"


namespace CXXUrl {

using namespace std;

#define DEFINE_PROP_GETTER_SETTER(type, prop) \
    protected: \
        type m_##prop; \
    public: \
        void set##prop(type val){ m_##prop = val; } \
        type get##prop(){ return m_##prop; }

#define DEFINE_MAP_PUSHER_GETTER(type1, type2, prop) \
    protected: \
        map<type1, type2> m_##prop; \
    public: \
        void set##prop(type1 key, type2 val){ m_##prop[key] = val; } \
        type2 get##prop(type1 key){ return m_##prop[key]; }

#define DEFINE_METHOD(func_name, method) CURLcode func_name(){ return exec(method); }

#define SET_CURL_OPT(opt,value) curl_easy_setopt(m_Curl,(CURLoption)(opt),(value))

class Request {
    public:
        Request() :
                m_Curl(nullptr),
                m_FollowLocation(true),
                m_ContentOutput(nullptr),
                m_HeaderOutput(nullptr),
                m_MaxRedirs(-1),
                m_Form(nullptr),
                m_Header(nullptr),
                m_Timeout(0L),
                m_VerifySSL(false),
                m_NoBody(false),
                m_Verbose(false) {
            m_UserAgent = "CXXUrl/" + CXX_URL_VERSION + " " + curl_version();
        }
        ~Request() = default;

    protected:
        CURL* m_Curl;

    protected:
        static size_t writeContent(char* buffer, size_t size, size_t count, void* stream);
        static size_t writeHeader(char* buffer, size_t size, size_t count, void* stream);

        DEFINE_PROP_GETTER_SETTER(string, Url)
        DEFINE_PROP_GETTER_SETTER(bool, FollowLocation)
        DEFINE_PROP_GETTER_SETTER(int, MaxRedirs)
        DEFINE_PROP_GETTER_SETTER(Form*, Form)
        DEFINE_PROP_GETTER_SETTER(string, UserAgent)
        DEFINE_PROP_GETTER_SETTER(string, Referer)
        DEFINE_PROP_GETTER_SETTER(string, ContentType)
        DEFINE_PROP_GETTER_SETTER(Header*, Header)
        DEFINE_PROP_GETTER_SETTER(long, Timeout)
        DEFINE_PROP_GETTER_SETTER(string, Proxy)
        DEFINE_PROP_GETTER_SETTER(string, CookieImportFile)
        DEFINE_PROP_GETTER_SETTER(string, CookieExportFile)
        DEFINE_PROP_GETTER_SETTER(bool, VerifySSL)
        DEFINE_PROP_GETTER_SETTER(string, Cacert)
        DEFINE_PROP_GETTER_SETTER(bool, NoBody)
        DEFINE_PROP_GETTER_SETTER(bool, Verbose)
        DEFINE_PROP_GETTER_SETTER(ostream*, ContentOutput)
        DEFINE_PROP_GETTER_SETTER(ostream*, HeaderOutput)
        DEFINE_MAP_PUSHER_GETTER(int, long, CurlOptionLong)
        DEFINE_MAP_PUSHER_GETTER(int, string, CurlOptionString)

    public:
        DEFINE_METHOD(get,      "GET")
        DEFINE_METHOD(post,     "POST")
        DEFINE_METHOD(put,      "PUT")
        DEFINE_METHOD(head,     "HEAD")
        DEFINE_METHOD(options,  "OPTIONS")
        DEFINE_METHOD(del,      "DELETE")
        DEFINE_METHOD(connect,  "CONNECT")

        CURLcode exec(string method="") {
            m_Curl = curl_easy_init();

            SET_CURL_OPT(CURLOPT_VERBOSE, m_Verbose);

            SET_CURL_OPT(CURLOPT_URL, m_Url.c_str());

            method = StringUtils::toupper(method);
            if(m_NoBody || (method == "HEAD"))
                SET_CURL_OPT(CURLOPT_NOBODY, 1);
            if(method!="HEAD")
                SET_CURL_OPT(CURLOPT_CUSTOMREQUEST, method.c_str());

            if (m_Form != nullptr) {
                switch (m_Form->m_Type){
                    case Form::SIMPLE: {
                        auto *simpleForm = (SimpleForm *) m_Form;
                        SET_CURL_OPT(CURLOPT_POSTFIELDS, simpleForm->getData());
                        SET_CURL_OPT(CURLOPT_POSTFIELDSIZE, simpleForm->length());
                        break;
                    }
                    case Form::MULTIPART: {
                        auto *multipartForm = (MultipartForm *) m_Form;
                        SET_CURL_OPT(CURLOPT_HTTPPOST, multipartForm->getData());
                        break;
                    }
                    case Form::RAW: {
                        auto *rawForm = (RawForm *) m_Form;
                        SET_CURL_OPT(CURLOPT_POSTFIELDS, rawForm->getData());
                        SET_CURL_OPT(CURLOPT_POSTFIELDSIZE, rawForm->length());
                        break;
                    }
                    default:
                        cerr << "form type unknown" << endl << flush;
                        break;
                }
            }



            SET_CURL_OPT(CURLOPT_FOLLOWLOCATION, m_FollowLocation);
            SET_CURL_OPT(CURLOPT_USERAGENT, m_UserAgent.c_str());

            if(!m_Referer.empty()){
                SET_CURL_OPT(CURLOPT_REFERER, m_Referer.c_str());
            }

            bool need_reset_header = false;
            if(!m_ContentType.empty()){
                if(m_Header==nullptr){
                    Header tmpHeader;
                    m_Header = &tmpHeader;
                    need_reset_header = true;
                }
                m_Header->add("Content-Type", m_ContentType);
            }

            if(m_Header!=nullptr){
                SET_CURL_OPT(CURLOPT_HTTPHEADER, m_Header->getHeaders());
                if(need_reset_header) m_Header = nullptr;
            }

            if(m_Timeout>0L){
                SET_CURL_OPT(CURLOPT_TIMEOUT_MS, m_Timeout);
            }


            if(!m_Proxy.empty()){
                SET_CURL_OPT(CURLOPT_PROXY, m_Proxy.c_str());
            }else{
                char* envProxy = getenv("http_proxy");
                if(envProxy==nullptr) envProxy = getenv("HTTP_PROXY");
                if(envProxy!=nullptr) SET_CURL_OPT(CURLOPT_PROXY, envProxy);
            }

            if(!m_CookieImportFile.empty()){
                SET_CURL_OPT(CURLOPT_COOKIEFILE, m_CookieImportFile.c_str());
            }
            if(!m_CookieExportFile.empty()){
                SET_CURL_OPT(CURLOPT_COOKIEJAR, m_CookieExportFile.c_str());
            }

            if(m_VerifySSL && !m_Cacert.empty()){
                SET_CURL_OPT(CURLOPT_SSL_VERIFYPEER, 1);
                SET_CURL_OPT(CURLOPT_SSL_VERIFYHOST, 1);
                SET_CURL_OPT(CURLOPT_CAINFO, m_Cacert.c_str());
            }else{
                SET_CURL_OPT(CURLOPT_SSL_VERIFYPEER, 0);
                SET_CURL_OPT(CURLOPT_SSL_VERIFYHOST, 0);
            }

            if (m_MaxRedirs != -1)
                SET_CURL_OPT(CURLOPT_MAXREDIRS, m_MaxRedirs);

            SET_CURL_OPT(CURLOPT_WRITEFUNCTION, writeContent);
            SET_CURL_OPT(CURLOPT_WRITEDATA, m_ContentOutput);
            SET_CURL_OPT(CURLOPT_HEADERFUNCTION, writeHeader);
            SET_CURL_OPT(CURLOPT_HEADERDATA, m_HeaderOutput);

            for (auto i : m_CurlOptionLong) {
                SET_CURL_OPT(i.first, i.second);
            }

            for (auto i : m_CurlOptionString) {
                SET_CURL_OPT(i.first, i.second.c_str());
            }


            CURLcode res;
            res = curl_easy_perform(m_Curl);
            curl_easy_cleanup(m_Curl);
            return res;
        }
};

size_t Request::writeContent(char *buffer, size_t size, size_t count, void *stream) {
    if(stream!= nullptr)
        ((ostream *) stream)->write(buffer, size * count);
    return size * count;
}

size_t Request::writeHeader(char *buffer, size_t size, size_t count, void *stream) {
    if(stream!= nullptr)
        ((ostream *) stream)->write(buffer, size * count);
    return size * count;
}

}

#endif //CXXURL_REQUEST_H
