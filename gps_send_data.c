#include <gps.h>         //.. for gps_*()
#include <math.h>        // for isfinite()
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
    "3D"
};

struct gps_data_t gpsData;
static volatile int exitLoop = 0;

double roundSixPlaces(double var)
{
    
    char* str; 

    sprintf(str, "%.6d", var);

    sscanf(str, "%d", &var); 

    return var; 
}


CURLcode makePost(double latitude, double longitude){
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl == NULL) {
        return 128;
    }

    cJSON* latitudeJson = cJSON_CreateNumber(latitude);
    cJSON* longitudeJson = cJSON_CreateNumber(longitude);
    
    
    cJSON* jObj = cJSON_CreateObject();
    
    cJSON_AddItemToObject(jObj, "latitude", latitudeJson);

    cJSON_AddItemToObject(jObj, "longitude", longitudeJson);


    char* jsonObj = cJSON_Print(jObj);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charset: utf-8");

    char* url = getenv("GPS_API");


    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcrp/0.1");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return res;
}

void sigintHandler(int number){
    puts("Signal sent");
    exitLoop = 1;
   
    
}

int gpsConnect(){
    if (0 != gps_open("localhost", "2947", &gpsData)) {
        printf("Couldn't open GPS connection\n");
        return 1;
    }

    (void)gps_stream(&gpsData, WATCH_ENABLE | WATCH_JSON, NULL);

    return 0;
}


int main(int argc, char *argv[])
{
    
    signal(SIGINT, sigintHandler);

    double previousLatitude = 0;
    double previousLongitude = 0;	
    int read_result = 0;

    gpsConnect();

    while (exitLoop == 0 && gps_waiting(&gpsData, 5000000)) {
		
		
		read_result = gps_read(&gpsData, NULL, 0);
        if (-1 == read_result) {
            printf("Read error.\n");

            if(gpsConnect() == 1){
                return 1;
            }

            continue;
        }
        if (MODE_SET != (MODE_SET & gpsData.set)) {
            // did not even get mode, nothing to see here
            continue;
        }
        if (0 > gpsData.fix.mode ||
            MODE_STR_NUM <= gpsData.fix.mode) {
            gpsData.fix.mode = 0;
        }
        printf("Fix mode: %s (%d) Time: ",
               mode_str[gpsData.fix.mode],
               gpsData.fix.mode);
        if (TIME_SET == (TIME_SET & gpsData.set)) {
            // not 32 bit safe
            printf("%ld.%09ld ", gpsData.fix.time.tv_sec,
                   gpsData.fix.time.tv_nsec);
        } else {
            puts("n/a ");
        }
        if (isfinite(gpsData.fix.latitude) &&
            isfinite(gpsData.fix.longitude)) {
            // Display data from the GPS receiver if valid.
            printf("Lat %.6f Lon %.6f\n",
                   gpsData.fix.latitude, gpsData.fix.longitude);
         
	     	
	    

	      int result = makePost(gpsData.fix.latitude, gpsData.fix.longitude);


	      
	    
	      if(result != 0){
	    	  printf("Couldn't connect to server: %d\n", result);
	    
	    
	    
	      }
	    
        
        } else {
            printf("Lat n/a Lon n/a\n");
        }

	sleep(5);
        
    }
    (void)gps_stream(&gpsData, WATCH_DISABLE, NULL);
    (void)gps_close(&gpsData);
    puts("Closed");
    return 0;
}
