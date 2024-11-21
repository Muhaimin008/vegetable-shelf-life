#include <Wire.h>
#include <WiFi.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include <WiFiManager.h>
#include <HTTPClient.h>
#include "time.h"


const byte MLX90640_address = 0x33; // Default 7-bit unshifted address of the MLX90640
#define TA_SHIFT 8 // Default shift for MLX90640 in open air

static float mlx90640To[768]; // Array to hold temperature values
paramsMLX90640 mlx90640;


boolean isConnected();

int state = 1;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 21600;
const int   daylightOffset_sec = 0;

// Google script ID and required credentials
String GOOGLE_SCRIPT_ID = "AKfycbz5WSoq1RmwRYSGG8KngJnzF0YC3upcLOowuvIE7e5oGOdOdsQZpS8tjCFd3vESv_ne";    // change Gscript ID
bool res;

String tmp1;
String tmp2;
String tmp3;
String tmp4;

int succ = 0;

void setup() {
    pinMode(4, OUTPUT);
    int state = 1;
    Serial.begin(115200);
    wifi_connect();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Wire.begin();
    Wire.setClock(400000); // Increase I2C clock speed to 400kHz
    digitalWrite(4, LOW);
    while (!Serial); // Wait for user to open terminal
    Serial.println("MLX90640 IR Array Example");

    if (!isConnected()) {
        Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
        while (1){
          Serial.println("MLX90640 offline!");
          digitalWrite(4, HIGH);
//          Wire.end(); 
          delay(10000); 
          ESP.restart();
        }
        state = 0;
        
    }
       
    Serial.println("MLX90640 online!");

    // Initialize MLX90640 sensor
    int status;
    uint16_t eeMLX90640[832];
    status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
    if (status != 0) Serial.println("Failed to load system parameters");

    status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
    if (status != 0) {
        Serial.println("Parameter extraction failed");
        Serial.print("status = ");
        Serial.println(status);
        state = 0;
        digitalWrite(4, HIGH);
//        Wire.end();
        delay(5000); 
//        ESP.restart();
    }

    MLX90640_SetRefreshRate(MLX90640_address, 0x04); // Set rate to 4Hz effective

}

void loop() {
    succ=0;
    while (succ==0){
      thermal_reading(); 
    }
    delay(1000);
    Serial.println("\n___________New Reading_________");
//    Serial.print("tmp1: ");
//    Serial.println(tmp1);
//    Serial.print("tmp2: ");
//    Serial.println(tmp2);

    if (res) {
        static bool flag = false;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
          Serial.println("Failed to obtain time");
        //      return;
        }
        char timeStringBuff[50]; //50 chars should be enough
        strftime(timeStringBuff, sizeof(timeStringBuff), "%A %B %d %Y %H:%M:%S", &timeinfo);
        String asString(timeStringBuff);
        asString.replace(" ", "-");
        Serial.print("Time:");
        Serial.println(asString);
        String urlFinal = "https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?"+"date=" + asString + "&tmp1=" + String(tmp1)+ "&tmp2=" + String(tmp2)+ "&tmp3=" + String(tmp3)+ "&tmp4=" + String(tmp4);
//        Serial.print("POST data to spreadsheet:");
//        Serial.println(urlFinal);
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET(); 
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);
        Serial.print("tmp1  length(): ");
        Serial.print(tmp1.length());
        Serial.print("   tmp2  length(): ");
        Serial.println(tmp2.length());
        Serial.print("tmp3  length(): ");
        Serial.print(tmp3.length());
        Serial.print("   tmp4  length(): ");
        Serial.println(tmp4.length());        
        
        //---------------------------------------------------------------------
        //getting response from google sheet
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
//            Serial.println("Payload: "+payload);    
        }
        //---------------------------------------------------------------------
        http.end();
    }
    delay(17000);
  
}

void error_msg() {
          static bool flag = false;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
          Serial.println("Failed to obtain time");
        //      return;
        }
        char timeStringBuff[50]; //50 chars should be enough
        strftime(timeStringBuff, sizeof(timeStringBuff), "%A %B %d %Y %H:%M:%S", &timeinfo);
        String asString(timeStringBuff);
        asString.replace(" ", "-");
        Serial.print("Time:");
        Serial.println(asString);
        String urlFinal = "https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?"+"date=" + asString + "&tmp1=" + "restart" + "&tmp2=" + "restart" + "&tmp3=" + "restart" + "&tmp4=" + "restart";
//        Serial.print("POST data to spreadsheet:");
//        Serial.println(urlFinal);
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET(); 
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);
        Serial.print("tmp1  length(): ");
        Serial.print(tmp1.length());
        Serial.print("   tmp2  length(): ");
        Serial.println(tmp2.length());
        Serial.print("tmp3  length(): ");
        Serial.print(tmp3.length());
        Serial.print("   tmp4  length(): ");
        Serial.println(tmp4.length());        
        
        //---------------------------------------------------------------------
        //getting response from google sheet
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
//            Serial.println("Payload: "+payload);    
        }
        //---------------------------------------------------------------------
        http.end();
}

// Utility function to check if MLX90640 is connected
boolean isConnected() {
    Wire.beginTransmission((uint8_t)MLX90640_address);
    if (Wire.endTransmission() != 0) return false; // Sensor did not ACK
    return true;
}

void thermal_reading() {
    tmp1 = "";
    tmp2 = "";
    tmp3 = "";
    tmp4 = "";
    succ = 1;
    for (byte x = 0; x < 2; x++) { // Read both subpages
      uint16_t mlx90640Frame[834];
      int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
      
      if (status < 0) {
          Serial.print("GetFrame Error: ");
          Serial.print(status);
//          continue;
          succ = 0;
          digitalWrite(4, HIGH);
//          error_msg();
//          Serial.println(" : >>> Restarting ESP");
          delay(5000);
//          ESP.restart();
      }
      digitalWrite(4, LOW);
      if (succ==0){
        break;
      }
      float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
      float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);
      float tr = Ta - TA_SHIFT; //Reflected temperature
      float emissivity = 0.95;

      MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);

      // Eliminate error pixels by averaging neighboring pixels
      mlx90640To[1*32 + 21] = 0.5 * (mlx90640To[1*32 + 20] + mlx90640To[1*32 + 22]);
      mlx90640To[4*32 + 30] = 0.5 * (mlx90640To[4*32 + 29] + mlx90640To[4*32 + 31]);
// 768
//      Serial.print("GetFrame Error: ");
      for (int i = 0; i < 384; i++) {
          if (x==0){
            tmp1 = tmp1 + String(mlx90640To[i], 2);
            if (i<383){
              tmp1 = tmp1 + "";
            }
          }
          else{
            tmp2 = tmp2 + String(mlx90640To[i], 2);
            if (i<383){
            tmp2 = tmp2 + "";
            }      
          }  
        }
      for (int i = 384; i < 768; i++) {
          if (x==0){
            tmp3 = tmp3 + String(mlx90640To[i], 2);
            if (i<767){
            tmp3 = tmp3 + "";
            }
          }
          else{
            tmp4 = tmp4 + String(mlx90640To[i], 2);
            if (i<767){
            tmp4 = tmp4 + "";
            }    
          }  
        }

      delay(1000); // Delay between frames, adjust as necessary for your application
  }

}


void wifi_connect(){
    WiFi.mode(WIFI_STA);

    WiFiManager wm;
 
    res = wm.autoConnect("ESP1","pass1234"); // password protected ap
 
    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
}
