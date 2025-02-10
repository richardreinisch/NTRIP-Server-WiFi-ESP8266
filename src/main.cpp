
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


const char* ssid = "NTRIPServer";

const char* password = "gnss";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>NTRIP Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2 {
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /* .button:hover { background-color: #0f8b8d } */
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 0.8rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
</head>
<body>
  <div class="topnav">
    <h1>NTRIP Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Data</h2>
      <p class="state">state: <span id="state">%STATE%</span></p>
      <br/><br/><br/>
      <p><button id="buttonStart" class="button">Start</button></p>
      <p><button id="buttonStop" class="button">Stop</button></p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.binaryType = 'arraybuffer';
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    /* console.log(event.data); */
    document.getElementById('state').innerHTML = event.data;
  }
  function onLoad(event) {
    initWebSocket();
    initButtons();
  }
  function initButtons() {
    document.getElementById('buttonStart').addEventListener('click', start);
    document.getElementById('buttonStop').addEventListener('click', stop);
  }
  function start() {
    websocket.send('start');
  }
  function stop() {
    websocket.send('stop');
    document.getElementById('state').innerHTML = "";
  }
</script>
</body>
</html>
)rawliteral";

String receivedLine = "";
bool startServer = false;

void notifyClients() {

  ws.binaryAll(receivedLine);

}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {

  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    
    data[len] = 0;

    if (strcmp((char*)data, "start") == 0) {
      startServer = true;
    } else if (strcmp((char*)data, "stop") == 0) {
      startServer = false;
    }

  }

}

void onEvent(AsyncWebSocket *server, 
             AsyncWebSocketClient *client, 
             AwsEventType type,
             void *arg, 
             uint8_t *data, 
             size_t len) {

  switch (type) {
    case WS_EVT_CONNECT:
      // Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      // Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }

}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  // Serial.println(var);
  return String();
}

void setup() {

  WiFi.softAP(ssid, password);
  Serial.begin(9600, SERIAL_8N1);
  // Serial.setDebugOutput(true);
  // Serial.println(F("Start"));

  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.begin();

}

void loop() {

  ws.cleanupClients();

  if (Serial.available()) {

    while (Serial.available()) {

      char c = Serial.read();
      receivedLine += c;

      if (c == '\n') {

        if (startServer && receivedLine != "") notifyClients();
        receivedLine = "";

      }

       if (!Serial.available()) delayMicroseconds(1000);

    }

  }

}
