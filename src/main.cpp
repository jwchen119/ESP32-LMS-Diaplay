// This project is inspired by https://github.com/jajito/LMS-ESP32-Monitor-ZX2D10GE01R-V4848/

#include <Arduino.h>
#include <string.h>
#include "FS.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <lvgl.h>
#include <ui/ui.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFiManager.h>
#include <RotaryEncoder.h>

#define PIN_IN1 25
#define PIN_IN2 26
RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

const int discoveryPort = 3483;

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
WiFiUDP udp;
HTTPClient http;
Preferences preferences;
String jsonBuffer;
WiFiManager wm;

unsigned long previousMillis = 0;
unsigned long interval = 1000;
unsigned long currentMillis;
int sleep_threhold = 10; // 目前定義為進入檢查頻率的次數，與interval有相對關係
int sleep_btn = 0;
bool deepSleep = false;

char autoConnectSSID[] = "ESP32_LMS_DISPLAY";
char autoConnectPW[] = "sfhes23452h";

static lv_disp_drv_t disp_drv;
static lv_group_t *lv_group;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;

String lmsPlayer = "";
String displayPlayer = "";
bool is_there_player = false;
String url;
int number_of_players = 1;
const char *PlayerId[10]; // maximum number of players, change if there are more
const char *PlayerName[10];

int volumen_lvgl = 50;
int focus_player = 0;
const char *title = "";
const char *artist = "";
const char *last_title = "";
const char *last_artist = "";

int actualSongId = 0;
int loop_int = 0;
bool is_playing = 0;
int discoveryAttemp = 0;

struct ServerInfo
{
  String ip = "";
  String name = "";
  int port = 0;
};

ServerInfo lmsServer;

// String lmsPlayerPlayList = "";
// LMS codes
String lmsPlayerStatus = "[\"status\",\"-\",\"1\",\"tags:cdegiloqrstuyAABGIKNST\"]";
String lmsNextSong = "[\"playlist\",\"index\",\"+1\"]";
String lmsPreviousSong = "[\"playlist\",\"index\",\"-1\"]";
String lmsServerStatus = "[\"serverstatus\",\"0\",\"100\"]";
String lmsPlaySong = "[\"play\"]";
String lmsStopSong = "[\"stop\"]";
String lmsPauseSong = "[\"pause\"]";
String lmsVolume = "[\"mixer\",\"volume\",\"<i>\"]";
// String ListNumber = "0";
// String lmsPlayNumberSong = "[\"playlist\",\"index\",\"" + ListNumber + "\"]";
// String lmsThisSong = "[\"playlist\",\"index\",\"+0\"]";

void Check_Regular();
void Get_Number_of_Players(void);
void Update_Players(void);
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void encoder_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
void init_lv_group();
bool SendCommand(String url, String command);
String extractName(byte *data, int length);
int extractPort(byte *data, int length);
bool serverDiscovery();
// void serverStatus(void);

bool serverDiscovery()
{
  // Broadcast discovery request
  byte req[] = {'e', 'I', 'P', 'A', 'D', 0, 'N', 'A', 'M', 'E', 0, 'J', 'S', 'O', 'N', 0};
  IPAddress broadcastAddress(255, 255, 255, 255);
  udp.beginPacket(broadcastAddress, discoveryPort);
  udp.write(req, sizeof(req));
  udp.endPacket();

  // Receive response
  byte resp[256];
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    udp.read(resp, sizeof(resp));
    if (resp[0] == 'E')
    {
      // ServerInfo server;
      lmsServer.ip = udp.remoteIP().toString();
      lmsServer.name = extractName(resp, packetSize);
      lmsServer.port = extractPort(resp, packetSize);
      // Parse the response and handle the server information
      // ServerInfo server = processResponse(resp, packetSize);

      // Print IP address, port, and other server details
      Serial.print("Server IP: ");
      Serial.println(lmsServer.ip);
      Serial.print("Server Port: ");
      Serial.println(lmsServer.port);
      Serial.print("Server Name: ");
      Serial.println(lmsServer.name);
      if (lmsServer.port != 0 && lmsServer.name != "")
      {
        return true;
      }
    }
  }
  return false;
}

