#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>


const char* ssid = "<ssid>";
const char* password = "<password>";
char chipid[12];
const char* mqttserver="<mqttserver>";
WiFiServer server(80);
WiFiClient client;
PubSubClient mqttclient(client);

#define HOST "modbusgw-%s"
#define MAXREGSIZE 26
#define SENDINTERVAL 180000



ModbusMaster node;
String req[4];//operation, group, address, value
enum reqtypes {
  reqtemp=0,
  reqalarm,
  reqtime,
  reqcontrol,
  reqspeed,
  reqairtemp,
  reqairflow,
  reqairheat,
  requser,
  requser2,
  reqinfo,
  reqinputairtemp,
  reqapp,
  reqoutput,
  reqdisplay1,
  reqdisplay2,
  reqmax
};
String groups[] ={"temp","alarm","time","control","speed","airtemp","airflow","airheat", "user", "user2", "info", "inputairtemp", "app", "output", "display1","display2"};
byte regsizes[] =     {23, 10, 6,  8,   2,  6,   2,   0, 6,  6,  14, 7,   4, 26, 4,   4};
int  regaddresses[] = {200,400,300,1000,200,1200,1100,0, 600,610,100,1200,0, 100,2002,2007 };
byte regtypes[] =     {8,  0,  1,  1,   1,  1,   1,   1,  1,  1,  0 ,0,   2, 1,  4,   4};
char *regnames[][MAXREGSIZE] = {
  //temp
  {"T0_Controller", NULL, NULL, "T3_Exhaust", "T4_Outlet", NULL, NULL ,"T7_Inlet" ,"T8_Outdoor", NULL, NULL,NULL,NULL,NULL,NULL,"T15_Room",NULL,NULL,NULL, NULL, NULL, "RH",NULL },
  //alarm
  {"Status", "List_1_ID ", "List_1_Date", "List_1_Time", "List_2_ID ", "List_2_Date", "List_2_Time", "List_3_ID ", "List_3_Date", "List_3_Time"},
  //time
  {"Second","Minute","Hour", "Day", "Month", "Year"},
  //control
  {"Type","RunSet","ModeSet","VentSet","TempSet","ServiceMode","ServicePct","Preset"},
  //speed
  {"ExhaustSpeeed", "InletSpeed"},
  //airtemp
  {"CoolSet","TempMinSum", "TempMinWin", "TempMaxSum", "TempMaxWin", "TempSummer"},
  //airflow
  {"AirExchMode", "CoolVent"},
  //airheat
  {},
  //program.user
  {"UserFuncAct","UserFuncSet","UserTimeSet", "UserVentSet","UserTempSet","UserOffsSet"},
  //program.user2
  {"User2FuncAct","User2FuncSet","User2TimeSet","User2VentSet","UserTempSet","UserOffsSet"},
  //info
  {"UserFunc","AirFilter","DoorOpen","Smoke","MotorThermo","Frost_overht","AirFlow","P_Hi","P_Lo","Boil","3WayPos","DefrostHG","Defrost","UserFunc_2"}, 
  //inputairtemp
  {"IsSummer","TempInletSet","TempControl","TempRoom","EffPct","CapSet","CapAct"},
  //app
  {"Bus.Version", "VersionMajor", "VersionMinor", "VersionRelease"},
  //output
  {"AirFlap","SmokeFlap","BypassOpen","BypassClose","AirCircPump","AirHeatAllow","AirHeat_1","AirHeat_2","AirHeat_3","Compressor","Compressor_2","4WayCool","HotGasHeat","HotGasCool","CondOpen","CondClose","WaterHeat","3WayValve","CenCircPump","CenHeat_1","CenHeat_2","CenHeat_3","CenHeatExt","UserFunc","UserFunc_2","Defrosting"},
  //display1
  {"Text_1_2","Text_3_4","Text_5_6","Text_7_8"},
  //display2
  {"Text_1_2","Text_3_4","Text_5_6","Text_7_8"}
  };
  
char * getName(reqtypes type, int address) {
  if (address >=0 && address <= regsizes[type]) {
    return regnames[type][address];
  }
  return NULL;
  
}

static int16_t rsbuffer[MAXREGSIZE];

