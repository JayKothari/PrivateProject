// Control ESP8266 anywhere
//***********************************
//Owner: Jay Kothari
//Description: This module would try to get connect to 
//             configured SSID and Password if it does 
//             not get connected then 
//***********************************
// Import required libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <aREST.h>
#include "WiFiUdp.h"
#include <EEPROM.h>

//Defines
//##################################
#define CONFIG_START      0
#define AP_SSID           "_aashka_"
#define AP_PASSWORD       "Jmk@1232" 
#define CONFIG_VERSION    "v01"
#define AP_CONNECT_TIME   20 //s
//#################################

//Global Variables
//##################################
WiFiClient espClient;
PubSubClient client(espClient);

// Create aREST instance
aREST rest = aREST(client);

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

// Unique ID to identify the device for cloud.arest.io
char* device_id = "jay412";

struct StoreStruct 
{
  // This is for mere detection if they are your settings
  char version[4];
  // The variables of your settings
  uint moduleId;  // module id
  bool state;     // state
  char ssid[20];
  char pwd[20];
} storage = {
  CONFIG_VERSION,
  // The default module 0
  0,
  0, // off
  AP_SSID,
  AP_PASSWORD
};

const unsigned int NET_PORT = 58266;
WiFiUDP Udp;
char NetMsg_Something[] = "ESP8266";
int broadCastCount=0;
//#########################

//Global Functions
//#########################
void callback(char* topic, byte* payload, unsigned int length);
static String macToStr(const uint8_t* mac);
static void loadConfig(void);
static void saveConfig(void);
static void AP_Setup(void);
static void AP_Loop(void);
static void BroadcastPresence(void);
//#########################

//**************************
//Function:macToStr
//Argument: in-Mac address
//return: String
//**************************
static String macToStr(const uint8_t* mac)
{
  String result;
  
  for (int i = 0; i<6; i++) 
  {
    result += String(mac[i], 16);
    if (i < 5)
    {
      result += ':';
    }
  }
  
  return result;
}

//**************************
//Function:loadConfig
//Argument: in-Mac address
//return: String
//**************************
static void loadConfig(void) 
{
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
  {
      for (unsigned int t=0; t<sizeof(storage); t++)
      {
        *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
      }
  }
}


static void saveConfig(void) 
{
  for (unsigned int t=0; t<sizeof(storage); t++)
  {
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
  }

  EEPROM.commit();
}

static void AP_Setup(void)
{
  Serial.println("setting mode");
  WiFi.mode(WIFI_AP);

  String clientName;
  clientName += "Thing-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  
  Serial.println("starting ap");
  WiFi.softAP((char*) clientName.c_str(), "");
  Serial.println("running server");
  server.begin();
}

static void AP_Loop(void)
{
  bool  inf_loop = true;
  int  val = 0;
  WiFiClient client;

  Serial.println("AP loop");

  while(inf_loop)
  {
    while (!client)
    {
      Serial.print(".");
      delay(100);
      client = server.available();
    }
    
    String ssid;
    String passwd;
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    client.flush();

    // Prepare the response. Start with the common header:
    String s = "HTTP/1.1 200 OK\r\n";
    s += "Content-Type: text/html\r\n\r\n";
    s += "<!DOCTYPE HTML>\r\n<html>\r\n";

    if (req.indexOf("&") != -1)
    {
      int ptr1 = req.indexOf("ssid=", 0);
      int ptr2 = req.indexOf("&", ptr1);
      int ptr3 = req.indexOf(" HTTP/",ptr2);
      ssid = req.substring(ptr1+5, ptr2);
      passwd = req.substring(ptr2+10, ptr3);    
      val = -1;
    }

    if (val == -1)
    {
      strcpy(storage.ssid, ssid.c_str());
      strcpy(storage.pwd, passwd.c_str());
      
      saveConfig();
      //storeAPinfo(ssid, passwd);
      s += "Setting OK";
      s += "<br>"; // Go to the next line.
      s += "Continue / reboot";
      inf_loop = false;
    }
    else
    {
      String content="";
      // output the value of each analog input pin
      content += "<form method=get>";
      content += "<label>SSID</label><br>";
      content += "<input  type='text' name='ssid' maxlength='19' size='15' value='"+ String(storage.ssid) +"'><br>";
      content += "<label>Password</label><br>";
      content += "<input  type='password' name='password' maxlength='19' size='15' value='"+ String(storage.pwd) +"'><br><br>";
      content += "<input  type='submit' value='Submit' >";
      content += "</form>";
      s += content;
    }
    
    s += "</html>\n";
    // Send the response to the client
    client.print(s);
    delay(1);
    client.stop();
  }
}