int extractPort(byte *data, int length)
{
  for (int i = 1; i < length - 1; i++)
  {
    if (data[i] == 'J' && data[i + 1] == 'S' && data[i + 2] == 'O' && data[i + 3] == 'N' && data[i + 4] == 4)
    {
      int j = i + 5;
      int port = 0;
      while (j < length && data[j] >= '0' && data[j] <= '9')
      {
        port = port * 10 + (data[j] - '0');
        j++;
      }
      if (port != 0)
        return port;
    }
  }
  return 0;
}

String extractName(byte *data, int length)
{
  String name = "";
  int nameStartIndex = -1;
  for (int i = 1; i < length - 4; i++)
  {
    if (data[i] == 'N' && data[i + 1] == 'A' && data[i + 2] == 'M' && data[i + 3] == 'E' && data[i + 4] == 3)
    {
      nameStartIndex = i + 5;
      break;
    }
  }

  // Find the index before 'J', 'S', 'O', 'N' which indicates the end of the name
  int nameEndIndex = -1;
  for (int i = nameStartIndex; i < length - 4; i++)
  {
    if (data[i] == 'J' && data[i + 1] == 'S' && data[i + 2] == 'O' && data[i + 3] == 'N' && data[i + 4] == 4)
    {
      nameEndIndex = i - 1;
      break;
    }
  }

  // Extract the name
  if (nameStartIndex != -1 && nameEndIndex != -1)
  {
    for (int i = nameStartIndex; i <= nameEndIndex; i++)
    {
      name += (char)data[i];
    }
    return name;
  }
  else
  {
    return "";
  }
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  // Serial.println("Entered config mode");
  char const *data;
  String dataString = "WIFI:S:" + String(autoConnectSSID) + ";T:WPA;P:" + String(autoConnectPW) + ";;";
  data = dataString.c_str();
  lv_obj_t *qr = lv_qrcode_create(lv_scr_act(), 80, lv_color_white(), lv_color_black());
  lv_qrcode_update(qr, data, strlen(data));
  lv_obj_align(qr, LV_ALIGN_CENTER, 0, 70);
  lv_label_set_text(ui_Label10, autoConnectSSID);
  lv_label_set_text(ui_Label9, autoConnectPW);
  lv_label_set_text(ui_Label12, WiFi.softAPIP().toString().c_str());
  lv_disp_load_scr(ui_Screen2);
  lv_task_handler();
}

// void serverStatus(void)
// {
//   if ((WiFi.status() == WL_CONNECTED))
//   {
//     String lmsjson = "{\"method\": \"slim.request\", \"params\": [\""
//                      "\", " +
//                      lmsServerStatus + "]}";
//     Serial.println(lmsjson);
//     http.begin(url);
//     String payload = "{}";
//     int httpCode = http.POST(lmsjson);
//     if (httpCode > 0)
//     {
//       // Serial.print("HTTP Response code: ");
//       // Serial.println(httpCode);
//       payload = http.getString();
//     }
//     else
//     {
//       Serial.print("Check_Regular Error code: ");
//       Serial.println(httpCode);
//       return;
//     }

//     // Serial.print ("Payload : ");
//     // Serial.println(payload);
//     // DynamicJsonDocument doc(10000);
//     StaticJsonDocument<4096> doc;
//     DeserializationError error = deserializeJson(doc, payload);
//     if (error)
//     {
//       Serial.print(F("deserializeJson check regular() failed: "));
//       Serial.println(error.f_str());
//       return;
//     }
//     http.end();
//     JsonArray player_loop = doc["result"]["players_loop"];
//     int arraySize = player_loop.size();
//     Serial.println(arraySize);
//     for (int i = 0; i < arraySize; i++)
//     {
//       PlayerNameS[i] = doc["result"]["players_loop"][i]["name"];
//       PlayerIdS[i] = doc["result"]["players_loop"][i]["playerid"];
//       Serial.println("Player No. " + String(i) + " Name: " + PlayerNameS[i] + " Id: " + PlayerIdS[i]);
//     }
//   }
// }

