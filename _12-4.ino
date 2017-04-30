#include <Wire.h>
#include <DS3231.h>
#include<UTFT.h>
#include<URTouch.h>
#include <SPI.h>
#include <SD.h>

#define DS3231_I2C_ADDRESS 0x68
#define ANALOGPIN        A1
#define AVGTIME          5          //avg time read volttage


#define DEBUG true 

//#define DURATIONTIME     600000   //30s  delay between two read


#define SERVERPORT       8080
#define TIMEOUT          3000     //delay between two command
#define BUADRATE         9600     //speed to transmit between Arduino UART and ESP2866
const int chipSelect = 53;
int sensorValue = 0;

float Thermistor(int Raw) //This function calculates temperature from ADC count
{
  long Resistance;
  float Resistor = 15000;
  float Temp;  // Dual-Purpose variable to save space.
  Resistance = ( Resistor * Raw / (1024 - Raw));
  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate  it 4 times later
  Temp = 1 / (0.00102119 + (0.000222468 * Temp) + (0.000000133342 * Temp * Temp * Temp));
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius
  return Temp;        // Return the Temperature
}




UTFT myGLCD(ILI9341_16,38,39,40,41);
URTouch myTouch(6,5,4,3,2);


extern uint8_t BigFont[];
extern uint8_t SmallFont[];
extern uint8_t SevenSegNumFont[];

int x, y;
char stCurrent1[20]="";
int stCurrentLen1=0;
char stLast1[20]="";

char stCurrent2[20]="";
int stCurrentLen2=0;
char stLast2[20]="";

char stCurrent3[20]="";
int stCurrentLen3=0;
char stLast3[20]="";

char stCurrent4[20]="";
int stCurrentLen4=0;
char stLast4[20]="";

char stCurrent5[20]="";
int stCurrentLen5=0;
char stLast5[20]="";



char currentPage;
void setup() {
// Initial setup
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  currentPage = '0';
  drawHomeScreen();
  Wire.begin();
  Serial.begin(9600);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}
String getStringTime()
{  
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  String rtcString="";
  String dayOfWeekStr="";
  
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  rtcString += hour;
  // convert the byte variable to a decimal number when displayed
  rtcString += ":";
  if (minute<10)
  {
    rtcString +="0";
  }
  rtcString +=minute;
  rtcString += ":";
  if (second<10)
  {
    rtcString +="0";
  }
  rtcString +=second;
  rtcString +=";";
  
  rtcString +=dayOfMonth;
  rtcString +="/";
  rtcString +=month;
  rtcString +="/";
  rtcString +=2000+year;
  rtcString +=";";
//  switch(dayOfWeek){
//  case 1:
//    dayOfWeekStr= "Sunday";
//    break;
//  case 2:
//    dayOfWeekStr= "Monday";
//    break;
//  case 3:
//    dayOfWeekStr= "Tuesday";
//    break;
//  case 4:
//    dayOfWeekStr= "Wednesday";
//    break;
//  case 5:
//    dayOfWeekStr= "Thursday";
//    break;
//  case 6:
//    dayOfWeekStr= "Friday";
//    break;
//  case 7:
//    dayOfWeekStr= "Saturday";
//    break;
//  }
//  rtcString +=dayOfWeekStr;
//
//  rtcString +="/";
  
  return rtcString;
}

//----------------------------------------
// SD Card
void writeLogFile(String fileName,String dataString){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(fileName, FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening "+ fileName);
  }
}



float getVoltage(int pinPort, int numberAvg, int delayTime ){
  float Count;
  float Voltage;
  float sum;
  for (int i = 0 ; i < numberAvg; i++)
    {
      delay (delayTime);
      Count = analogRead(pinPort); //read 0 to 5 volt analog lines
      sum = sum + Count;
      //if (DEBUG){
        //Serial.print("voltage: ");
        //Serial.println(Count);
      //}
    }
    Voltage = sum / numberAvg / 1024.0 * 5.0; ; //convert average count to voltage (0 to 5 volt input)
    return Voltage;
}


void loop() {
  if(currentPage == '0'){
    if(myTouch.dataAvailable()){
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
  //If we press Sampling Button
  if ((x>=35) && (x<=285) && (y>=42) && (y<=82)){
        drawFrame(35, 42, 285, 82);
        currentPage = '1';// 
        myGLCD.clrScr();
        drawSampling();
        drawBack();
    }

    // If we press Calibration Button
      if ((x>=35) && (x<=285) && (y>=90) && (y<=130)) {
        drawFrame(35, 90, 285, 130); 
        currentPage = '2';
        myGLCD.clrScr();
        drawCalibration();
        drawBack();
      }

    // If we press Setting Button
      if ((x>=35) && (x<=285) && (y>=140) && (y<=180)) {
        drawFrame(35, 140, 285, 180);  
        currentPage = '3';
        myGLCD.clrScr();
        //drawSetting();
        drawSetting();
        drawBack();
      }

     // If we press OFF Button
      if ((x>=35) && (x<=285) && (y>=190) && (y<=230)) {
        drawFrame(35, 190, 285, 230);
        currentPage = '4';
        myGLCD.clrScr();
        //drawHomeScreen_4();
        //drawBack();
      }                
     
}
}




  if (currentPage == '1') {    
      //drawBack();
      //getDOSensor(); // Gets distance from the sensor and this function is repeatedly called while we are at the first example in order to print the lasest results from the distance sensor
      //thoigian();
      if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
       
        // If we press the Back Button
        if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawHomeScreen(); // Draws the home screen
        }
        // If we press DO button
        if ((x>=35) && (x<=285) && (y>=42) && (y<=82))
        {
        drawFrame(35, 42, 285, 82);
        currentPage = '5';// 
        myGLCD.clrScr();
        drawBack();
        drawDOSensor();
        getDOSensor();
        }

        // If we press Temp button
        if ((x>=35) && (x<=285) && (y>=90) && (y<=130)) {
        drawFrame(35, 90, 285, 130);
        currentPage = '8';// 
        myGLCD.clrScr();
        drawBack();
        drawTemperatureSensor();
        getTempSensor();
        }

        // If we press NO3 button
        if ((x>=35) && (x<=285) && (y>=140) && (y<=180)) {
        drawFrame(35, 140, 285, 180);  
        currentPage = '9';
        myGLCD.clrScr();
        drawBack();
        drawNO3Sensor();
        getNO3Sensor(); 
      }

        
     // If we press ** Button
      if ((x>=35) && (x<=285) && (y>=190) && (y<=230)) {
        drawFrame(35, 190, 285, 230);
        currentPage = '6';
        myGLCD.clrScr();
        drawSampling_2();
      }           
        
        }
       }

// Calibration
if(currentPage =='2'){
  //getPHSensor();
  if(myTouch.dataAvailable()){
    myTouch.read();
    x=myTouch.getX();
    y=myTouch.getY();
    //Back Button
       if ((x>=10) && (x<=60) &&(y>=10) && (y<=36)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0';
          myGLCD.clrScr();
          drawHomeScreen();
        }
        // If we press DO button
        if ((x>=35) && (x<=285) && (y>=42) && (y<=82))
        {
        drawFrame(35, 42, 285, 82);
        currentPage = 'j';// 
        myGLCD.clrScr();
        drawBack();
        drawDOCalibration();
        
        }
        // If we press ** in calibration
       if ((x>=35) && (x<=285) && (y>=190) && (y<=230)) {
        drawFrame(35, 190, 285, 230);
        currentPage = 'k';
        myGLCD.clrScr();
        drawCalibration_2();
        drawBack();
      }
         
        
        }
        }
