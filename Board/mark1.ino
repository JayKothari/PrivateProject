// Control ESP8266 anywhere
//***********************************
//Owner: Jay Kothari
//Description: This module would try to get connect to 
//             configured SSID and Password if it does 
//             not get connected then 
//***********************************
// Import required libraries
#include <ESP8266WiFi.h>
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

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

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
static String macToStr(const uint8_t* mac);
static void loadConfig(void);
static void saveConfig(void);
static void AP_Setup(void);
static void AP_Loop(void);
static void BroadcastPresence(void);
static void setupGpioMode(void);
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
//Function:setupGpioMode
//Argument: void
//return: void
//**************************
static void setupGpioMode(void)
{
   pinMode(2, OUTPUT);
   pinMode(16, OUTPUT);
}

//**************************
//Function:loadConfig:- Read config.
//Argument: void
//return: void
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

//**************************
//Function:saveConfig:- write to eeprom.
//Argument: void
//return: void
//**************************
static void saveConfig(void) 
{
  for (unsigned int t=0; t<sizeof(storage); t++)
  {
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
  }

  EEPROM.commit();
}

//**************************
//Function:AP_Setup: access point setup.
//Argument: void
//return: void
//**************************
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

//**************************
//Function:AP_Loop:- access point loop 
//Argument: void
//return: void
//**************************
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

//**************************
//Function:BroadcastPresence:- Broadcast the and tell everyone 
//                               about the presence.
//Argument: void
//return: void
//**************************
static void BroadcastPresence(void)
{
  // Calculate this network broadcast address
  IPAddress broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;

  // A broadcast packet goes to every host on the subnet.
  // It is a NORMAL packet but sent to a specific address.
  // Serial.print(broadcastIP);
  Udp.beginPacket(broadcastIP, NET_PORT);
  Udp.write(NetMsg_Something);
  Udp.endPacket();
}

//**************************
//Function:setup : Its called once before the loop function.
//Argument: void
//return: void
//**************************
void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);

  EEPROM.begin(512);
  
  loadConfig();

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
  while ((WiFi.status() != WL_CONNECTED) 
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

  setupGpioMode();
}

//**************************
//Function:loop : Its the main loop
//Argument: void
//return: void
//**************************
void loop()
{  
  broadCastCount++;
  if ( broadCastCount == 500000)
  {
    broadCastCount=0;
    BroadcastPresence();
  }

   // Check if a client has connected
  WiFiClient clientLocal = server.available();
  if (clientLocal) 
  {
    // Wait until the client sends some data
    Serial.println("new client");
    while(!clientLocal.available())
    {
      delay(1);
    }
  
    // Read the first line of the request
    String req = clientLocal.readStringUntil('\r');
    Serial.println(req);
    clientLocal.flush();
  
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
    else if (req.indexOf("/digital/2/state") != -1)
    {
      val = digitalRead(2);
    }
    else if (req.indexOf("/digital/16/0") != -1)
    {
      val = 0;
      // Set GPIO16 according to the request
      digitalWrite(16, val);
    }
    else if (req.indexOf("/digital/16/1") != -1)
    {
      val = 1;
      // Set GPIO16 according to the request
      digitalWrite(16, val);
    }
    else if (req.indexOf("/digital/16/state") != -1)
    {
      val = digitalRead(16);
    }
    else 
    {
      Serial.println("invalid request");
      clientLocal.stop();
      return;
    }
  
    clientLocal.flush();
 
    // Prepare the response
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nSwitch is now ";
    s += (val)?"ON":"OFF";
    s += "</html>\n";
 
    // Send the response to the client
    clientLocal.print(s);
    delay(1);
    Serial.println("Client disonnected");

    
    // The client will actually be disconnected 
    // when the function returns and 'client' object is detroyed
    //return;
  }
}