void Check_Regular()
{
  // char buffer[100];
  if ((WiFi.status() == WL_CONNECTED))
  {
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + lmsPlayerStatus + "]}";

    // Serial.println(lmsjson);
    http.begin(url);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0)
    {
      // Serial.print("HTTP Response code: ");
      // Serial.println(httpCode);
      payload = http.getString();
    }
    else
    {
      Serial.print("Check_Regular Error code: ");
      Serial.println(httpCode);
      return;
    }

    // Serial.print ("Payload : ");
    // Serial.println(payload);
    // DynamicJsonDocument doc(10000);
    StaticJsonDocument<5600> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print(F("deserializeJson check regular() failed: "));
      Serial.println(error.f_str());
      return;
    }
    http.end();
    JsonObject result = doc["result"];
    const char *result_mode = result["mode"];
    int waitingToPlay = result["waitingToPlay"];
    //
    if (strcmp(result_mode, "play") == 0)
    {
      if (waitingToPlay)
      {
        lv_label_set_text(ui_Label3, "Wait to play");
        lv_label_set_text(ui_Label4, "");
        lv_label_set_text(ui_Label5, "");
      }
      else
      {
        title = result["remoteMeta"]["title"];
        if (title)
        {
          // Serial.println(title);
        }
        else
        {
          title = result["playlist_loop"][0]["title"];
          if (title)
          {
            // Serial.println(title);
          }
          else
          {
            lv_label_set_text(ui_Label3, "");
            title = "";
          }
        }

        artist = result["remoteMeta"]["artist"];
        if (artist)
        {
          // Serial.println(artist);
        }
        else
        {
          artist = result["playlist_loop"][0]["trackartist"];
          if (artist)
          {
            // Serial.println(artist);
          }
          else
          {
            lv_label_set_text(ui_Label4, "");
            artist = "";
          }
        }
      }

      // Serial.println(title);
      // Serial.println(last_title);
      // Serial.println(artist);
      // Serial.println(last_artist);
      if ((strncmp(title, last_title, 8) != 0 || strncmp(artist, last_artist, 8) != 0) && (waitingToPlay == 0 || waitingToPlay == NULL))
      {
        lv_label_set_text(ui_Label3, title);
        lv_label_set_text(ui_Label4, artist);
        last_title = lv_label_get_text(ui_Label3);
        last_artist = lv_label_get_text(ui_Label4);
      }

      // int bitrate = result["playlist_loop"][0]["bitrate"];
      // int samplerate = result["playlist_loop"][0]["samplerate"];
      // snprintf(buffer, 99, "%dK - %d", (samplerate / 1000), bitrate);
      // bitrate &&samplerate ? lv_label_set_text(ui_Label5, buffer) : lv_label_set_text(ui_Label5, "");
    }
    else if (strcmp(result_mode, "pause") == 0)
    {
      lv_label_set_text(ui_Label3, "Paused");
      lv_label_set_text(ui_Label4, "");
      lv_label_set_text(ui_Label5, "");
    }
    else
    {
      lv_label_set_text(ui_Label3, "Not Playing");
      lv_label_set_text(ui_Label4, "");
      lv_label_set_text(ui_Label5, "");
    }
    int result_mixer_volume = result["mixer volume"]; // 85
    double result_duration = result["duration"];      // 226.666
    double result_time = result["time"];
    int duration_percentage = int((result_time / result_duration) * 100);

    lv_arc_set_value(ui_Arc1, result_mixer_volume); // updates volume in lvgl
    lv_arc_set_value(ui_Arc2, duration_percentage);
    volumen_lvgl = result_mixer_volume;
    lv_label_set_text_fmt(ui_Label1, "%02d", volumen_lvgl);
    lv_label_set_text(ui_Label2, PlayerName[focus_player]);
  }
}

