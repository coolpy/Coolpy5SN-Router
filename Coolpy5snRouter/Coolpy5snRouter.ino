#include <ESP8266WiFi.h>
#include <ESP8266Client.h>

const char* iClientId     = "Client01";

//------------------------------------------
const char* ssid     = "CMCC_2.4G";
const char* password = "zrr83320938";


// create a UDP client
ESP8266Client client("192.168.2.61", 1884, UDP);

//------------------------------------------
// CB forward declarations
void onDataCb(ESP8266Client& client, char *data, unsigned short length);
void onConnectCb(ESP8266Client& client);
void onDisconnectCb();
void onReconnectCb(ESP8266Client& client, sint8 err);

//------------------------------------------
void setup() {
  Serial.begin(9600);
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  while (!client.connect()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connect Coolpy5SN Ready");

  //注册到sn网关，此步完成即下属所有子网节点直接开始注册、pub、sub操作即可
  char clientId[0];
  memcpy(clientId, iClientId, strlen(iClientId));
  int ptlen = strlen(iClientId) + 6;
  byte cbuf[ptlen];
  cbuf[0] = byte(ptlen);
  cbuf[1] = 0x04;
  cbuf[2] = 0x0c;
  cbuf[3] = 0x01;
  cbuf[4] = 0x00;
  cbuf[5] = 0x1e;
  for (int i = 0; i < strlen(iClientId); i++) {
    cbuf[i + 6] = byte(clientId[i]);
  }
  if (client.isConnected()) {
    sint8 res = client.send(cbuf, ptlen);
    if (res != ESPCONN_OK) {
      Serial.print("error sending: ");
      Serial.println(res);
    }
  } else {
    Serial.println("err udp");
  }

  // set callback functions
  client.onData(onDataCb);
}

byte *buf;
byte *cluser = NULL , *end_point;
uint16 len = 0;
byte buf_temp;
int is_first = 1;
int is_start = 0;

void loop() {
  int bufsize = Serial.available();
  if (bufsize > 0) {
    for (int i = 0; i < bufsize; i++) {
      byte c = Serial.read();
      if (cluser == NULL) {
        if (len == 0) {
          is_start = 0;
          is_first = 1;
          len = c;
        }
        if (len == 0x01) {
          if (is_start) {
            if (is_first) {
              buf_temp = c;
              is_first = 0;
            } else {
              len = buf_temp;
              len <<= 8;
              len += c;
              cluser = buf = (byte*) malloc(sizeof(byte) * len);
              end_point = buf + len;
              *cluser++ = 0x01;
              *cluser++ = buf_temp;
              *cluser++ = c;
            }
          }
        } else {
          cluser = buf = (byte*) malloc(sizeof(byte) * len);
          end_point = buf + len;
          *cluser++ = c;
        }
      } else {
        *cluser++ = c;
        if (cluser == end_point) {
          if (client.isConnected()) {
            sint8 res = client.send(buf, end_point - buf);
            if (res != ESPCONN_OK) {
              Serial.print("error sending: ");
              Serial.println(res);
            }
          } else {
            Serial.println("err udp");
          }
          free(buf);
          len = 0;
          cluser = NULL;
        }
      }
      is_start = 1;
    }
  }
  delay(1);
}

//------------------------
// general callbacks
void onDataCb(ESP8266Client& client, char *data, unsigned short length)
{
  Serial.write(data, length);
}
