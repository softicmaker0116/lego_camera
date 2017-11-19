//  File: SerialCamera_WeiLun
//  Author: Wei-Lun Tsai
//  Description: Camera design
//  Updates:
//    2016.11.01    Basic function
//    2016.11.13    

#include <arduino.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

#define PIC_PKT_LEN           128              //data length of each read, dont set this too big because ram is limited
#define PIC_JPEG_RESO_LOW     0x01             // JPEG resolution: 80*64
#define PIC_JPEG_RESO_QQVGA   0x03             // JPEG resolution: 160*128
#define PIC_JPEG_RESO_QVGA    0x05             // JPEG resolution: 320*240
#define PIC_JPEG_RESO_VGA     0x07             // JPEG resolution: 640*480
#define PIC_COLOER_GRAY       0x03             // 8-bits Gray-Scale
#define PIC_COLOER_JPEG       0x07             // JPEG

#define CAM_ADDR       0
#define CAM_SERIAL     softSerial

#define PIC_JPEG_RESO         PIC_JPEG_RESO_VGA
#define PIC_COLOER            PIC_COLOER_JPEG

File myFile;
SoftwareSerial softSerial(2, 3);  //rx,tx for UART

const byte cameraAddr = (CAM_ADDR << 5);  // addr
const int buttonPin = A5;                 // the number of the pushbutton pin
unsigned long picTotalLen = 0;            // picture length
const int Camera_CS = 10;                 // Camera CS for Arduino
//const int Camera_CS = 17;                 // Camera CS for LinkIt 7688
char picName[] = "pic******.jpg";
int picNameNum = 0;

/*********************************************************************/
void setup()
{
    Serial.begin(9600);
    CAM_SERIAL.begin(9600);       //cant be faster than 9600, maybe difference with diff board.
  
    pinMode(buttonPin, INPUT);    // initialize the pushbutton pin as an input
    pinMode(Camera_CS, OUTPUT);          // CS pin of SD Card Shield
   
    delay(3000);  // uart log failed if not add this line...
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
    Serial.println("success!\n");
}
/*********************************************************************/
void loop()
{
    int n=0;
    while(1){
        Serial.println("\nPress the button to take a picture");
        while (digitalRead(buttonPin) == LOW);
        if(digitalRead(buttonPin) == HIGH){
            delay(20);                               //Debounce
            if (digitalRead(buttonPin) == HIGH)
            {
                Serial.println("Start taking picture...\n");
                delay(200);
                if(n == 0) CAM_init();
                CAM_CapSetting();
                CAM_Capture();
            }
            Serial.print("\r\nProcess completed ,number : ");
            Serial.println(n,"\n\n\n");
            n++ ;
        }
    }
}
/*********************************************************************/
void clearRxBuf()
{
  while (CAM_SERIAL.available()) 
  {
    CAM_SERIAL.read(); 
  }
}
/*********************************************************************/
void sendCmd(char cmd[], int cmd_len)
{
  Serial.print("[M->>S] "); // For debug: debug_M2S()
  for (char i = 0; i < cmd_len; i++) {
    CAM_SERIAL.write(cmd[i]);
    Serial.print((unsigned char)cmd[i],HEX); // For debug: debug_M2S()
    Serial.print("-"); // For debug: debug_M2S()
  }
  Serial.println(""); // For debug: debug_M2S()
}
/*********************************************************************/
void debug_S2M(unsigned char resp[]){ // print cmds from slave to master
    Serial.print("[M<<-S] ");
    for(char i = 0; i < 6; i++){
      Serial.print((unsigned char)resp[i],HEX);
      Serial.print("-");
    }
    Serial.println("");
}
/*********************************************************************/
int readBytes(char *dest, int len, unsigned int timeout)
{
  int read_len = 0;
  unsigned long t = millis();
  while (read_len < len)
  {
    while (CAM_SERIAL.available()<1)
    {
      if ((millis() - t) > timeout)
      {
        Serial.println("[S->>M]\tTimeout reach");
        return read_len;
      }
    }
    *(dest+read_len) = CAM_SERIAL.read();
    //Serial.print((unsigned char) *(dest+read_len),HEX);
    read_len++;
  }
  //Serial.println();
  return read_len;
}
/*********************************************************************/
void CAM_sync()
{
    char cmd[] = {0xaa,0x0d|cameraAddr,0x00,0x00,0x00,0x00} ;
    unsigned char resp[6];

    Serial.println("Sync with camera...");
  
  while (1) 
  {
    //clearRxBuf();
    sendCmd(cmd,6);
    
    if (readBytes((char *)resp, 6,1000) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x0d && resp[4] == 0 && resp[5] == 0) {
      //Serial.println("\nACK received");
      if (readBytes((char *)resp, 6, 500) != 6) {continue;}
      else {debug_S2M(resp);}
      
      if (resp[0] == 0xaa && resp[1] == (0x0d | cameraAddr) && resp[2] == 0 && resp[3] == 0 && resp[4] == 0 && resp[5] == 0) {
        //Serial.println("SYNC received");
        break;
      }
    }
  }  
  cmd[1] = 0x0e | cameraAddr;
  cmd[2] = 0x0d;
    //Serial.println("Start sending ACK...");
    sendCmd(cmd, 6);
    //Serial.println("done");
    Serial.println("Camera synchronization success!");
}
/*********************************************************************/
void CAM_init()
{
    char cmd[] = { 0xaa, 0x01 | cameraAddr, 0x00, PIC_COLOER, 0x00,  PIC_JPEG_RESO};
    unsigned char resp[6]; 
  
    Serial.println("Inititialing camera...");
  while (1)
  {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x01 && resp[4] == 0 && resp[5] == 0) {
      Serial.println("Camera inititialization success!\n");
      break;
    }
  }
}