bool SendCommand(String url, String command)
{
  // Serial.println("Sending Command "  + command + " to " + url);
  //  Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED))
  {
    // PRINT_STR("[HTTP] begin...\n");
    // HTTPClient http;
    //  Configure server and url
    http.begin(url);
    int httpCode = http.POST(command);
    if (httpCode > 0)
    {
      // Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      //  File found at server
      if (httpCode == HTTP_CODE_OK)
      {
        // Serial.println ("Command sent");
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        // PRINT_STR(" [HTTP] GET... failed, error");
        return 0;
      }
      // http.end();
    }
    String s = http.getString(); // to avoid error "no more processes"
    // Serial.print(s + "\n");
    http.end();
    return 1; // File was fetched from web
  }
}

void Get_Number_of_Players(void)
{
  // Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED))
  {
    // Serial.println("Get number of players from " + url);
    http.begin(url);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"FF:FF:FF:FF\", [\"player\",\"count\",\"?\"]]}";
    // Serial.println(lmsjson);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0)
    {
      // Serial.print("HTTP Response code: ");
      // Serial.println(httpCode);
      payload = http.getString();
    }
    else
    {
      Serial.print("Get_Number_of_Players Error code: ");
      Serial.println(httpCode);
      return;
    }
    // Serial.print("Payload : ");
    // Serial.println(payload);
    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print("deserializeJson number of players() failed: ");
      Serial.println(error.c_str());
      return;
    }
    http.end();
    number_of_players = doc["result"]["_count"];
    // Serial.println("Number of players : " + String(number_of_players));
  }
}

void Update_Players(void)
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    // Serial.println("Get players from " + url);
    http.begin(url);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"FF:FF:FF:FF\", [\"players\",\"0\",\"" + String(number_of_players) + "\"]]}";
    // Serial.println (lmsjson);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0)
    {
      // Serial.print("HTTP Response code: ");
      // Serial.println(httpCode);
      payload = http.getString();
    }
    else
    {
      Serial.print("Update_Players Error code: ");
      Serial.println(httpCode);
      return;
    }
    // Serial.print ("Payload Players : ");
    // Serial.println(payload);
    // DynamicJsonDocument doc(2048);
    StaticJsonDocument<2500> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print("deserializeJson update players() failed: ");
      Serial.println(error.c_str());
      return;
    }
    http.end();
    for (int i = 0; i < number_of_players; i++)
    {
      PlayerName[i] = doc["result"]["players_loop"][i]["name"];
      PlayerId[i] = doc["result"]["players_loop"][i]["playerid"];
      // Serial.println("Player No. " + String(i) + " Name: " + PlayerName[i] + " Id: " + PlayerId[i]);
    }
  }
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