JsonObject&  HandleRequest(JsonBuffer& jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();
  reqtypes r = reqmax;
  char type = 0;
  if (req[1] != "") {
    for (int i = 0; i < reqmax; i++) {
      if (groups[i] == req[1]) {
        r = (reqtypes)i;
      }
    }
  }
  type = regtypes[r];
  if (req[0] == "read") {
    int address = 0;
    int nums = 0;
    char result = -1;
    address = regaddresses[r];
    nums = regsizes[r];
    
    result = ReadModbus(address,nums, rsbuffer, type & 1);
    if (result == 0) {
      for (int i = 0; i < nums ; i++) {
        char *name = getName(r,i);
        if (name != NULL && strlen(name) > 0) {
          if ((type & 2  && i > 0) || type & 4) {
            String str = "";
            str += (char )(rsbuffer[i] & 0x00ff);
             str += (char)(rsbuffer[i] >> 8);
            root[name] = str;
          } else if (type & 8 ) {
            root[name] = rsbuffer[i]/100.0;
          }else {
            root[name] = rsbuffer[i];
          }
        } 
      }
    }
    root["requestaddress"] = address;
    root["requestnum"] = nums;
  }
  if (req[0] == "set" && req[2] != "" && req[3] != "") {
    int addtoaddress = 0;//type==0?30000:40000;
    int address = atoi(req[2].c_str());
    int value = atoi(req[3].c_str());
    char result = WriteModbus(address + addtoaddress, value);
    root["result"] = result;
    root["address"] = address + addtoaddress;
    root["value"] = value;


  }
  if (req[0] == "help") {
    for (int i = 0; i < reqmax; i++) {
      root[groups[i]] = 0;
    }
  }
  root["operation"] = req[0];
  root["group"] = req[1];
  return root;
}


void setup() {
 char host[64];
  sprintf(chipid, "%08X", ESP.getChipId());
  sprintf(host, HOST, chipid);
  delay(500);
  WiFi.hostname(host);
  ArduinoOTA.setHostname(host);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.onStart([]() {

  });
  ArduinoOTA.onEnd([]() {
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
  });
  ArduinoOTA.begin();
  server.begin();
  Serial.begin(19200,SERIAL_8E1);
  node.begin(30, Serial);


  mqttclient.setServer(mqttserver, 1883);
}


bool readRequest(WiFiClient& client) {
  req[0] = "";
  req[1] = "";
  req[2] = "";
  req[3] = "";
  
  int n =-1;
  bool readstring = false;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n') {
        return false;
      } else if (c == '/') {
        n++;
      } else if (c != ' ' && n >= 0 && n < 4) {
        req[n] += c; 
      } else if (c == ' ' && n >= 0 && n < 4) {
        return true;
      }
    }
  }
  
  return false;
}

void writeResponse(WiFiClient& client, JsonObject& json) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  json.prettyPrintTo(client);
}

static uint32_t shiti;
char ReadModbus(uint16_t addr, uint8_t sizer, int16_t* vals, int type) {
  char result = 0;
  switch (type) {
    case 0: 
      result = node.readInputRegisters(addr,sizer);
    break;
    case 1:
      result = node.readHoldingRegisters(addr,sizer);
      break;
  }
  if (result == node.ku8MBSuccess) {
    for (int j = 0; j < sizer; j++) {
      vals[j] = node.getResponseBuffer(j);
    }
    return result;
  }
  return result;
}
char WriteModbus(uint16_t addr, int16_t val) {
  node.setTransmitBuffer(0, val);
  char result = 0;
  result = node.writeMultipleRegisters(addr, 1);
  return result;
}

static long lastMsg = 0;


void mqttreconnect() {
  int numretries = 0;
  while (!mqttclient.connected() && numretries < 3) {
    if (mqttclient.connect(chipid)) {
      //mqttclient.subscribe("");
    } else {
      delay(1000);
    }
    numretries ++;
  }
}

void loop() {
   #ifdef DEBUG_TELNET
    // handle Telnet connection for debugging
    handleTelnet();
  #endif

  ArduinoOTA.handle();
  WiFiClient client = server.available();
  if (client) {
    bool success = readRequest(client);
    if (success) {
      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& json = HandleRequest(jsonBuffer);//prepareResponse(jsonBuffer);
      writeResponse(client, json);
    }
    client.stop();
  } 

  if (!mqttclient.connected()) {
    mqttreconnect();
  }

  if (mqttclient.connected()) {
    mqttclient.loop();
    long now = millis();
    if (now - lastMsg > SENDINTERVAL) {
          char result = ReadModbus(regaddresses[reqtemp],regsizes[reqtemp], rsbuffer, regtypes[reqtemp]& 1);
      if (result == 0) {
        for (int i = 0; i < regsizes[reqtemp] ; i++) {
          char *name = getName(reqtemp,i);
          char numstr[8];
          if (name != NULL && strlen(name) > 0) {
            String mqname = "temp/";
            
            if (strncmp("RH", name,2) == 0) {
              mqname = "moist/Nilan";
            }
            mqname += (char*)name;
            mqttclient.publish(mqname.c_str(),dtostrf((rsbuffer[i]/100.0), 5,2,numstr));
          } 
        }
      }
      lastMsg = now;
    }
  }
}