//Temp sensor
if (currentPage =='3'){

  if(myTouch.dataAvailable()){
    myTouch.read();
    x=myTouch.getX();
    y=myTouch.getY();
    //Back Button
       if ((x>=10) && (x<=60) &&(y>=10) && (y<=36)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0';
          myGLCD.clrScr();
          drawHomeScreen();
        }
        // If we press Parameters
        if ((x>=35) && (x<=285) && (y>=42) && (y<=82))
        {
        drawFrame(35, 42, 285, 82);
        currentPage = 'd';// 
        myGLCD.clrScr();
        drawBack();
        drawSamplingParameters();
        }
        // If we press wifi button
        if ((x>=35) && (x<=285) && (y>=90) && (y<=130)) {
        drawFrame(35, 90, 285, 130);
        currentPage = 'e';// 
        myGLCD.clrScr();
        drawBack();
        drawWifi();
        }
        if ((x>=35) && (x<=285) && (y>=190) && (y<=230)) {
          drawFrame(35,190,285,230);
          currentPage ='f';
          myGLCD.clrScr();
          drawBack();
          drawSetting_2();
          }
        }
}
        
//NH4 sensor
if (currentPage =='4'){
  if(myTouch.dataAvailable()){
    myTouch.read();
    x=myTouch.getX();
    y=myTouch.getY();
    //Back Button
       if ((x>=10) && (x<=60) &&(y>=10) && (y<=36)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0';
          myGLCD.clrScr();
          drawHomeScreen();
        }
}
}
if (currentPage == '5') {    

   if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
       
        // If we press the Back Button
        if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '1'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling();
          drawBack();// Draws the home screen
        }
        }
        }

if (currentPage == '6'){
  //drawSampling();
   if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
       
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0'; // Indicates we are at home screen
          myGLCD.clrScr();
          //drawBack();
          drawHomeScreen();
          // Draws the home screen
        }
        // If we press  * Button
        if ((x>=35) && (x<=285) &&(y>=190) && (y<=230)) {
          drawFrame(35, 190, 285, 230);
          currentPage = '1'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling();
          drawBack();// Draws the home screen
        }
        // If we press PH Button
        if ((x>=35) && (x<=285) && (y>=42) && (y<=82))
        {
        drawFrame(35, 42, 285, 82);
        currentPage = 'a';// 
        myGLCD.clrScr();
        drawBack();
        drawPHSensor();
        getPHSensor();   
        }

        // If we press NH4 button
        if ((x>=35) && (x<=285) && (y>=90) && (y<=130)) {
        drawFrame(35, 90, 285, 130);
        currentPage = 'b';// 
        myGLCD.clrScr();
        drawBack();
        drawNH4Sensor();
        getNH4Sensor();
        }

        // If we press Salinity button
        if ((x>=35) && (x<=285) && (y>=140) && (y<=180)) {
        drawFrame(35, 140, 285, 180);  
        currentPage = 'c';
        myGLCD.clrScr();
        drawBack();
        drawSalinitySensor();
        getSalinitySensor();  
      }
        
        }              
   }

if(currentPage == '7'){
     if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
          // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '1'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling();
          drawBack();// Draws the home screen
        }
        // If we press Temp Button(35, 90, 285, 130)
          if ((x>=35) && (x<=285) &&(y>=90) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '1'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling();
          drawBack();// Draws the home screen
        }        
  
}

}
if(currentPage == '8'){     
     if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '1'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling();
          drawBack();// Draws the home screen
        }
     }
}

if(currentPage == '9'){    
     if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '1'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling();
          drawBack();// Draws the home screen
        }
     }
}

if(currentPage == 'a'){
  
     if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '7'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling_2();
          drawBack();// Draws the home screen
        }
     }
}

if(currentPage == 'b'){
     
     if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '7'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling_2();
          drawBack();// Draws the home screen
        }
     }
}

if(currentPage == 'c'){
   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '7'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSampling_2();
          drawBack();// Draws the home screen
        }
     } 
}

if(currentPage == 'd'){
   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '3'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSetting();
          drawBack();// Draws the home screen
        }
        // If we press the < 0 >
        if((x>=180) && (x<=319) &&(y>=40) && (y<=100)){
          drawFrame(180, 50, 310, 90);
          currentPage = 'm'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawDOValueI();
//          drawBack();// Draws the home screen          
        }      

     } 
}
if(currentPage == 'e'){
   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '3'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSetting();
          drawBack();// Draws the home screen          
        }
        
     } 
}


if(currentPage == 'f'){
   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawHomeScreen();
          drawBack();// Draws the home screen          
        }
        //If we press server connection Button
        if ((x>=35) && (x<=285) && (y>=42) && (y<=82))
        {
        drawFrame(35, 42, 285, 82);
        currentPage = 'g';// 
        myGLCD.clrScr();
        drawBack();
        drawConnectServer();
        //getPHSensor();   
        }
        //If we press Date-Time Button
        if ((x>=35) && (x<=285) && (y>=90) && (y<=130)) {
        drawFrame(35, 90, 285, 130);
        currentPage = 'h';// 
        myGLCD.clrScr();
        drawBack();
        drawDate();
        //getNH4Sensor();
        }
        // If we press * Button
        if ((x>=35) && (x<=285) &&(y>=190) && (y<=230)) {
          drawFrame(35, 190, 285, 230);
          currentPage = '3'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSetting();
          drawBack();// Draws the home screen
        }             
     } 
}

if(currentPage == 'g'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = 'f'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSetting_2();
          drawBack();// Draws the home screen          
        }
        // If we press Identity
          if ((x>=175) && (x<=319) &&(y>=40) && (y<=80)) {
          drawFrame(175, 40, 315, 80);
          currentPage = 'r'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawKeyBoard();            
        }
     }
}

if(currentPage == 'h'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = 'f'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawSetting_2();
          drawBack();// Draws the home screen          
        }
     }
}

// calibration 9.4
if(currentPage == 'j'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '2'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawCalibration();
          drawBack();// Draws the home screen          
        }
        // If we press the < 0 >
        if((x>=180) && (x<=319) &&(y>=40) && (y<=100)){
          drawFrame(180, 50, 310, 90);
          currentPage = 'l'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawDOValueI();
//          drawBack();// Draws the home screen          
        }
     // If we press Next -> Button
      if ((x>=35) && (x<=285) && (y>=190) && (y<=230)) {
        drawFrame(35, 190, 285, 230);
        currentPage = 'z';
        myGLCD.clrScr();
        drawBack();
        drawDOCalibration_2();
      } 

        
     }
}

if(currentPage == 'k'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawHomeScreen();        
        }
         // If we press * Button
        if ((x>=35) && (x<=285) &&(y>=190) && (y<=230)) {
          drawFrame(35, 190, 285, 230);
          currentPage = '2'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawCalibration();
          drawBack();// Draws the home screen
        }  
     }
}