void encoder_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
  // static bool but_flag = true;
  lv_indev_state_t encoder_act;
  // int encoder_diff;
  // int last_count = 0;
  int btn = 0;
  int count = (int)(encoder.getDirection());

  if (digitalRead(32) == 0) // encoder button
  {
    sleep_btn = 0;
    Get_Number_of_Players();
    Update_Players();
    btn++;
    encoder_act = LV_INDEV_STATE_PR; // pressed
    if (btn + focus_player < number_of_players)
    {
      focus_player = btn + focus_player;
    }
    else
    {
      focus_player = 0;
    }
    lmsPlayer = PlayerId[focus_player];
    displayPlayer = PlayerName[focus_player];
    Check_Regular();
    preferences.putString("last_player_id", PlayerId[focus_player]);
    // Serial.println(PlayerId[focus_player]);
  }
  else
  {
    encoder_act = LV_INDEV_STATE_REL; // release
  }
  // Serial.println((int)(encoder.getDirection()));
  if (count != 0)
  {
    sleep_btn = 0;

    volumen_lvgl = min<int>(max<int>(volumen_lvgl + count, 0), 100);
    lv_arc_set_value(ui_Arc1, volumen_lvgl);
    lv_label_set_text_fmt(ui_Label1, "%02d", volumen_lvgl);

    char buf[10];
    int i = volumen_lvgl;
    char *lmsVolume = "[\"mixer\",\"volume\",\"%i\"]";
    sprintf(buf, lmsVolume, i);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + buf + "]}";
    SendCommand(url, lmsjson);
  }

  // if ((digitalRead(26) == 0) && but_flag) // encoder turn left
  // {
  //   sleep_btn = 0;
  //   encoder_diff--;

  //   volumen_lvgl = min<int>(max<int>(volumen_lvgl + encoder_diff, 0), 100);
  //   lv_arc_set_value(ui_Arc1, volumen_lvgl);
  //   lv_label_set_text_fmt(ui_Label1, "%02d", volumen_lvgl);

  //   char buf[10];
  //   int i = volumen_lvgl;
  //   char *lmsVolume = "[\"mixer\",\"volume\",\"%i\"]";
  //   sprintf(buf, lmsVolume, i);
  //   String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + buf + "]}";
  //   SendCommand(url, lmsjson);
  //   but_flag = false;
  // }
  // else if ((digitalRead(25) == 0) && but_flag) // encoder turn right
  // {
  //   sleep_btn = 0;
  //   encoder_diff++;

  //   volumen_lvgl = min<int>(max<int>(volumen_lvgl + encoder_diff, 0), 100);
  //   lv_arc_set_value(ui_Arc1, volumen_lvgl);
  //   lv_label_set_text_fmt(ui_Label1, "%02d", volumen_lvgl);
  //   char buf[10];
  //   int i = volumen_lvgl;
  //   char *lmsVolume = "[\"mixer\",\"volume\",\"%i\"]";
  //   sprintf(buf, lmsVolume, i);
  //   String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + buf + "]}";
  //   SendCommand(url, lmsjson);
  //   but_flag = false;
  // }
  // else
  // {
  //   but_flag = true;
  // }

  // data->enc_diff = encoder_diff;
  data->state = encoder_act;
  // encoder_diff = 0;
  btn = 0;
  /*Return `false` because we are not buffering and no more data to read*/
  // return false;
}

