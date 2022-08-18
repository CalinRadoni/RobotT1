#include <HTTPClient.h>
#include <Update.h>

#include <esp_ota_ops.h>

#include "UpdateFromWeb.h"

const char *cert_raw_githubusercontent_com = "-----BEGIN CERTIFICATE-----\n" \
    "MIIEvjCCA6agAwIBAgIQBtjZBNVYQ0b2ii+nVCJ+xDANBgkqhkiG9w0BAQsFADBh\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
    "QTAeFw0yMTA0MTQwMDAwMDBaFw0zMTA0MTMyMzU5NTlaME8xCzAJBgNVBAYTAlVT\n" \
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxKTAnBgNVBAMTIERpZ2lDZXJ0IFRMUyBS\n" \
    "U0EgU0hBMjU2IDIwMjAgQ0ExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n" \
    "AQEAwUuzZUdwvN1PWNvsnO3DZuUfMRNUrUpmRh8sCuxkB+Uu3Ny5CiDt3+PE0J6a\n" \
    "qXodgojlEVbbHp9YwlHnLDQNLtKS4VbL8Xlfs7uHyiUDe5pSQWYQYE9XE0nw6Ddn\n" \
    "g9/n00tnTCJRpt8OmRDtV1F0JuJ9x8piLhMbfyOIJVNvwTRYAIuE//i+p1hJInuW\n" \
    "raKImxW8oHzf6VGo1bDtN+I2tIJLYrVJmuzHZ9bjPvXj1hJeRPG/cUJ9WIQDgLGB\n" \
    "Afr5yjK7tI4nhyfFK3TUqNaX3sNk+crOU6JWvHgXjkkDKa77SU+kFbnO8lwZV21r\n" \
    "eacroicgE7XQPUDTITAHk+qZ9QIDAQABo4IBgjCCAX4wEgYDVR0TAQH/BAgwBgEB\n" \
    "/wIBADAdBgNVHQ4EFgQUt2ui6qiqhIx56rTaD5iyxZV2ufQwHwYDVR0jBBgwFoAU\n" \
    "A95QNVbRTLtm8KPiGxvDl7I90VUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQG\n" \
    "CCsGAQUFBwMBBggrBgEFBQcDAjB2BggrBgEFBQcBAQRqMGgwJAYIKwYBBQUHMAGG\n" \
    "GGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBABggrBgEFBQcwAoY0aHR0cDovL2Nh\n" \
    "Y2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9vdENBLmNydDBCBgNV\n" \
    "HR8EOzA5MDegNaAzhjFodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vRGlnaUNlcnRH\n" \
    "bG9iYWxSb290Q0EuY3JsMD0GA1UdIAQ2MDQwCwYJYIZIAYb9bAIBMAcGBWeBDAEB\n" \
    "MAgGBmeBDAECATAIBgZngQwBAgIwCAYGZ4EMAQIDMA0GCSqGSIb3DQEBCwUAA4IB\n" \
    "AQCAMs5eC91uWg0Kr+HWhMvAjvqFcO3aXbMM9yt1QP6FCvrzMXi3cEsaiVi6gL3z\n" \
    "ax3pfs8LulicWdSQ0/1s/dCYbbdxglvPbQtaCdB73sRD2Cqk3p5BJl+7j5nL3a7h\n" \
    "qG+fh/50tx8bIKuxT8b1Z11dmzzp/2n3YWzW2fP9NsarA4h20ksudYbj/NhVfSbC\n" \
    "EXffPgK2fPOre3qGNm+499iTcc+G33Mw+nur7SpZyEKEOxEXGlLzyQ4UfaJbcme6\n" \
    "ce1XR2bFuAJKZTRei9AqPCCcUZlM51Ke92sRKw2Sfh3oius2FkOH6ipjv3U/697E\n" \
    "A7sKPPcw7+uvTPyLNhBzPvOk\n" \
    "-----END CERTIFICATE-----";

UpdateFromWeb::UpdateFromWeb(void)
{
    //
}

UpdateFromWeb::~UpdateFromWeb()
{
    //
}

bool UpdateFromWeb::SetCertificateForHost(String host)
{
    if (host == "raw.githubusercontent.com") {
        strncpy(certificate, cert_raw_githubusercontent_com, MaxCertLen);
        return true;
    }

    return false;
}

bool UpdateFromWeb::SetCertificate(String link)
{
    int index = link.indexOf(':');
    if (index < 0) {
        log_e("Invalid link");
        return false;
    }

    String str = link.substring(0, index);
    if (str != "https") {
        log_e("Invalid protocol");
        return false;
    }

    str = link.substring(index + 3); // remove 'protocol://'
    index = str.indexOf('/');
    if (index < 0) {
        log_e("Invalid link");
        return false;
    }

    str = str.substring(0, index); // str is now the host part of the URL
    return SetCertificateForHost(str);
}

bool UpdateFromWeb::UpdateFromLink(String link)
{
    int index = link.indexOf(':');
    if (index < 0) {
        log_e("Invalid link");
        return false;
    }

    bool httpsMode = false;
    String protocol = link.substring(0, index);
    if (protocol == "https") {
        if (!SetCertificate(link)) {
            log_e("Failed to set certificate");
            return false;
        }
        httpsMode = true;
    }
    else {
        if (protocol != "http") {
            log_e("Unknown protocol");
            return false;
        }
    }

    HTTPClient *client = new HTTPClient;
    if (client == nullptr) {
        log_e("Failed to create HTTPClient");
        return false;
    }

    bool res;
    if (httpsMode) {
        res = client->begin(link, certificate);
    }
    else {
        res = client->begin(link);
    }

    if (!res) {
        log_e("HTTPClient begin failed");
        delete client;
        client = nullptr;
        return false;
    }

    res = false;
    int httpCode = client->GET();
    int httpSize = client->getSize();
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            if (httpSize > 0) {
                int freeSpace = ESP.getFreeSketchSpace();
                if (freeSpace >= httpSize) {
                    res = true;
                }
                else {
                    log_e("Not enough free space, %d < %d", freeSpace, httpSize);
                }
            }
            else {
                log_e("Wrong HTTP size: %d", httpSize);
            }
        }
        else{
            log_e("Unexpected HTTP code, %d", httpCode);
        }
    }
    else {
        log_e("HTTPClient GET failed, %s", client->errorToString(httpCode));
    }

    if (!res) {
        client->end();
        delete client;
        client = nullptr;
        return false;
    }

    WiFiClient* httpStream = client->getStreamPtr();

    res = false;
    if (Update.begin(httpSize, U_FLASH)) {
        if (Update.writeStream(*httpStream)) {
            if (Update.end()) {
                res = true;
            }
            else {
                log_e("Update.end failed, %s", Update.errorString());
            }
        }
        else {
            log_e("Update.writeStream failed, %s", Update.errorString());
        }
    }
    else {
        log_e("Update.begin failed, %s", Update.errorString());
    }

    if (res) {
        log_i("Update success, code size %d", httpSize);
    }

    client->end();
    delete client;
    client = nullptr;
    return res;
}