if(currentPage == 'l'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();

       // If press the rest Button       
       if ((y>=10) && (y<=60))  // Upper row
      {
        if ((x>=10) && (x<=60))  // Button: 1
        {
          waitForIt(10, 10, 60, 60);
          updateStr1('1');
        }
        if ((x>=70) && (x<=120))  // Button: 2
        {
          waitForIt(70, 10, 120, 60);
          updateStr1('2');
        }
        if ((x>=130) && (x<=180))  // Button: 3
        {
          waitForIt(130, 10, 180, 60);
          updateStr1('3');
        }
        if ((x>=190) && (x<=240))  // Button: 4
        {
          waitForIt(190, 10, 240, 60);
          updateStr1('4');
        }
        if ((x>=250) && (x<=300))  // Button: 5
        {
          waitForIt(250, 10, 300, 60);
          updateStr1('5');
        }
      }
      
      if ((y>=70) && (y<=120))  // Center row
      {
        if ((x>=10) && (x<=60))  // Button: 6
        {
          waitForIt(10, 70, 60, 120);
          updateStr1('6');
        }
        if ((x>=70) && (x<=120))  // Button: 7
        {
          waitForIt(70, 70, 120, 120);
          updateStr1('7');
        }
        if ((x>=130) && (x<=180))  // Button: 8
        {
          waitForIt(130, 70, 180, 120);
          updateStr1('8');
        }
        if ((x>=190) && (x<=240))  // Button: 9
        {
          waitForIt(190, 70, 240, 120);
          updateStr1('9');
        }
        if ((x>=250) && (x<=300))  // Button: 0
        {
          waitForIt(250, 70, 300, 120);
          updateStr1('0');
        }
      }
      
            
      if ((y>=130) && (y<=180))  // Upper row
      {
        if ((x>=10) && (x<=150))  // Button: Clear
        {
          waitForIt(10, 130, 150, 180);
          stCurrent1[0]='\0';
          stCurrentLen1=0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(0, 224, 319, 239);
        }
        if ((x>=160) && (x<=300))  // Button: Enter
        {
          waitForIt(160, 130, 300, 180);
          currentPage = 'j'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawBack();
          drawDOCalibration();
          getDOCalibration();

        }
        } 
     }
    }


if(currentPage == 'z'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '0'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawHomeScreen();        
        }
        // If we press the < 0>
        if((x>=180) && (x<=319) &&(y>=40) && (y<=100)){
          drawFrame(180, 50, 310, 90);
          currentPage = 'x'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawDOValueI();         
        }
        
     }
 }


if(currentPage == 'x'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
       // If press the rest Button       
       if ((y>=10) && (y<=60))  // Upper row
      {
        if ((x>=10) && (x<=60))  // Button: 1
        {
          waitForIt(10, 10, 60, 60);
          updateStr2('1');
        }
        if ((x>=70) && (x<=120))  // Button: 2
        {
          waitForIt(70, 10, 120, 60);
          updateStr2('2');
        }
        if ((x>=130) && (x<=180))  // Button: 3
        {
          waitForIt(130, 10, 180, 60);
          updateStr2('3');
        }
        if ((x>=190) && (x<=240))  // Button: 4
        {
          waitForIt(190, 10, 240, 60);
          updateStr2('4');
        }
        if ((x>=250) && (x<=300))  // Button: 5
        {
          waitForIt(250, 10, 300, 60);
          updateStr2('5');
        }
      }
      
      if ((y>=70) && (y<=120))  // Center row
      {
        if ((x>=10) && (x<=60))  // Button: 6
        {
          waitForIt(10, 70, 60, 120);
          updateStr2('6');
        }
        if ((x>=70) && (x<=120))  // Button: 7
        {
          waitForIt(70, 70, 120, 120);
          updateStr2('7');
        }
        if ((x>=130) && (x<=180))  // Button: 8
        {
          waitForIt(130, 70, 180, 120);
          updateStr2('8');
        }
        if ((x>=190) && (x<=240))  // Button: 9
        {
          waitForIt(190, 70, 240, 120);
          updateStr2('9');
        }
        if ((x>=250) && (x<=300))  // Button: 0
        {
          waitForIt(250, 70, 300, 120);
          updateStr2('0');
        }
      }
      
            
      if ((y>=130) && (y<=180))  // Upper row
      {
        if ((x>=10) && (x<=150))  // Button: Clear
        {
          waitForIt(10, 130, 150, 180);
          stCurrent2[0]='\0';
          stCurrentLen2=0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(0, 224, 319, 239);
        }
        if ((x>=160) && (x<=300))  // Button: Enter
        {
          waitForIt(160, 130, 300, 180);
          currentPage = 'n'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawBack();
          drawDOCalibration_2();
          getDOCalibration_2();

        }
        }     

     }
     }
if(currentPage == 'n'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '2'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawCalibration();
          drawBack();// Draws the home screen          
        }
//        // If we press the < 0>
//        if((x>=180) && (x<=319) &&(y>=40) && (y<=100)){
//          drawFrame(180, 50, 310, 90);
//          currentPage = 'l'; // Indicates we are at home screen
//          myGLCD.clrScr();
//          drawDOValueI();
////          drawBack();// Draws the home screen          
//        }
     
     }
     }


if(currentPage == 'm'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();

       // If press the rest Button       
       if ((y>=10) && (y<=60))  // Upper row
      {
        if ((x>=10) && (x<=60))  // Button: 1
        {
          waitForIt(10, 10, 60, 60);
          updateStr3('1');
        }
        if ((x>=70) && (x<=120))  // Button: 2
        {
          waitForIt(70, 10, 120, 60);
          updateStr3('2');
        }
        if ((x>=130) && (x<=180))  // Button: 3
        {
          waitForIt(130, 10, 180, 60);
          updateStr3('3');
        }
        if ((x>=190) && (x<=240))  // Button: 4
        {
          waitForIt(190, 10, 240, 60);
          updateStr3('4');
        }
        if ((x>=250) && (x<=300))  // Button: 5
        {
          waitForIt(250, 10, 300, 60);
          updateStr3('5');
        }
      }
      
      if ((y>=70) && (y<=120))  // Center row
      {
        if ((x>=10) && (x<=60))  // Button: 6
        {
          waitForIt(10, 70, 60, 120);
          updateStr3('6');
        }
        if ((x>=70) && (x<=120))  // Button: 7
        {
          waitForIt(70, 70, 120, 120);
          updateStr3('7');
        }
        if ((x>=130) && (x<=180))  // Button: 8
        {
          waitForIt(130, 70, 180, 120);
          updateStr3('8');
        }
        if ((x>=190) && (x<=240))  // Button: 9
        {
          waitForIt(190, 70, 240, 120);
          updateStr3('9');
        }
        if ((x>=250) && (x<=300))  // Button: 0
        {
          waitForIt(250, 70, 300, 120);
          updateStr3('0');
        }
      }
      
            
      if ((y>=130) && (y<=180))  // Upper row
      {
        if ((x>=10) && (x<=150))  // Button: Clear
        {
          waitForIt(10, 130, 150, 180);
          stCurrent3[0]='\0';
          stCurrentLen1=0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(0, 224, 319, 239);
        }
        if ((x>=160) && (x<=300))  // Button: Enter
        {
          waitForIt(160, 130, 300, 180);
          currentPage = 'q'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawBack();
          drawSamplingParameters();
          getSamplingParameters();

        }
        } 
     }
    }