static void BroadcastPresence(void)
{
  // Calculate this network broadcast address
  IPAddress broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;
//  IPAddress broadcastIP = subnetMask | gatewayIP;
  
  //IPAddress broadcastIP = WiFi.subnetMask() | WiFi.gatewayIP();
  // A broadcast packet goes to every host on the subnet.
  // It is a NORMAL packet but sent to a specific address.
 // Serial.print(broadcastIP);
  Udp.beginPacket(broadcastIP, NET_PORT);
  Udp.write(NetMsg_Something);
  Udp.endPacket();
}

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);

  EEPROM.begin(512);
  
  loadConfig();

  // Set callback
  client.setCallback(callback);
  
  // Give name and ID to device
  rest.set_id(device_id);
  rest.set_name("relay_anywhere");

  Serial.print("Scan start ... ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" network(s) found");
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i));
  }
  Serial.println();

  int i = 0;
  
  // Connect to WiFi
  WiFi.mode(WIFI_STA);

  Serial.println("************ Jay Kothari ***************");
  //Serial.println(storage.ssid);
  //Serial.println(storage.pwd);
  WiFi.begin(storage.ssid, storage.pwd);

  Serial.print("Connecting ... ");
  Serial.print(storage.ssid);
  Serial.print("\n");
  //WiFi.begin(ssid, password);
  //IPAddress myip = WiFi.softAP(ssid, password);
  //Serial.println(myip);
  while (WiFi.status() != WL_CONNECTED 
          && i++ < (AP_CONNECT_TIME*2))
  {
    delay(500);
    Serial.print("WiFi not connected.\n");
    Serial.print(WiFi.status());
    Serial.print("\n");
   // Serial.println(myip);
  }

  if (!(i < (AP_CONNECT_TIME*2)))
  {
    AP_Setup();
    AP_Loop();
    ESP.reset();
  }
  
  Serial.println("");
  Serial.println("WiFi connected...");

  //Start the server
  server.begin();
  Serial.println("Server started");
  
  Serial.println(WiFi.localIP());

  // Set output topic
//  char* out_topic = rest.get_topic();
 // char* out_topic = rest.get_topic();
}

void loop()
{  
  // Connect to the cloud
  rest.loop(client);

  broadCastCount++;
  if ( broadCastCount == 100000)
  {
    broadCastCount=0;
    BroadcastPresence();
  }

   // Check if a client has connected
  WiFiClient client = server.available();
  if (client) 
  {
    // Wait until the client sends some data
    Serial.println("new client");
    while(!client.available())
    {
      delay(1);
    }
  
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();
  
    // Match the request
    int val;
    if (req.indexOf("/digital/2/0") != -1)
    {
      val = 0;
      // Set GPIO2 according to the request
      digitalWrite(2, val);
    }
    else if (req.indexOf("/digital/2/1") != -1)
    {
      val = 1;
      // Set GPIO2 according to the request
      digitalWrite(2, val);
    }
    else if (req.indexOf("/digital/16/0") != -1)
    {
      val = 0;
      // Set GPIO16 according to the request
      digitalWrite(2, val);
    }
    else if (req.indexOf("/digital/16/1") != -1)
    {
      val = 1;
      // Set GPIO16 according to the request
      digitalWrite(2, val);
    }
    else 
    {
      Serial.println("invalid request");
      client.stop();
      return;
    }
  
    client.flush();
 
    // Prepare the response
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
    s += (val)?"high":"low";
    s += "</html>\n";
 
    // Send the response to the client
    client.print(s);
    delay(1);
    Serial.println("Client disonnected");

    
    // The client will actually be disconnected 
    // when the function returns and 'client' object is detroyed
    //return;
  }
}

// Handles message arrived on subscribed topic(s)
void callback(char* topic, byte* payload, unsigned int length) 
{
  rest.handle_callback(client, topic, payload, length);
}