void init_lv_group()
{
  lv_group = lv_group_create();
  lv_group_set_default(lv_group);
  lv_indev_t *cur_drv = NULL;
  for (;;)
  {
    cur_drv = lv_indev_get_next(cur_drv);
    if (!cur_drv)
    {
      break;
    }
    if (cur_drv->driver->type == LV_INDEV_TYPE_ENCODER)
    {
      Serial.println("Encoder linked to group");
      lv_indev_set_group(cur_drv, lv_group);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  // setCpuFrequencyMhz(240);
  // pinMode(26, INPUT_PULLUP);
  // pinMode(25, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);
  lv_init();
  // delay(500);

  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * TFT_WIDTH * 6, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, TFT_WIDTH * 6);
  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = TFT_WIDTH;
  disp_drv.ver_res = TFT_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* Initialize the input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_ENCODER;
  indev_drv.read_cb = encoder_read;
  lv_indev_drv_register(&indev_drv);
  init_lv_group();

  ui_init();

  tft.fillScreen(TFT_BLACK);
  tft.begin();
  // lv_obj_clean(lv_scr_act());
  // tft.fillScreen(TFT_BLACK);

  // wm.resetSettings();
  wm.setAPCallback(configModeCallback);
  if (!wm.autoConnect(autoConnectSSID, autoConnectPW))
  {
    lv_disp_load_scr(ui_Screen3);
    lv_label_set_text(ui_Label13, "Wi-Fi Setting Failed!");
    while (1)
    {
      lv_task_handler();
      delay(10000);
      lv_obj_clean(lv_scr_act());
      delay(100);
      ESP.restart();
    }
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("Connected!");
  }

  // lv_obj_t *labelServerStatus = lv_label_create(lv_scr_act());
  // lv_label_set_text(labelServerStatus, "LMS Server Not Found!");
  // lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0xffffff), LV_PART_MAIN);
  // lv_obj_align(labelServerStatus, LV_ALIGN_CENTER, 0, 0);
  // lv_obj_set_style_text_font(labelServerStatus, &ui_font_fontHEI18, LV_PART_MAIN | LV_STATE_DEFAULT);
  while (!serverDiscovery())
  {
    Serial.print(".");
    delay(500);
    discoveryAttemp++;
    if (discoveryAttemp > 10)
    {
      lv_disp_load_scr(ui_Screen3);
      lv_label_set_text(ui_Label13, "LMS Server Not Found!");
      lv_task_handler();
    }
  }
  // lv_label_set_text(labelServerStatus, "");
  url = "http://" + lmsServer.ip + ":" + lmsServer.port + "/jsonrpc.js";

  preferences.begin("server_info", false);                  // opens storage space
  lmsPlayer = preferences.getString("last_player_id", "0"); // check if previous player

  if (lmsPlayer == "0")
  {
    Serial.println("no player");
    is_there_player = false;
    focus_player = 0;
    Get_Number_of_Players();
    Update_Players();
    // lv_label_set_text(ui_Label2, PlayerName[focus_player]);
    // Check_Regular();
    lmsPlayer = PlayerId[focus_player];

    // lv_disp_load_scr(ui_ScreenNavigate);// loads inicial navigate screen
    // lv_list_screen_player();//
  }
  else
  {
    // Serial.print("Previous Player : "); // there is previous player in preferences
    // Serial.println(lmsPlayer);
    Get_Number_of_Players();
    Update_Players();

    for (int i = 0; i < number_of_players; i++)
    {
      // PlayerName[i] = doc["result"]["players_loop"][i]["name"];
      // PlayerId[i] = doc["result"]["players_loop"][i]["playerid"];
      // Serial.println ("Player N " + String(i) + " -Name : " + PlayerName[i] + " -Id : " + PlayerId[i]);
      if (lmsPlayer.equals(PlayerId[i]))
      {
        is_there_player = true;
        Serial.print("Previous player found!");
        focus_player = i;
        // Serial.println(PlayerName[focus_player]);
        // Serial.println(PlayerId[focus_player]);
        lmsPlayer = PlayerId[i];
        displayPlayer = PlayerName[i];
        // Serial.println(focus_player);
        // Serial.println(PlayerId[focus_player]);
        // previous_player = PlayerId[i];
        lv_label_set_text(ui_Label2, PlayerName[i]);
        // lv_disp_load_scr(ui_Screen1);
        // Check_Regular();
      }
    }
    if (!is_there_player)
    {
      Serial.println("Previous player not present, select player");
      // opens select player screen
      is_there_player = false;
      focus_player = 0;
      Get_Number_of_Players();
      Update_Players();
      lmsPlayer = PlayerId[focus_player];
      displayPlayer = PlayerName[focus_player];
      // lv_label_set_text(ui_Label2, PlayerName[focus_player]);
      // Check_Regular();
      // lmsPlayer = PlayerId[focus_player];
    }
  }

  PlayerName[focus_player] = displayPlayer.c_str();
  PlayerId[focus_player] = lmsPlayer.c_str();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, 0);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0);
  // esp_sleep_enable_ext1_wakeup(BIT64(GPIO_NUM_26) | BIT64(GPIO_NUM_25) | BIT64(GPIO_NUM_32), ESP_EXT1_WAKEUP_ANY_LOW);
  // Serial.println(PlayerName[focus_player]);

  lv_disp_load_scr(ui_Screen1);
  // tft.fillScreen(TFT_BLACK);
  Serial.println("Done");
}

void loop()
{
  // static int pos = 0;
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    Check_Regular();
    sleep_btn = sleep_btn + 1;
    if (sleep_btn >= 10 && deepSleep == true)
    {
      Serial.println("sleep");
      free(disp_draw_buf);
      esp_deep_sleep_start();
    }
  }
  lv_timer_handler_run_in_period(5);

  encoder.tick();
}