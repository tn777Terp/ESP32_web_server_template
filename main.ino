
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

#include "config.h"
#include "html.h"

class mutex_t {
  public:
    mutex_t(){
      time    = xSemaphoreCreateMutex();
      buttons = xSemaphoreCreateMutex();
      pwm     = xSemaphoreCreateMutex();
    }

    /*ADD MORE MUTEX DOWN HERE*/
    SemaphoreHandle_t time;       // for hours/minutes/seconds
    SemaphoreHandle_t buttons;
    SemaphoreHandle_t pwm;
};

typedef struct singleton_t{
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  uint8_t  buttons;
  uint16_t pwm;
} singleton_t;

typedef struct example_t{
  int     int_val;
  float   float_val;
  char    string_val[32];
  
  int     arr_val[3];
} example_t;

singleton_t global_data = {
  0, 0 , 0,               // hours, minutes, seconds
  0, 0                    // buttons, pwm
};

example_t example_data = {
  123, 45.67, "Hello",    // int_val, float_val, string_val
  {8,9,0}                 // arr_val
};

static mutex_t        mutex;
static AsyncWebServer server(80);
static TaskHandle_t   task_clock;

// #########################################################################
// MAIN SETUP
// #########################################################################
void setup() {
  Serial.begin(9600);

  // Initiating internal LED
  pinMode(INTERNAL_LED_PIN, OUTPUT);
  digitalWrite(INTERNAL_LED_PIN, 0);


  // Initiating external pwm pin
  ledcSetup(PWM_CHANNEL_0, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(EXTERNAL_PWM_PIN, PWM_CHANNEL_0);
  ledcWrite(PWM_CHANNEL_0, 0);


  // Initiate Wifi server
  WiFi.begin(LOCAL_SSID, LOCAL_PASS);
  Serial.printf("Connecting to WiFi...\n");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString());
  digitalWrite(INTERNAL_LED_PIN, 1);


  // Initiate Timer Task
  xTaskCreatePinnedToCore(
    task_clock_main,        /* pvTaskCode    */
    "clock_timer",          /* pcName        */
    1024,                   /* usStackDepth  */
    NULL,                   /* pvParameters  */
    1,                      /* uxPriority    */
    &task_clock,            /* pxCreatedTask */
    0);                     /* xCoreID       */


  // Listens for new client root request ("/") and send HTML to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MAIN_HTML);
  });

  // Listen for GET request from client for syncing
  server.addHandler(new AsyncCallbackJsonWebHandler("/sync", sync_data));

  // Listens for GET request from client to send data to client
  // server.on("/m2s_data", HTTP_GET, send_data);
  server.addHandler(new AsyncCallbackJsonWebHandler("/m2s_data", m_send_data));

  // Listens for POST request from client to receive data from client
  server.addHandler(new AsyncCallbackJsonWebHandler("/s2m_data", m_recv_data));

  server.begin();
}




// #########################################################################
// MAIN LOOP and TASKs
// #########################################################################
void loop(){
  /* Intentionally left empty */
  /* loop() runs on core1     */
}

