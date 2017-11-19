//  File: SerialCamera_WeiLun
//  Author: Wei-Lun Tsai
//  Description: Camera design
//  Updates:
//    2016.11.01    Basic function
//    2016.11.13    

#include <arduino.h>
#include <SD.h>
#include <SPI.h>

#define PIC_PKT_LEN           128              //data length of each read, dont set this too big because ram is limited
#define PIC_JPEG_RESO_LOW     0x01             // JPEG resolution: 80*64
#define PIC_JPEG_RESO_QQVGA   0x03             // JPEG resolution: 160*128
#define PIC_JPEG_RESO_QVGA    0x05             // JPEG resolution: 320*240
#define PIC_JPEG_RESO_VGA     0x07             // JPEG resolution: 640*480
#define PIC_COLOER_GRAY       0x03             // 8-bits Gray-Scale
#define PIC_COLOER_JPEG       0x07             // JPEG

#define CAM_ADDR              0                // ??
#define CAM_SERIAL            Serial

#define PIC_JPEG_RESO         PIC_JPEG_RESO_VGA
#define PIC_COLOER            PIC_COLOER_JPEG

File myFile;

const byte cameraAddr = (CAM_ADDR << 5);  // addr
const int buttonPin = A5;                 // the number of the pushbutton pin
unsigned long picTotalLen = 0;            // picture length
//const int Camera_CS = 17;                 // Camera CS for LinkIt 7688 Duo
const int Camera_CS = 10;                 // Camera CS for Arduino
char picName[] = "pic******.jpg";
int picNameNum = 0;

/*********************************************************************/
void setup()
{
    Serial.begin(9600);
    pinMode(buttonPin, INPUT);    // initialize the pushbutton pin as an input
    pinMode(Camera_CS, OUTPUT);          // CS pin of SD Card Shield
    
    SD_init();
    CAM_sync();
}

void SD_init()
{
    delay(2000);     // wait for serial port stable before printing log
    Serial.print("Initializing SD card....");
 
    while(!SD.begin(Camera_CS)){
        Serial.println("failed");
        delay(5000);
        Serial.print("Initializing SD card....");
    }
    Serial.println("success!");
}
/*********************************************************************/
void loop()
{
    int n=0;
    while(1){
        Serial.println("\r\nPress the button to take a picture");
        while (digitalRead(buttonPin) == LOW);
        if(digitalRead(buttonPin) == HIGH){
            delay(20);                               //Debounce
            if (digitalRead(buttonPin) == HIGH)
            {
                Serial.println("\r\nStart taking picture...");
                delay(200);
                if(n == 0) CAM_init();
                Capture();
                GetData();
            }
            Serial.print("\r\nTaking pictures success ,number : ");
            Serial.println(n);
            n++ ;
        }
    }
}
/*********************************************************************/
void clearRxBuf()
{
    while (Serial.available())
    {
        Serial.read();
    }
}
/*********************************************************************/
void sendCmd(char cmd[], int cmd_len)
{
    for (char i = 0; i < cmd_len; i++) Serial.print(cmd[i]);
}
/*********************************************************************/
void CAM_sync()
{
    char cmd[] = {0xaa,0x0d|cameraAddr,0x00,0x00,0x00,0x00} ;
    unsigned char resp[6];

    Serial.println("Sync with camera...");
  
    Serial.setTimeout(500);
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd,6);
        if (Serial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x0d && resp[4] == 0 && resp[5] == 0) {
            Serial.println("\nACK received");
            if (Serial.readBytes((char *)resp, 6) != 6) continue;
            if (resp[0] == 0xaa && resp[1] == (0x0d | cameraAddr) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) {
              Serial.println("SYNC received");
              break;
            }
        }
    }
    cmd[1] = 0x0e | cameraAddr;
    cmd[2] = 0x0d;
    Serial.println("Start sending ACK...");
    sendCmd(cmd, 6);
    Serial.println("done\nCamera synchronization success!");
}
/*********************************************************************/
void CAM_init()
{
    char cmd[] = { 0xaa, 0x01 | cameraAddr, 0x00, PIC_COLOER, 0x00,  PIC_JPEG_RESO};
    unsigned char resp[6];

    Serial.print("Start inititialing camera...");
    Serial.setTimeout(100);
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (Serial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x01 && resp[4] == 0 && resp[5] == 0) {
          Serial.print("\nsuccessful!");
          break;
        }
    }
}