if(currentPage == 'q'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '3'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawBack();// Draws the home screen  
          drawSetting();        
        }
        // If we press the < 1 >
        if((x>=180) && (x<=319) &&(y>=110) && (y<=170)){
          drawFrame(180, 120, 310, 160);
          currentPage = 'w'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawDOValueI();        
        }

        // If we press the < 0>
        if((x>=180) && (x<=319) &&(y>=40) && (y<=100)){
          drawFrame(180, 50, 310, 90);
          currentPage = 'm'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawDOValueI();         
        }
             
     }     
    }
    

if(currentPage == 'w'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();

       // If press the rest Button       
       if ((y>=10) && (y<=60))  // Upper row
      {
        if ((x>=10) && (x<=60))  // Button: 1
        {
          waitForIt(10, 10, 60, 60);
          updateStr4('1');
        }
        if ((x>=70) && (x<=120))  // Button: 2
        {
          waitForIt(70, 10, 120, 60);
          updateStr4('2');
        }
        if ((x>=130) && (x<=180))  // Button: 3
        {
          waitForIt(130, 10, 180, 60);
          updateStr4('3');
        }
        if ((x>=190) && (x<=240))  // Button: 4
        {
          waitForIt(190, 10, 240, 60);
          updateStr4('4');
        }
        if ((x>=250) && (x<=300))  // Button: 5
        {
          waitForIt(250, 10, 300, 60);
          updateStr4('5');
        }
      }
      
      if ((y>=70) && (y<=120))  // Center row
      {
        if ((x>=10) && (x<=60))  // Button: 6
        {
          waitForIt(10, 70, 60, 120);
          updateStr4('6');
        }
        if ((x>=70) && (x<=120))  // Button: 7
        {
          waitForIt(70, 70, 120, 120);
          updateStr4('7');
        }
        if ((x>=130) && (x<=180))  // Button: 8
        {
          waitForIt(130, 70, 180, 120);
          updateStr4('8');
        }
        if ((x>=190) && (x<=240))  // Button: 9
        {
          waitForIt(190, 70, 240, 120);
          updateStr4('9');
        }
        if ((x>=250) && (x<=300))  // Button: 0
        {
          waitForIt(250, 70, 300, 120);
          updateStr4('0');
        }
      }
      
            
      if ((y>=130) && (y<=180))  // Upper row
      {
        if ((x>=10) && (x<=150))  // Button: Clear
        {
          waitForIt(10, 130, 150, 180);
          stCurrent4[0]='\0';
          stCurrentLen4=0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(0, 224, 319, 239);
        }
        if ((x>=160) && (x<=300))  // Button: Enter
        {
          waitForIt(160, 130, 300, 180);
          currentPage = 'q'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawBack();
          drawSamplingParameters();
          getSamplingParameters_2();

        }
        } 
     }
    }

if(currentPage == 'r'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();

       // If press the rest Button       
       if ((y>=10) && (y<=60))  // Upper row
      {
        if ((x>=10) && (x<=60))  // Button: 1
        {
          waitForIt(10, 10, 60, 60);
          updateStr5('a');
        }
        if ((x>=70) && (x<=120))  // Button: 2
        {
          waitForIt(70, 10, 120, 60);
          updateStr5('b');
        }
        if ((x>=130) && (x<=180))  // Button: 3
        {
          waitForIt(130, 10, 180, 60);
          updateStr5('c');
        }
        if ((x>=190) && (x<=240))  // Button: 4
        {
          waitForIt(190, 10, 240, 60);
          updateStr5('d');
        }
        if ((x>=250) && (x<=300))  // Button: 5
        {
          waitForIt(250, 10, 300, 60);
          updateStr5('e');
        }
      }
      
      if ((y>=70) && (y<=120))  // Center row
      {
        if ((x>=10) && (x<=60))  // Button: 6
        {
          waitForIt(10, 70, 60, 120);
          updateStr5('f');
        }
        if ((x>=70) && (x<=120))  // Button: 7
        {
          waitForIt(70, 70, 120, 120);
          updateStr5('g');
        }
        if ((x>=130) && (x<=180))  // Button: 8
        {
          waitForIt(130, 70, 180, 120);
          updateStr5('2');
        }
        if ((x>=190) && (x<=240))  // Button: 9
        {
          waitForIt(190, 70, 240, 120);
          updateStr5('1');
        }
        if ((x>=250) && (x<=300))  // Button: 0
        {
          waitForIt(250, 70, 300, 120);
          updateStr5('0');
        }
      }
      
            
      if ((y>=130) && (y<=180))  // Upper row
      {
        if ((x>=10) && (x<=150))  // Button: Clear
        {
          waitForIt(10, 130, 150, 180);
          stCurrent5[0]='\0';
          stCurrentLen5=0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(0, 224, 319, 239);
        }
        if ((x>=160) && (x<=300))  // Button: Enter
        {
          waitForIt(160, 130, 300, 180);
          currentPage = 't'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawBack();
          drawConnectServer();
          getConnectServer();

        }
        } 
     }
    }


if(currentPage == 't'){   
     if (myTouch.dataAvailable()){ 
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
        // If we press the Back Button
         if ((x>=0) && (x<=100) &&(y>=0) && (y<=100)) {
          drawFrame(10, 10, 60, 36);
          currentPage = '3'; // Indicates we are at home screen
          myGLCD.clrScr();
          drawBack();// Draws the home screen  
          drawSetting();        
        }
     }
     }  