void CAM_CapSetting()
{
  char cmd[] = { 0xaa, 0x06 | cameraAddr, 0x08, PIC_PKT_LEN & 0xff, (PIC_PKT_LEN>>8) & 0xff ,0}; 
  unsigned char resp[6];

  Serial.println("Capturing image...");
    
  while (1)
  {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
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
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x05 && resp[4] == 0 && resp[5] == 0) break;
  }

  
    // get snapshot picture
  cmd[1] = 0x04 | cameraAddr;
  cmd[2] = 0x1;
  while (1) 
  {
    clearRxBuf();
    sendCmd(cmd, 6);
    if (readBytes((char *)resp, 6, 100) != 6) {continue;}
    else {debug_S2M(resp);}
    
    if (resp[0] == 0xaa && resp[1] == (0x0e | cameraAddr) && resp[2] == 0x04 && resp[4] == 0 && resp[5] == 0)
    {
      if (readBytes((char *)resp, 6, 1000) != 6) {continue;}
      else {debug_S2M(resp);}
      
      if (resp[0] == 0xaa && resp[1] == (0x0a | cameraAddr) && resp[2] == 0x01)
      {
        picTotalLen = (resp[3]) | (resp[4] << 8) | (resp[5] << 16); 
        Serial.print("Snap shot data get! picTotalLen: ");
        Serial.println(picTotalLen);
        break;
      }
    }
  }
  
  Serial.println("Capturing success!\n");
}
/*********************************************************************/
void CAM_Capture()
{
  
  Serial.println("Saving picture...");
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
    for (unsigned int i = 0; i < pktCnt; i++)
    {
      cmd[4] = i & 0xff;
      cmd[5] = (i >> 8) & 0xff;
      
      int retry_cnt = 0;
    retry:
      delay(10);
      clearRxBuf(); 
      sendCmd(cmd, 6); 
      uint16_t cnt = readBytes((char *)pkt, PIC_PKT_LEN, 200);
      
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
  Serial.print("PIC name: ");
  Serial.print(picName);
  picNameNum ++;
}

void int2str(int i, char *s) {
  sprintf(s,"pic%d.jpg",i);
}
