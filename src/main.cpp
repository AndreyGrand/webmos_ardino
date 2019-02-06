#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

// SKETCH BEGIN
AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");
// AsyncEventSource events("/events");

const char *ssid = "Room63";
const char *password = "MyRoom63";
const char *hostName = "esp-async";
const char *http_username = "admin";
const char *http_password = "admin";

const char *PARAM_MESSAGE = "message";
const char *PARAM_GPIO = "gpio";
const char *PARAM_TO = "state";
#define NO_GPIO -1
const char * INDEX_HTM = "<!DOCTYPE html>" \
"<html lang=\"en\">" \
"<head>" \
"    <meta charset=\"UTF-8\">" \
"    <title>My arduino project</title>" \
"</head>" \
"<body>" \
"<form action=\"./\" id=\"gpio_post\" method=\"post\">" \
"    <div>GPIO <input name=\"gpio\" type=\"text\" value=\"GPIO_NUM\"/></div>" \
"   <div>VALUE <br/>" \
"        <input type=\"radio\" name=\"state\" value=\"0\" checked0/>0<br/>" \
"         <input type=\"radio\" name=\"state\" value=\"1\" checked1/>1<br/></div>" \
    "<input type=\"submit\" value=\"POST\">" \
"</form></body></html>";
static String ONE = "1";

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}
const int ports[] = {2, 12};

int getGPIO(AsyncWebServerRequest *request){
    String result = "";
    if (request->hasParam(PARAM_GPIO)){
        result = request->getParam(PARAM_GPIO)->value();
    } 
    else if(request->hasParam(PARAM_GPIO, true))
        {
            result =  request->getParam(PARAM_GPIO, true)->value();
        }
    else{
        Serial.println("No Send GPIO ");
        return NO_GPIO;
    }
    Serial.println("Send GPIO " + result);
    return result.toInt();
}

String makeResponse(int gpio){
    String response = String(INDEX_HTM);
    if(gpio != NO_GPIO){
        int vl = digitalRead(gpio);
        response.replace("GPIO_NUM", String(gpio));
        response.replace("GPIO_VAL", String(vl));
        if(vl == 0){
            response.replace("checked0", "checked=\"checked\"");
            response.replace("checked1", "");
        }
        else{
            response.replace("checked1", "checked=\"checked\"");
            response.replace("checked0", "");
        }
    }
    else{
        response.replace("GPIO_NUM", "");
        response.replace("GPIO_VAL", "");
        response.replace("checked0", "");
        response.replace("checked1", "");
    }
    return response;
}
void printParams(AsyncWebServerRequest *request){
    if (request->hasParam(PARAM_GPIO)){
        Serial.println("GET gpio " + request->getParam(PARAM_GPIO)->value());
    } 
    else if(request->hasParam(PARAM_GPIO, true))
        {
        Serial.println("POST gpio " + request->getParam(PARAM_GPIO, true)->value());
        }
    else{
        Serial.println("No Send GPIO ");
    }
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
}
void setup()
{
    Serial.begin(9600);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    for (int thisPin = 0; thisPin < 1; thisPin++)
    {
        pinMode(ports[thisPin], OUTPUT);
    }
    Serial.setDebugOutput(true);
    Serial.println("embedded led: " + String(LED_BUILTIN));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.hostname());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        int gpio = getGPIO(request);
        request->send(200, "text/html",makeResponse(gpio));
    });
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
        printParams(request);
        int gpio = getGPIO(request);
        if (gpio != NO_GPIO && request->hasParam(PARAM_TO, true))
        {
            String to = request->getParam(PARAM_TO, true)->value();
            digitalWrite(gpio, ONE.equals(to) ? HIGH : LOW);
        }
        request->send(200, "text/html",makeResponse(gpio));
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/board", HTTP_GET, [](AsyncWebServerRequest *request) {
        String message;
        int gpio = getGPIO(request);
        if (gpio != NO_GPIO)
        {
            message = "State of " + String(gpio) + " is " + digitalRead(gpio);
        }
        else
        {
            message = "No message sent";
        }
        request->send(200, "text/plain", "GET: " + message);
        Serial.println(message);
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/board", HTTP_POST, [](AsyncWebServerRequest *request) {
        printParams(request);
        String message;
        int gpio = getGPIO(request);
        if (gpio != NO_GPIO && request->hasParam(PARAM_TO, true))
        {
            String to = request->getParam(PARAM_TO, true)->value();
            message = "Switch state of " + String(gpio) + " to " + to;
            digitalWrite(gpio, ONE.equals(to) ? HIGH : LOW);
        }
        else
        {
            message = "No message sent";
        }
        request->send(200, "text/plain", "POST: " + message);
        Serial.println(message);
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop()
{

}