// If we press save button
if((x>=10) && (x<90) &&(y>=195) && (y<=223)) {
  drawFrame(10,195,90,223);
if(currentPage == '5'){
  char typeofsensor[10]="DO";
  float DO_Intercept = -1.47237; // hieu chuan CTU
  float DO_Slope = 7.02247;       // hieu chuan CTU
  float DO_Voltage=getVoltage(A1,AVGTIME, 1000);
  float DO = DO_Intercept + DO_Voltage * DO_Slope; //converts voltage to sensor reading
  // make a string for assembling the data to log:
   String dataString = "";
  // read three sensors and append to the string:
    //int sensor = analogRead(DO);
    dataString += String(DO_Voltage);
    dataString +=  ";";
    dataString += String(DO);
    dataString += ";";
    dataString += String(typeofsensor);
  
  dataString = getStringTime() + ";" + dataString;
  writeLogFile("datalog.txt",dataString);
  //delay(1000);
        }

if(currentPage == 'a'){
  char typeofsensor[10]="PH";  
  float PH_Intercept = 16.237; // nhà cung cấp
  float PH_Slope = -7.752;     // nhà cung cấp
  float PH_Voltage = getVoltage(A1,AVGTIME,1000);
  float PH = PH_Intercept + PH_Voltage * PH_Slope; //converts voltage to sensor reading
  // make a string for assembling the data to log:
   String dataString = "";
  // read three sensors and append to the string:
    //int sensor = analogRead(PH);
    dataString += String(PH_Voltage);
    dataString +=  ";";
    dataString += String(PH);
    dataString += ";";
    dataString += String(typeofsensor);
  
  dataString = getStringTime() + ";" + dataString;
  writeLogFile("datalog.txt",dataString);
  //delay(1000);              
        }


  if(currentPage == '8'){
    char typeofsensor[15]="Temperature";  
    float temperature_analog = 0;
    float Temp;
    temperature_analog = analogRead(A1);   // and  convert it to CelsiusSerial.print(Time/1000); //display in seconds, not milliseconds
    Temp = Thermistor(temperature_analog);
      // make a string for assembling the data to log:
   String dataString = "";
  // read three sensors and append to the string:
    //int sensor = analogRead(Temp);
    dataString += String(Temp);
    dataString += ";";
    dataString += String(typeofsensor);
  
  dataString = getStringTime() + ";" + dataString;
  writeLogFile("datalog.txt",dataString);
  delay(1000);
    }

  if(currentPage == 'b'){
  char typeofsensor[15]="NH4";  
  float Eo = 182.7733;  // tự điều chỉnh
  float m = 2.986860299; // tự điều chỉnh
  
  float NH4_Voltage = getVoltage(A1,AVGTIME, 1000);
  float ElectrodeReading = 137.55 * NH4_Voltage - 0.1682; // converts raw voltage to mV from electrode
  double(val) = ((ElectrodeReading - Eo) / m); // calculates the value to be entered into exp func.
  float NH4 = exp(val); // converts mV value to concentration 
    String dataString = "";
  // read three sensors and append to the string:
    //int sensor = analogRead(NH4);
    dataString += String(NH4_Voltage);
    dataString +=  ";";
    dataString += String(NH4);
    dataString += ";";
    dataString += String(typeofsensor);
  
  dataString = getStringTime() + ";" + dataString;
  writeLogFile("datalog.txt",dataString);
 // delay(1000);            
  }    

  if(currentPage == '9'){
  char typeofsensor[15]="NO3";  
  float E_no3 = 210.9300610;  // tự điều chỉnh
  float m_no3  = -6.569360279; // tự điều chỉnh
  
  float NO3_Voltage = getVoltage(A1,AVGTIME,1000);
  float ElectrodeReading_no3 = 137.55 * NO3_Voltage - 0.1682; // converts raw voltage to mV from electrode
  double(val_no3) = ((ElectrodeReading_no3 - E_no3) / m_no3); // calculates the value to be entered into exp func.
  float NO2 = exp(val_no3); // converts mV value to concentration 
      String dataString = "";
  // read three sensors and append to the string:
    //int sensor = analogRead(NO2);
    dataString += String(NO3_Voltage);
    dataString +=  ";";    
    dataString += String(NO2);
    dataString += ";";
    dataString += String(typeofsensor);
  
  dataString = getStringTime() + ";" + dataString;
  writeLogFile("datalog.txt",dataString);
  //delay(1000);                 
  }

  if(currentPage == 'c'){
  char typeofsensor[15]="Salinity";  
  float DoMan_Intercept = 0; // nhà cung cấp
  float DoMan_Slope = 16.3;       // nhà cung cấp
  float DoMan_Voltage = getVoltage(A1,AVGTIME,1000);
  float DoMan = DoMan_Intercept + DoMan_Voltage * DoMan_Slope; //converts voltage to sensor reading    
    String dataString = "";
  // read three sensors and append to the string:
    //int sensor = analogRead(DoMan);
    dataString += String(DoMan_Voltage);
    dataString +=  ";";      
    dataString += String(DoMan);
    dataString += ";";
    dataString += String(typeofsensor);
  
  dataString = getStringTime() + ";" + dataString;
  writeLogFile("datalog.txt",dataString);
  //delay(1000);      
    
  }

if(currentPage == 'n'){
  int Voltage1; //= getVoltage(A1,AVGTIME,1000);
  int Voltage2; //= getVoltage(A1,AVGTIME,1000);
  int stCurrent1;
  int stCurrent2;
  int InterCept = (stCurrent2 * Voltage1 - stCurrent1 * Voltage2 )/(Voltage1 - Voltage2);
  int Slope = (stCurrent1 - InterCept)/Voltage1;
Serial.print(Voltage1);
//String dataString = "";
//dataString += String(InterCept);
//    dataString +=  "/";
//dataString += String(Slope);   
//writeLogFile("config.cfg",dataString);
  }
  //myGLCD.setColor(0, 255, 0);
  //myGLCD.print(stCurrent1, LEFT, 224);
}



//If we press cancel button(210,195,310,223)
if((x>=210) && (x<310) &&(y>=195) && (y<=223)){
  drawFrame(210,195,310,223);
  if(currentPage == '5'){
  //myGLCD.clrScr();
  drawDOSensor();
  }
  if(currentPage == '8')
  {
  drawTemperatureSensor();
  }
  if(currentPage == '9'){
  drawNO3Sensor();
  }
  if(currentPage == 'a'){
  drawPHSensor(); 
  }
  if(currentPage == 'b'){
  drawNH4Sensor();
  }
  if(currentPage == 'c'){
  drawSalinitySensor();
  }  
  }
}
void drawHomeScreen(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Main menu",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - Sampling
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Sampling", CENTER, 52); // Prints the string




  // Button - Calibration
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Calibration", CENTER, 102); // Prints the string



   // Button Setting
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 140, 285, 180); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 140, 285, 180); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Setting", CENTER, 152); // Prints the string



  // Button Graph Screen
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("OFF", CENTER, 202);
}

void drawHomeScreen_2(){
  
}
void drawSetting(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Setting",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - Sampling parameters
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Sampling Pt", CENTER, 52); // Prints the string




  // Button Wifi
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Wifi", CENTER, 102); // Prints the string

  // Button **
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("**", CENTER, 202);
  
}

void drawSetting_2(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Setting",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - server connection
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Connect Server", CENTER, 52); // Prints the string




  // Button Date - Time
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Date - Time", CENTER, 102); // Prints the string

  // Button *
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("*", CENTER, 202);
  
}
void drawHomeScreen_4(){
  }



 void drawFrame(int x1, int y1, int x2, int y2){
   myGLCD.setColor(255, 0, 0);
   myGLCD.drawRoundRect (x1, y1, x2, y2);
   while (myTouch.dataAvailable())
    myTouch.read();
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (x1, y1, x2, y2);
  }

//(x>=0) && (x<=100) &&(y>=100) && (y<=240
 void drawSampling(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Sampling",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - Sampling
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("DO", CENTER, 52); // Prints the string




  // Button - Calibration
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Temp", CENTER, 102); // Prints the string



   // Button Setting
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 140, 285, 180); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 140, 285, 180); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("NO3", CENTER, 152); // Prints the string



  // Button Graph Screen
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("**", CENTER, 202);

  
  }


  void drawSampling_2()
  {
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Sampling",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - PH
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("PH", CENTER, 52); // Prints the string




  // Button - NH4
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("NH4", CENTER, 102); // Prints the string



   // Button Salinity
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 140, 285, 180); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 140, 285, 180); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Salinity", CENTER, 152); // Prints the string



  // Button Graph Screen
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("*", CENTER, 202);    
    
    }
void drawCalibration()
  {
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Calibration",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - Sampling
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("DO", CENTER, 52); // Prints the string




  // Button - Calibration
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Temp", CENTER, 102); // Prints the string



   // Button Setting
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 140, 285, 180); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 140, 285, 180); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("NO3", CENTER, 152); // Prints the string



  // Button Graph Screen
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("**", CENTER, 202);  
  }

 void drawCalibration_2()
  {
   myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Calibration",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - PH
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("PH", CENTER, 52); // Prints the string




  // Button - NH4
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("NH4", CENTER, 102); // Prints the string



   // Button Salinity
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 140, 285, 180); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 140, 285, 180); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Salinity", CENTER, 152); // Prints the string



  // Button Graph Screen
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("*", CENTER, 202);    
    
    
  }
 void drawOFF()
  {
      
  }

  void drawNH4Sensor()
  {
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Ve truoc", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Cam Bien Ammoninum", CENTER, 50);
  myGLCD.print("(mg/L)", CENTER, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255,255,255);
  myGLCD.setFont(BigFont);
  myGLCD.print("NH4_Voltage: ",5, 120);
  myGLCD.print("Gia tri do:",5,160);
  //myGLCD.setColor(255, 255, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);    
    
  }

