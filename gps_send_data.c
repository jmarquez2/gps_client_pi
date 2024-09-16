#include <gps.h>  //.. for gps_*()
#include <math.h> // for isfinite()
#include <curl/curl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "cJSON.h"

#define MODE_STR_NUM 4
static char *mode_str[MODE_STR_NUM] = {
    "n/a",
    "None",
    "2D",
    "3D"};

typedef struct
{
    CURLcode curlCode;
    long statusCode;
} curl_response;

struct gps_data_t gpsData;
static volatile int exitLoop = 0;

double roundSixPlaces(double var);

size_t response_data(void *b, size_t size, size_t nmemb, void *userp);

curl_response makePost(double latitude, double longitude);

int gpsConnect();

int gpsDisconnect();

int gpsReset();



void sigintHandler(int number);


int main(int argc, char *argv[])
{

    signal(SIGINT, sigintHandler);

    double previousLatitude = 0;
    double previousLongitude = 0;
    int read_result = 0;

    printf("Starting gps client program\n");

    gpsConnect();

    while (exitLoop == 0 && gps_waiting(&gpsData, 5000000))
    {

        read_result = gps_read(&gpsData, NULL, 0);
        if (-1 == read_result)
        {
            printf("Read error.\n");

            int conectResult = gpsConnect();
            if (conectResult == 1)
            {
                printf("Couldn\'t connect to device status: %d", conectResult);
                sleep(5);
            }

            continue;
        }

        if (MODE_SET != (MODE_SET & gpsData.set))
        {
            // did not even get mode, nothing to see here
            continue;
        }
        if (gpsData.fix.mode <= 1 ||
            gpsData.fix.mode >= MODE_STR_NUM)
        {
            gpsReset(); //mode should be 2 or 3, otherwise no meaningful data
        }

       
        
        if (isfinite(gpsData.fix.latitude) &&
            isfinite(gpsData.fix.longitude))
        {

            curl_response result = makePost(gpsData.fix.latitude, gpsData.fix.longitude);

            if (!(result.statusCode == 200 || result.curlCode != CURLE_ABORTED_BY_CALLBACK))
            {
                printf("Error sending coordinates: status code: %d, curlCode: %d\n", result.statusCode, result.curlCode);
            }
        }

        sleep(5);
    }

    int disconnectStatus = gpsDisconnect();
    if(disconnectStatus != 0){
        printf("Error disconecting status: %d\n", disconnectStatus);
    }else{
         puts("Closed");
    }
    return 0;
}

double roundSixPlaces(double var)
{

    char *str;

    sprintf(str, "%.6d", var);

    sscanf(str, "%d", &var);

    return var;
}

size_t response_data(void *b, size_t size, size_t nmemb, void *userp)
{
    return size * nmemb;
}

curl_response makePost(double latitude, double longitude)
{
    CURL *curl;
    CURLcode res;

    curl_response resp;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl == NULL)
    {
        resp.curlCode = CURLE_SEND_ERROR;
        resp.statusCode = -1;
        return resp;
    }

    cJSON *latitudeJson = cJSON_CreateNumber(latitude);
    cJSON *longitudeJson = cJSON_CreateNumber(longitude);

    cJSON *jObj = cJSON_CreateObject();

    cJSON_AddItemToObject(jObj, "latitude", latitudeJson);

    cJSON_AddItemToObject(jObj, "longitude", longitudeJson);

    char *jsonObj = cJSON_Print(jObj);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charset: utf-8");

    char *url = getenv("GPS_API");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_data);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcrp/0.1");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    resp.curlCode = res;
    resp.statusCode = httpCode;

    curl_easy_cleanup(curl);
    curl_global_cleanup();


    return resp;
}

int gpsConnect()
{
    int openGps =  gps_open("localhost", "2947", &gpsData);
    if (0 != openGps)
    {
        printf("Couldn't open GPS connection\n");
        return openGps;
    }

    int result = gps_stream(&gpsData, WATCH_ENABLE | WATCH_JSON, NULL);

    return result;
}



int gpsDisconnect()
{
    int stopStream = gps_stream(&gpsData, WATCH_DISABLE, NULL);

    if(stopStream != 0){
        printf("Couldn't stop gps stream: %d\n", stopStream);
        return stopStream;
    }
    int closeResult = gps_close(&gpsData);
    return 0;
}

int gpsReset(){
    int disconnectStatus = gpsDisconnect();
    if(disconnectStatus != 0){
        printf("Error disconnecting gps: %d\n", disconnectStatus);
        return disconnectStatus;
    }

    return gpsConnect();
}

void sigintHandler(int number)
{
    puts("Signal sent");
    exitLoop = 1;
}
