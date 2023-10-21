
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "lwip/apps/httpd.h"
#include <map>
#include <string>

#include "webInterface.h"
#include "deviceConfig.h"
#include "serviceControl.h"


// HTTPD callbacks aren't conducive to multiple instances
WebServer *_globalInstance;

WebServer::WebServer(std::shared_ptr<DeviceConfig> config, std::shared_ptr<IWifiConnection> wifiConnection)
:   _config(std::move(config)),
    _wifiConnection(std::move(wifiConnection))
{
    _globalInstance = this;
}

struct CgiContext {
    WebServer *pThis;
    bool result;
};

extern "C" {
    // Called before processing any URL with a query string
    void httpd_cgi_handler(struct fs_file *file, const char* uri, int iNumParams,
                                char **pcParam, char **pcValue, void *connectionState)
    {
        CgiContext *ctx = (CgiContext *)connectionState;
        ctx->result = ctx->pThis->HandleRequest(file, uri, iNumParams, pcParam, pcValue);
    }

    // Called by httpd when a file is opened
    void *fs_state_init(struct fs_file *file, const char *name)
    {
        // Hack
        auto ctx = new CgiContext;
        ctx->pThis = _globalInstance;
        ctx->result = false;
        return ctx;
    }

    // Called by httpd when a file is closed
    void fs_state_free(struct fs_file *file, void *state)
    {
        delete (CgiContext *)state;
    }
}


 uint16_t WebServer::HandleResponseEntry(const char *tag, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, void *connectionState)
 {
    auto ctx = (CgiContext *)connectionState;
    return ctx->pThis->HandleResponse(tag, pcInsert, iInsertLen, tagPart, nextPart, ctx->result);
 }

 uint16_t WebServer::HandleResponse(const char *tag, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, bool cgiResult)
 {
    
    if(!strcmp(tag, "result"))
    {
        if(cgiResult)
        {
            memcpy(pcInsert, "true", 4);
            return 4;
        }
        memcpy(pcInsert, "false", 5);
        return 5;
    }

    auto subscription = _responseSubscriptions.find(tag);
    if(subscription == _responseSubscriptions.end())
    {
        // Unknown tag...
        printf("Unknown SSI tag: %8s\n", tag);
        return 0;
    }

    return subscription->second(pcInsert, iInsertLen, tagPart, nextPart);
 }


void WebServer::Start()
{
    httpd_init();

    http_set_ssi_handler(HandleResponseEntry, nullptr, 0);

}

bool WebServer::HandleRequest(fs_file *file, const char *uri, int iNumParams, char **pcParam, char **pcValue)
{
    printf("CGI executed: %s\n", uri);

    std::map<std::string, std::string> params;
    for(auto a = 0; a < iNumParams; a++)
    {
        printf("    %s = %s\n", pcParam[a], pcValue[a]);
        params[pcParam[a]] = pcValue[a];
    }

    for(const auto &kvp : _requestSubscriptions)
    {
        puts(kvp.first.c_str());
        if(!strcmp(uri, kvp.first.c_str()))
        {
            return kvp.second(params);
        }
    }

    puts("Ignoring unknown CGI\n");
    return false;
}

void WebServer::AddRequestHandler(std::string url, CgiSubscribeFunc &&callback)
{
    _requestSubscriptions.insert({ url, callback});

}

void WebServer::RemoveRequestHandler(std::string url)
{
    _requestSubscriptions.erase(url);
}


void WebServer::AddResponseHandler(std::string tag, SsiSubscribeFunc &&callback)
{
    _responseSubscriptions.insert({ tag, callback});

}

void WebServer::RemoveResponseHandler(std::string tag)
{
    _responseSubscriptions.erase(tag);
}