void drawBack(){
  
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  //myGLCD.print("Back to Main Menu", 70, 18);
  }
  
void drawDOSensor()
  {
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Ve truoc", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Cam Bien DO", CENTER, 50);
  myGLCD.print("(ppt)", CENTER, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255,255,255);
  myGLCD.setFont(BigFont);
  myGLCD.print("DO_Voltage: ",5, 120);
  myGLCD.print("Gia tri do:",5,160);
  //myGLCD.setColor(255, 255, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);        
  }

  void drawTemperatureSensor()
  {
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Ve truoc", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Cam Bien Nhiet Do", CENTER, 50);
  myGLCD.print("(TMP-BTA)", CENTER, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255,255,255);
  myGLCD.setFont(BigFont);
  myGLCD.print("Gia tri do: ",5, 120);
  //myGLCD.setColor(255, 255, 255);
myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);        
  }

void drawNO3Sensor()
  {
 myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
    
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Ve truoc", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Cam Bien Nitrate", CENTER, 50);
//  myGLCD.print("(mg/L)", CENTER, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255,255,255);
  myGLCD.setFont(BigFont);
  myGLCD.print("NO3_Voltage: ",5, 120);
  myGLCD.print("Gia tri do:",5,160);
  //myGLCD.setColor(255, 255, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);   
    
  }
void  drawPHSensor(){
 myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Ve truoc", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Cam Bien PH", CENTER, 50);
  myGLCD.print("", CENTER, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255,255,255);
  myGLCD.setFont(BigFont);
  myGLCD.print("PH_Voltage: ",5, 120);
  myGLCD.print("Gia tri do:",5,160);
  //myGLCD.setColor(255, 255, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200); 
  
}

void drawNH4(){
 myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Ve truoc", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Cam Bien Ammoninum", CENTER, 50);
  myGLCD.print("(mg/L)", CENTER, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255,255,255);
  myGLCD.setFont(BigFont);
  myGLCD.print("NH4_Voltage: ",5, 120);
  myGLCD.print("Gia tri do:",5,160);
  //myGLCD.setColor(255, 255, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);     
}

void drawSalinitySensor(){

 myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect(10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-",18,15);
  myGLCD.setBackColor(0,0,0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Ve truoc", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Cam Bien Do Man", CENTER, 50);
  myGLCD.print("(ppt)", CENTER, 76);
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255,255,255);
  myGLCD.setFont(BigFont);
  myGLCD.print("DoMan_Voltage: ",5, 120);
  myGLCD.print("Gia tri do:",5,160);
  //myGLCD.setColor(255, 255, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);   
  }

void drawSamplingParameters()
{
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Parameters",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - Interval
  //myGLCD.setColor(16, 167, 103); // Sets green color
  //myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  //myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  //myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Interval :", 35, 62); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("< 10 >", 200, 62);


  // Button - Sample No
  //myGLCD.setColor(16, 167, 103); // Sets green color
  //myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  //myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  //myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Sample No:", 35, 132); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("< 3  >",200,132);
  
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);  
}

void drawWifi()
{
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("WIFI",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - SSID
//  myGLCD.setColor(16, 167, 103); // Sets green color
//  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
//  myGLCD.setColor(255, 255, 255); // Sets color to white
//  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
//  myGLCD.setFont(BigFont); // Sets the font to big
//  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
//  myGLCD.print("SSID", CENTER, 52); // Prints the string
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.print("SSID    :", 35, 62); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("CNPM", 250, 62);
  
  // Button - Password
//  myGLCD.setColor(16, 167, 103); // Sets green color
//  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
//  myGLCD.setColor(255, 255, 255); // Sets color to white
//  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
//  myGLCD.setFont(BigFont); // Sets the font to big
//  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
//  myGLCD.print("Password", CENTER, 102); // Prints the string
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.print("Password:", 35, 132); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("CNPM@2015", 175, 132);  
  
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);  
 }

void drawConnectServer(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Connect Server",70,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white



  //Button - Identity
//  myGLCD.setColor(16, 167, 103); // Sets green color
//  myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
//  myGLCD.setColor(255, 255, 255); // Sets color to white
//  myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
//  myGLCD.setFont(BigFont); // Sets the font to big
//  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
//  myGLCD.print("Identity", CENTER, 52); // Prints the string
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.print("Identity:", 35, 52); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("Handy0001",175,52); 

  // Button - Host
//  myGLCD.setColor(16, 167, 103); // Sets green color
//  myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
//  myGLCD.setColor(255, 255, 255); // Sets color to white
//  myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
//  myGLCD.setFont(BigFont); // Sets the font to big
//  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
//  myGLCD.print("Host", CENTER, 102); // Prints the string
    myGLCD.setColor(255, 255, 255); // Sets color to white
    myGLCD.setFont(BigFont); // Sets the font to big
    myGLCD.print("Password:", 35, 102); // Prints the string
    myGLCD.setColor(0, 0, 255);
    myGLCD.print("Handy0001",175,102); 

  // Button Password
//  myGLCD.setColor(16, 167, 103); // Sets green color
//  myGLCD.fillRoundRect (35, 140, 285, 180); // Draws filled rounded rectangle
//  myGLCD.setColor(255, 255, 255); // Sets color to white
//  myGLCD.drawRoundRect (35, 140, 285, 180); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
//  myGLCD.setFont(BigFont); // Sets the font to big
//  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
//  myGLCD.print("Password", CENTER, 152); // Prints the string
    myGLCD.setColor(255, 255, 255); // Sets color to white
    myGLCD.setFont(BigFont); // Sets the font to big
    myGLCD.print("Host:", 35, 152); // Prints the string
    myGLCD.setColor(0, 0, 255);
    myGLCD.print("103.221.220.148",110,152); 
  
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);  
}

void drawDate(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("Date - Time",CENTER,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white

  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.print("Date:", 35, 62); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("01/01/2017",150,62);

  
  
  
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.print("Time:", 35, 132); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("00:00:00",175,132);

  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);

  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(210,195,310,223);
  myGLCD.setColor(255,255,255);
  myGLCD.drawRoundRect(210,195,310,223);
  myGLCD.setBackColor(255,0,0);
  myGLCD.setColor(255,255,255);
  myGLCD.print("cancel", 213, 200);  

}