/* 1 second period timer task */
void task_clock_main(void *pvParameters){
  static uint8_t internal_led_state = 1;
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1s delay
  xLastWakeTime = xTaskGetTickCount();

  while(1){
    if(xSemaphoreTake(mutex.time, 0) == pdTRUE){
      digitalWrite(INTERNAL_LED_PIN, internal_led_state);
      internal_led_state = !internal_led_state;
      
      global_data.seconds++;
      if(global_data.seconds >= 60){
        global_data.minutes++;
        global_data.seconds = 0;
      }
      if(global_data.minutes >= 60){
        global_data.hours++;
        global_data.minutes = 0;
      }
      if(global_data.hours >= 24){
        global_data.hours   = 0;
      }

      xSemaphoreGive(mutex.time);
    }
    
    // Wait for the next cycle.
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// #########################################################################
// TEMP FUNCTIONS. To be moved to separate files later
// #########################################################################
void sync_data(AsyncWebServerRequest *request, JsonVariant &json) {
  // Checks if request's body contains a json object
  if(!json.is<JsonObject>()) { // Respond to client
    request->send(400, "application/json", "{\"sync_data\":\"Not an json object\"}");
    return;
  }

  JsonObject client_data = json.as<JsonObject>();

  // Check if client data is same as server. 
  StaticJsonDocument<512> m2s_data;
  bool is_synced = true;

  if(client_data["hr"].as<uint8_t>() != global_data.hours){
    m2s_data["hr"] = global_data.hours;
    is_synced = false;
  }
  if(client_data["min"].as<uint8_t>() != global_data.minutes){
    m2s_data["min"] = global_data.minutes;
    is_synced = false;
  }
  if(client_data["btn"].as<uint16_t>() != (uint16_t)global_data.buttons){ // casting to 16-bit to see sign bit
    m2s_data["btn"] = global_data.buttons;
    is_synced = false;
  }
  if(client_data["pwm"].as<uint16_t>() != global_data.pwm){
    if(xSemaphoreTake(mutex.pwm, 0) == pdTRUE){
      m2s_data["pwm"] = global_data.pwm;  // pwm > 1-byte in size. so we need a mutex to make sure 
      xSemaphoreGive(mutex.pwm);          // both bytes are read without race condition.
    } is_synced = false;
  }

  // Repond with empty json if data is synced
  if(is_synced){
    request->send(200, "application/json", "{}");
    return;
  }

  // Respond with data that differs between server-client
  String jsonString;
  serializeJson(m2s_data, jsonString);
  request->send(200, "application/json", jsonString);
}



void m_send_data(AsyncWebServerRequest *request, JsonVariant &json) {
  if(!json.is<JsonObject>()) { // Respond to client
    request->send(400, "application/json", "{\"send_data\":\"Invalid Request\"}");
    return;
  }

  // Examples on how to set values for datas to be sent to client
  // You can add 
  StaticJsonDocument<256> m2s_data;
  m2s_data["intVal"]    = example_data.int_val;
  m2s_data["floatVal"]  = example_data.float_val;
  m2s_data["stringVal"] = example_data.string_val;

  JsonArray m2s_data_arrval = m2s_data.createNestedArray("arrayVal"); // Creates m2s_data["arrayVal"] 
  copyArray(example_data.arr_val, m2s_data_arrval);                   // m2s_data["arrayVal"] = example_data.arr_val;
  

  // Send json to client
  String jsonString;
  serializeJson(m2s_data, jsonString);
  request->send(200, "application/json", jsonString);
}


void m_recv_data(AsyncWebServerRequest *request, JsonVariant &json) {
  if(!json.is<JsonObject>()) {
    request->send(400, "application/json", "{\"recv_data\":\"Not an json object\"}");
    return;
  }

  JsonObject client_data = json.as<JsonObject>();

  // Print the raw string
  String jsonString;
  if(serializeJson(client_data, jsonString))
    Serial.println(jsonString);
  else
    Serial.println("serializeJson() failed");

  // You can add an early return conditions to prevent checking unecessary fields

  // Handling time data
  if(client_data.containsKey("g_time")){
    update_global_time(client_data);
    request->send(200, "application/json", "{\"server\":\"g_time recieved\"}");
    return;
  }

  // Handling btn0-btn7 data
  if(client_data.containsKey("btn")){
    update_global_button(client_data["btn"].as<uint8_t>());
    request->send(200, "application/json", "{\"server\":\"btn recieved\"}");
    return;
  }

  // Handling slider value
  if(client_data.containsKey("pwm")){
    update_global_pwm(client_data["pwm"].as<uint16_t>());
    request->send(200, "application/json", "{\"server\":\"pwm recieved\"}");
    return;
  }

  // Handling example_data coming from client
  if(client_data.containsKey("myInt"))
    Serial.printf("myInt = %d\n",  client_data["myInt"].as<int>());    
  if(client_data.containsKey("myFloat"))
    Serial.printf("myFloat = %f\n",  client_data["myFloat"].as<float>());
  if(client_data.containsKey("myString"))
    Serial.printf("myString = %s\n",  client_data["myString"].as<const char*>());  // client_data["myString"] return value is const char*
  if(client_data.containsKey("sdata")){
    JsonArray sdata = client_data["sdata"].as<JsonArray>();
    Serial.println("sdata:");
    Serial.println(sdata[0].as<int>());
    Serial.println(sdata[1].as<float>());
    Serial.println(sdata[2].as<const char*>());
  }

  // Respond to client
  request->send(200, "application/json", "{\"server\":\"json recieved\"}");
}


// #########################################################################
// HELPER FUNCTIONS
// #########################################################################
void update_global_time(JsonObject client_data){
  if(xSemaphoreTake(mutex.time, portMAX_DELAY) == pdTRUE){
    global_data.hours   = client_data["hr"].as<uint8_t>();
    global_data.minutes = client_data["min"].as<uint8_t>();
    global_data.seconds = client_data["sec"].as<uint8_t>();
    // Serial.printf("t: %hhu, %hhu, %hhu\n", global_data.hours, global_data.minutes, global_data.seconds);
    xSemaphoreGive(mutex.time);
  }
}

void update_global_button(uint8_t flag_bit){
  // Serial.printf("flag: %02hX\n", flag_bit);
  /* Sets flag bit for buttons */
  if(xSemaphoreTake(mutex.buttons, 0) == pdTRUE){
    global_data.buttons ^= flag_bit;    // Toggle flag bit
    xSemaphoreGive(mutex.buttons);
  }
}

void update_global_pwm(uint16_t pwm_val){
  if(xSemaphoreTake(mutex.pwm, 0) == pdTRUE){
    global_data.pwm = pwm_val;  
    ledcWrite(PWM_CHANNEL_0, global_data.pwm);
    xSemaphoreGive(mutex.pwm);
  }
}