void Capture()
{
    char cmd[] = { 0xaa, 0x06 | cameraAddr, 0x08, PIC_PKT_LEN & 0xff, (PIC_PKT_LEN>>8) & 0xff ,0}; // set package size
    unsigned char resp[6];

    Serial.setTimeout(100);
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (Serial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x06 && resp[4] == 0 && resp[5] == 0) break;
    }

    // set snapshot - compressed picture
    cmd[1] = 0x05 | cameraAddr;
    cmd[2] = 0;
    cmd[3] = 0;
    cmd[4] = 0;
    cmd[5] = 0;
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (Serial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x05 && resp[4] == 0 && resp[5] == 0) break;
    }

    // get snapshot picture
    cmd[1] = 0x04 | cameraAddr;
    cmd[2] = 0x1;
    while (1)
    {
        clearRxBuf();
        sendCmd(cmd, 6);
        if (Serial.readBytes((char *)resp, 6) != 6) continue;
        if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x04 && resp[4] == 0 && resp[5] == 0)
        {
            Serial.setTimeout(1000);
            if (Serial.readBytes((char *)resp, 6) != 6)
            {
                continue;
            }
            if (resp[0] == 0xaa && resp[1] == (0x0a | cameraAddr) && resp[2] == 0x01)
            {
                picTotalLen = (resp[3]) | (resp[4] << 8) | (resp[5] << 16);
                Serial.print("\nsnap shot data get!\npicTotalLen: ");
                Serial.println(picTotalLen);
                break;
            }
        }
    }

}
/*********************************************************************/
void GetData()
{
    Serial.print("\n\nSaving picture...\n\n\n");
    unsigned int pktCnt = (picTotalLen) / (PIC_PKT_LEN - 6);
    if ((picTotalLen % (PIC_PKT_LEN-6)) != 0) pktCnt += 1;

    char cmd[] = { 0xaa, 0x0e | cameraAddr, 0x00, 0x00, 0x00, 0x00 };
    unsigned char pkt[PIC_PKT_LEN];

    int2str(picNameNum, picName);
    while(SD.exists(picName)){
      picNameNum++;
      int2str(picNameNum, picName);
    }

    myFile = SD.open(picName, FILE_WRITE);
    if(!myFile){
        Serial.println("myFile open fail...");
    }else{
        Serial.setTimeout(1000);
        for (unsigned int i = 0; i < pktCnt; i++)
        {
            cmd[4] = i & 0xff;
            cmd[5] = (i >> 8) & 0xff;

            int retry_cnt = 0;
            retry:
            delay(10);
            clearRxBuf();
            sendCmd(cmd, 6);
            uint16_t cnt = Serial.readBytes((char *)pkt, PIC_PKT_LEN);

            unsigned char sum = 0;
            for (int y = 0; y < cnt - 2; y++)
            {
                sum += pkt[y];
            }
            if (sum != pkt[cnt-2])
            {
                if (++retry_cnt < 100) goto retry;
                else break;
            }

            myFile.write((const uint8_t *)&pkt[4], cnt-6);
            //if (cnt != PIC_PKT_LEN) break;
        }
        cmd[4] = 0xf0;
        cmd[5] = 0xf0;
        sendCmd(cmd, 6);
    }
    myFile.close();
    Serial.print("\n\nPIC name: ");
    Serial.print(picName);
    picNameNum ++;
}

void int2str(int i, char *s) {
  sprintf(s,"pic%d.jpg",i);
}