void drawDOCalibration(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("DO Calibration",75,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white

//  myGLCD.setColor(255, 255, 255);
//  myGLCD.drawLine(0,80,319,80);
//  myGLCD.drawLine(0,60,319,60);
  
  //Button - Interval
  //myGLCD.setColor(16, 167, 103); // Sets green color
  //myGLCD.fillRoundRect (35, 42, 285, 82); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  //myGLCD.drawRoundRect (35, 42, 285, 82); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(BigFont); // Sets the font to big
  //myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Value I:", 35, 62); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("<   >", 200, 62);


  // Button - Voltage
  //myGLCD.setColor(16, 167, 103); // Sets green color
  //myGLCD.fillRoundRect (35, 90, 285, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  //myGLCD.drawRoundRect (35, 90, 285, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame 10, 42, 160, 82
  myGLCD.setFont(BigFont); // Sets the font to big
  //myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Voltage:", 35, 132); // Prints the string
  myGLCD.setColor(0, 0, 255);
  //myGLCD.print("< 3 >",200,132);

   // Button **
  myGLCD.setColor(16, 167, 103);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(16, 167, 103);
  myGLCD.print("NEXT ->", CENTER, 202);
  }

void drawDOValueI(){ 
myGLCD.setBackColor(0, 0, 255);
myGLCD.setFont(BigFont);
// Draw the upper row of buttons
  for (x=0; x<5; x++)
  {
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (10+(x*60), 10, 60+(x*60), 60);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (10+(x*60), 10, 60+(x*60), 60);
    myGLCD.printNumI(x+1, 27+(x*60), 27);
  }
// Draw the center row of buttons
  for (x=0; x<5; x++)
  {
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (10+(x*60), 70, 60+(x*60), 120);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (10+(x*60), 70, 60+(x*60), 120);
    if (x<4)
      myGLCD.printNumI(x+6, 27+(x*60), 87);
  }
  myGLCD.print("0", 267, 87);
// Draw the lower row of buttons
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (10, 130, 150, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 130, 150, 180);
  myGLCD.print("Clear", 40, 147);
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (160, 130, 300, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (160, 130, 300, 180);
  myGLCD.print("Enter", 190, 147);
  myGLCD.setBackColor (0, 0, 0);
}

void drawKeyBoard(){ 
myGLCD.setBackColor(0, 0, 255);
myGLCD.setFont(BigFont);
// Draw the upper row of buttons
  for (x=0; x<5; x++)
  {
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (10+(x*60), 10, 60+(x*60), 60);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (10+(x*60), 10, 60+(x*60), 60);
    myGLCD.print("a", 27+(x*60), 27);
    myGLCD.print("b", 27+(1*60), 27);
    myGLCD.print("c", 27+(2*60), 27);
    myGLCD.print("d", 27+(3*60), 27);
    myGLCD.print("e", 27+(4*60), 27);
  }
  
// Draw the center row of buttons
  for (x=0; x<5; x++)
  {
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (10+(x*60), 70, 60+(x*60), 120);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (10+(x*60), 70, 60+(x*60), 120);
    if (x<4)
      myGLCD.print("f", 27+(x*60), 87);
      myGLCD.print("g", 27+(1*60), 87);
      myGLCD.print("2", 27+(2*60), 87);
      myGLCD.print("1", 27+(3*60), 87);
  }
  myGLCD.print("0", 267, 87);
// Draw the lower row of buttons
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (10, 130, 150, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 130, 150, 180);
  myGLCD.print("Clear", 40, 147);
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRoundRect (160, 130, 300, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (160, 130, 300, 180);
  myGLCD.print("Enter", 190, 147);
  myGLCD.setBackColor (0, 0, 0);
}


void drawDOCalibration_2(){
  myGLCD.setBackColor(0,0,0); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets font to big
  myGLCD.print("IoTLab-CTU", 220, 0);
  myGLCD.setFont(BigFont); // Sets font to big
  myGLCD.print("DO Calibration",75,20);
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(210,12,310,12);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawLine(0,38,319,38); // Draws the red line
  myGLCD.drawLine(0,184,319,184);
  myGLCD.drawLine(0,185,319,185);
  myGLCD.drawLine(0,186,319,186);
  myGLCD.setColor(255, 255, 255); // Sets color to white

  
  //Button - Interval
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.print("Value II:", 35, 62); // Prints the string
  myGLCD.setColor(0, 0, 255);
  myGLCD.print("<   >", 200, 62);


  // Button - Voltage II

  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(BigFont); // Sets the font to big
  myGLCD.print("Voltage :", 35, 132); // Prints the string
  myGLCD.setColor(0, 0, 255);
  //myGLCD.print("< 3 >",200,132);

   // Button save
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect (10, 195, 90, 223);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 195, 90, 223);
  myGLCD.setBackColor(255,0 , 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Save", 17, 200);
  }



  
  void getDOSensor(){
  float DO_Intercept = -1.47237; // hieu chuan CTU
  float DO_Slope = 7.02247;       // hieu chuan CTU

  float DO_Voltage=getVoltage(A1,AVGTIME, 1000);
  float DO = DO_Intercept + DO_Voltage * DO_Slope; //converts voltage to sensor reading
  //int sensor = analogRead(DO);  
    // Read the input on analog pin 0(named 'sensor')
  //  byte sensorValue0 = analogRead(sensor); 
    // Print out the value you read
    myGLCD.setFont(BigFont);   
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0,0,0);
      if (DEBUG){
      myGLCD.printNumF(DO_Voltage,2,210,120);
      //myGLCD.printNumI(DO, 210, 160,1,'0');
      myGLCD.printNumF(DO,2,210,160);
    //myGLCD.printNumI(DO, 110, 145);
     //Serial.print("DO: ");
     //Serial.println(DO);
     //delay(45000);
  }
    
    //myGLCD.setBackColor(100, 155, 203);
//    myGLCD.setFont(BigFont);
//    myGLCD.print("mg/L", 235, 178);
   //delay(1000);
    
  }

  
  void getTempSensor(){
     // Read the input on analog pin 0(named 'sensor')
    float temperature_analog = 0;
    float Temp;
    // sensorValue0 = analogRead(sensor); 
    // Print out the value you read
    temperature_analog = analogRead(A1);   // and  convert it to CelsiusSerial.print(Time/1000); //display in seconds, not milliseconds
    Temp = Thermistor(temperature_analog);
    
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    if (DEBUG){
    //myGLCD.printNumI();
    //myGLCD.printNumI(Temp,210, 160,1,'0');
    myGLCD.printNumF(Temp,2,210,160);
    
    //Serial.println(Temp);
    }
//    myGLCD.setFont(BigFont);
    //myGLCD.print("C", 235, 178);
    delay(1500);
    }

void getNO3Sensor(){
  float E_no3 = 210.9300610;  // tự điều chỉnh
  float m_no3  = -6.569360279; // tự điều chỉnh
  
  float NO3_Voltage = getVoltage(A1,AVGTIME,10);
  float ElectrodeReading_no3 = 137.55 * NO3_Voltage - 0.1682; // converts raw voltage to mV from electrode
  double(val_no3) = ((ElectrodeReading_no3 - E_no3) / m_no3); // calculates the value to be entered into exp func.
  float NO2 = exp(val_no3); // converts mV value to concentration
     
     // Read the input on analog pin 0(named 'sensor')
    //sensorValue0 = analogRead(sensor); 
    // Print out the value you read
    
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    if (DEBUG){
//    myGLCD.printNumI(NO3_Voltage, 210, 120, 1, '0');
//    myGLCD.printNumI(NO2, 210, 160,1,'0');    
    myGLCD.printNumF(NO3_Voltage, 2, 210, 120);
    myGLCD.printNumF(NO2, 2, 210, 160);
    //Serial.print("NO2: ");
    //Serial.println(NO2);    
  }      
    //myGLCD.setFont(BigFont);
    //myGLCD.print("mg/L", 235, 178);
//    delay(1000);
    }

  void getPHSensor(){
     
  float PH_Intercept = 16.237; // nhà cung cấp
  float PH_Slope = -7.752;     // nhà cung cấp

  float PH_Voltage = getVoltage(A1,AVGTIME,1000);
  float PH = PH_Intercept + PH_Voltage * PH_Slope; //converts voltage to sensor reading
     
     // Read the input on analog pin 0(named 'sensor')
   // byte sensorValue0 = analogRead(sensor); 
    // Print out the value you read
    
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
      if (DEBUG){
//    myGLCD.printNumI(PH_Voltage,210,120,1,'0');
//    myGLCD.printNumI(PH, 210, 160,1,'0');
    myGLCD.printNumF(PH_Voltage,2,210,120);
    myGLCD.printNumF(PH,2, 210, 160);
    //Serial.print("PH: ");
    //Serial.println(PH);
    //delay(1000); 
  }
    
//    myGLCD.setFont(BigFont);
    //myGLCD.print("mg/L", 235, 178);
    //delay(1000);
    }

void getNH4Sensor(){
    
  float Eo = 182.7733;  // tự điều chỉnh
  float m = 2.986860299; // tự điều chỉnh
  
  float NH4_Voltage = getVoltage(A1,AVGTIME, 1000);
  float ElectrodeReading = 137.55 * NH4_Voltage - 0.1682; // converts raw voltage to mV from electrode
  double(val) = ((ElectrodeReading - Eo) / m); // calculates the value to be entered into exp func.
  float NH4 = exp(val); // converts mV value to concentration
     // Read the input on analog pin 0(named 'sensor')
   // byte sensorValue0 = analogRead(sensor); 
    // Print out the value you read
    
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
      if (DEBUG){
   myGLCD.printNumF(NH4_Voltage, 2, 210, 120);
   myGLCD.printNumF(NH4, 2, 210, 160);
  //Serial.print("NH4: ");
  //Serial.println(NH4);    
  }    
//    myGLCD.setFont(BigFont);
//    myGLCD.print("mg/L", 235, 178);
//    delay(1000);
    }

void getSalinitySensor(){
     //Salinity sensor
  float DoMan_Intercept = 0; // nhà cung cấp
  float DoMan_Slope = 16.3;       // nhà cung cấp
  float DoMan_Voltage = getVoltage(A1,AVGTIME,1000);
  float DoMan = DoMan_Intercept + DoMan_Voltage * DoMan_Slope; //converts voltage to sensor reading
     // Read the input on analog pin 0(named 'sensor')
//    sensorValue0 = analogRead(sensor); 
    // Print out the value you read
    myGLCD.setFont(BigFont);
    //myGLCD.setFont(SevenSegNumFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
     if (DEBUG){  
  myGLCD.printNumF(DoMan_Voltage, 2,210, 120);
  myGLCD.printNumF(DoMan, 2, 210, 160);
  //Serial.print("DoMan: ");
  //Serial.println(DoMan);  
  }  

//    myGLCD.setFont(BigFont);
//    myGLCD.print("ppt", 235, 178);
//    delay(1000);
    }



void updateStr1(int val)
{
  if (stCurrentLen1<20)
  {
    stCurrent1[stCurrentLen1]=val;
    stCurrent1[stCurrentLen1+1]='\0';
    stCurrentLen1++;
    myGLCD.setColor(0, 255, 0);
    myGLCD.print(stCurrent1, LEFT, 224);
    int (stCurrent1) = Serial.read();
  }
  else
  {
    myGLCD.setColor(255, 0, 0);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    delay(500);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    myGLCD.setColor(0, 255, 0);
  }
}



void updateStr2(int val)
{
  if (stCurrentLen2<20)
  {
    stCurrent2[stCurrentLen2]=val;
    stCurrent2[stCurrentLen2+1]='\0';
    stCurrentLen2++;
    myGLCD.setColor(0, 255, 0);
    myGLCD.print(stCurrent2, LEFT, 224);
   int (stCurrent2) = Serial.read();
  }
  else
  {
    myGLCD.setColor(255, 0, 0);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    delay(500);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    myGLCD.setColor(0, 255, 0);
  }
}

void updateStr3(int val)
{
  if (stCurrentLen3<20)
  {
    stCurrent3[stCurrentLen3]=val;
    stCurrent3[stCurrentLen3+1]='\0';
    stCurrentLen3++;
    myGLCD.setColor(0, 255, 0);
    myGLCD.print(stCurrent3, LEFT, 224);
   int (stCurrent3) = Serial.read();
  }
  else
  {
    myGLCD.setColor(255, 0, 0);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    delay(500);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    myGLCD.setColor(0, 255, 0);
  }
}


void updateStr4(int val)
{
  if (stCurrentLen4<20)
  {
    stCurrent4[stCurrentLen4]=val;
    stCurrent4[stCurrentLen4+1]='\0';
    stCurrentLen4++;
    myGLCD.setColor(0, 255, 0);
    myGLCD.print(stCurrent4, LEFT, 224);
   int (stCurrent4) = Serial.read();
  }
  else
  {
    myGLCD.setColor(255, 0, 0);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    delay(500);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    myGLCD.setColor(0, 255, 0);
  }
}


void updateStr5(int val)
{
  if (stCurrentLen5<20)
  {
    stCurrent5[stCurrentLen5]=val;
    stCurrent5[stCurrentLen5+1]='\0';
    stCurrentLen5++;
    myGLCD.setColor(0, 255, 0);
    myGLCD.print(stCurrent5, LEFT, 224);
   int (stCurrent5) = Serial.read();
  }
  else
  {
    myGLCD.setColor(255, 0, 0);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    delay(500);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    myGLCD.setColor(0, 255, 0);
  }
}

// Draw a red frame while a button is touched
void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}




      
void getDOCalibration(){
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
     if (DEBUG)
      {
    //myGLCD.printNumF(stCurrent1,2,40,62);
    float Voltage1 = getVoltage(A1,10,1000);
    myGLCD.print(stCurrent1,230, 62);
    myGLCD.printNumF(Voltage1,2,205,132);
    //Serial.print(stCurrent1);
    //Serial.println(Voltage1);
//    int (Voltage1) = Serial.read();
//    Serial.println(Voltage1);
  }
}


void getDOCalibration_2(){
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
     if (DEBUG)
      {
    //myGLCD.printNumF(stCurrent1,2,40,62);
    float Voltage2 = getVoltage(A1,10,1000);
    
    myGLCD.print(stCurrent2,230, 62);
    myGLCD.printNumF(Voltage2,2,205,132);
    //int Voltage2 = Serial.read();
  }
  }


void getSamplingParameters(){
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 0, 255);
    myGLCD.setBackColor(0, 0, 0);
     if (DEBUG)
      {
            myGLCD.print(stCurrent3, 230, 62);
      }
  
}

void getSamplingParameters_2(){
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 0, 255);
    myGLCD.setBackColor(0, 0, 0);
     if (DEBUG)
      {
            myGLCD.print(stCurrent3, 230, 62);
            myGLCD.print(stCurrent4, 230, 132);
      }
  
}

void getConnectServer(){
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 0, 255);
    myGLCD.setBackColor(0, 0, 0);
     if (DEBUG)
      {
            myGLCD.print(stCurrent5, 175, 52);
      }
  
}
