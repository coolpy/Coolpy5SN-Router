#include <SoftwareSerial.h>
SoftwareSerial mySerial(7, 8); // RX, TX
//UserKey用户密钥(必改项)
char ukey[] = "986c17fd-ca85-4b9f-a2ab-1f4748f7effa";
//Hub ID(必改项)
char hub[] = "5";
//Node ID(必改项)
char node[] = "11";

uint16_t MsgId;
void setup() {
  mySerial.begin(9600);
  randomSeed(analogRead(0));
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  //reg();
  sub();
}

void reg() {
  //启动时reg当前节点及mqtt topic的注册
  int ukey_len = strlen(ukey);
  int hub_len = strlen(hub);
  int node_len = strlen(node);
  int ptlen = 8 + ukey_len + hub_len + node_len;
  byte cbuf[ptlen];
  byte *p = cbuf;
  *p++ = ptlen;
  *p++ = 0x0A;
  uint16_t topicid = 0;
  *((uint16_t *)p) = topicid; p += 2;
  MsgId = random(1, 65535);
  *((uint16_t *)p) = MsgId; p += 2;
  memcpy(p, ukey, ukey_len);
  *(p += ukey_len)++ = ':';
  memcpy(p, hub, hub_len);
  *(p += hub_len)++ = ':';
  memcpy(p, node, node_len);
  Serial.write(cbuf, *cbuf);
}

byte *buf;
byte *cluser = NULL , *end_point;
uint16_t len = 0;
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
          snReader(buf);
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

uint16_t inline bswap(const uint16_t iValue) {
  return (iValue << 8) | (iValue >> 8);
}

struct Header {
  uint8_t length;
  uint8_t msgType;
};

struct Regack {
  Header header;
  uint16_t topicId;
  uint16_t msgId;
  uint8_t returnCode;
};

union Flags {
  uint8_t byte;
  struct
  {
    bool dup             : 1;
    uint8_t qos          : 2;
    bool retain          : 1;
    bool will            : 1;
    bool cleanSession    : 1;
    uint8_t topicIdType  : 2;
  } bits;
};

struct Suback {
  Header header;
  Flags flags;
  uint16_t topicId;
  uint16_t msgId;
  uint8_t returnCode;

  uint16_t getTopicId() {
    return bswap(topicId);
  }

  uint16_t getMsgId() {
    return bswap(msgId);
  }
};

void snReader(byte *sn) {
//  if (sn[1] == 0x0B) {//regack
//    Regack &p = *(Regack *)sn;
//    if (p.msgId == MsgId) {
//      if (p.returnCode == 0x00) {
//        sub();
//      } else {
//        mySerial.write("access deny");
//      }
//    }
//  } 
  if (sn[1] == 0x13) {//suback
    Suback &p = *(Suback *)sn;
    mySerial.write(p.getTopicId());
  }
}

void sub() {
  int ukey_len = strlen(ukey);
  int hub_len = strlen(hub);
  int node_len = strlen(node);
  int ptlen = 7 + ukey_len + hub_len + node_len;
  byte cbuf[ptlen];
  byte *p = cbuf;
  *p++ = ptlen;
  *p++ = 0x12;
  byte flags = 0;
  flags = bitWrite(flags, 7, 1);
  flags = bitWrite(flags, 6, 0);
  flags = bitWrite(flags, 5, 0);
  flags = bitWrite(flags, 4, 0);
  flags = bitWrite(flags, 3, 0);
  flags = bitWrite(flags, 2, 0);
  flags = bitWrite(flags, 1, 0);
  flags = bitWrite(flags, 0, 0);
  *p++ = flags;
  MsgId = random(1, 65535);
  *((uint16_t *)p) = MsgId; p += 2;
  memcpy(p, ukey, ukey_len);
  *(p += ukey_len)++ = ':';
  memcpy(p, hub, hub_len);
  *(p += hub_len)++ = ':';
  memcpy(p, node, node_len);
  Serial.write(cbuf, *cbuf);
}


