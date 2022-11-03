#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

char colors[5] = {'W','W','W','W','W'};

LiquidCrystal_I2C lcd(0x27, 20, 4);// connect SDA to A4, SCL to A5 for arduino nano. Connects I2C LCD Display

volatile bool touched, clapped;
int prevState = 0, currState = 0;
bool firstTime;

int rotAngle, incAngle, v_r, v_l, dist;
bool rotDir;
bool flagBKforNextLine;

bool endFlag;

bool bigLeftRG, bigLeftBK, bigRightRG, bigRightBK;
bool smallBKFlag;

long startListenTime;

int v_op = 40;

byte controlByte = 0;
byte DAT1 = 1, DAT0 = 0, DEC_ = 0;

#define IDEL 0
#define CAL 1
#define MAZE 2
#define SOS 3

void setup() {
  Serial.begin(19200);
  lcd.begin(20,4);
 	lcd.backlight();
	lcd.clear();  
  touched = false;
  clapped = false;
  firstTime = true;
  endFlag = false;

  bigLeftRG = false;
  bigLeftBK = false;
  bigRightRG = false;
  bigRightBK = false;
  smallBKFlag = false;
  flagBKforNextLine = false;

  attachInterrupt(digitalPinToInterrupt(2),ISR_capTouch,RISING);
  attachInterrupt(digitalPinToInterrupt(3),ISR_clapSnap,RISING);
}

void loop() {
  lcd_info();
  touched = false;
  clapped = false;
  firstTime = true;

  switch(currState){
    case 0:
      IDLE_State();
      break;
    case 1:
      CAL_State();
      break;
    case 2:
      MAZE_State();
      break;
    case 3:
      SOS_State();
      break;
    default:
      IDLE_State();
      break;
    }
}

// ISRs. Set up so that capTouch and clap/snap is done from 1st principles on
// Nano Every. Nano does SNC and NAVCON. Very easy :)
void ISR_capTouch(){
  touched = true;
  return;
}

void ISR_clapSnap(){
  clapped = true;
  return;
}

// helper functions
void lcd_info(){
// prints to LCD. used for debugging,
// but won't be part of final car.
  lcd.clear();    
    lcd.setCursor(0,1);
    lcd.print("STATE:");
    switch (currState){
    case 0:
            lcd.print("IDLE");
            break;
    case 1:
            lcd.print("CAL ");
            break;
    case 2:
            lcd.print("MAZE");
            break;
    default: lcd.print("SOS");
    }
}

void writeData(){
  // writes ctrl, DAT1, DAT0 to HUB
  Serial.write(controlByte);
  Serial.write(DAT1);
  Serial.write(DAT0);
  Serial.write(DEC_);
}

void readData(){
  while (Serial.available()<4){
    //wait
  }
  if (Serial.available()>=4){
    // if 4 bytes available, reads the 4 bytes from
    // HUB to my 4 respective bytes.
    controlByte = byte(Serial.read());
    DAT1 = byte(Serial.read());
    DAT0 = byte(Serial.read());
    DEC_ = byte(Serial.read());
  }
}

void getColours(){ 
  // maps DATA = <DAT1:DAT0> to krkt colours
  // and puts them in colours array
  byte s1,s2,s3_1,s3_2,s4,s5;

  s1 = DAT1 &   0b01110000;
  s2 = DAT1 &   0b00001110;
  s3_1 = DAT1 & 0b00000001;
  s3_2 = DAT0 & 0b11000000;
  s4 = DAT0 &   0b00111000;
  s5 = DAT0 &   0b00000111;
  
  switch (s1){
    case 0b0000000:
      colors[0] = 'W';
      break;
    case 0b0010000:
      colors[0] = 'R';
      break;
    case 0b0100000:
      colors[0] = 'G';
      break;
    case 0b0110000:
      colors[0] = 'B';
      break;
    case 0b1000000:
      colors[0] = 'K';
      break;
    default:
      colors[0] = 'X';
  }

  switch (s2){
    case 0b0000:
      colors[1] = 'W';
      break;
    case 0b0010:
      colors[1] = 'R';
      break;
    case 0b0100:
      colors[1] = 'G';
      break;
    case 0b0110:
      colors[1] = 'B';
      break;
    case 0b1000:
      colors[1] = 'K';
      break;
    default:
      colors[1] = 'X';
  }

  switch (s3_2){
    case 0b00000000:
      if (s3_1==0b0){
          colors[2] = 'W';
      } else {
          colors[2] = 'K';
      }
      break;
    case 0b01000000:
      colors[2] = 'R';
      break;
    case 0b10000000:
      colors[2] = 'G';
      break;
    case 0b11000000:
      colors[2] = 'B';
      break;
    default:
      colors[2] = 'X';
  }

  switch (s4){
    case 0b000000:
      colors[3] = 'W';
      break;
    case 0b001000:
      colors[3] = 'R';
      break;
    case 0b010000:
      colors[3] = 'G';
      break;
    case 0b011000:
      colors[3] = 'B';
      break;
    case 0b100000:
      colors[3] = 'K';
      break;
    default:
      colors[3] = 'X';
  }

  switch (s5){
    case 0b000:
      colors[4] = 'W';
      break;
    case 0b001:
      colors[4] = 'R';
      break;
    case 0b010:
      colors[4] = 'G';
      break;
    case 0b011:
      colors[4] = 'B';
      break;
    case 0b100:
      colors[4] = 'K';
      break;
    default:
      colors[3] = 'X';
  }
}

void printColours(){
  getColours();
  lcd.setCursor(0,0);
  lcd.print("COLOR:");
  for (int k=0;k<5;k++){
    lcd.print(colors[k]);
  }
}

void setNextState(int pro_state){
  prevState = currState;
  currState = pro_state;
}


void tellNoTouchNoClap(){
  controlByte = 0b10010001; // listening for clap. 2-1-1
  DAT1 = 0b00000000; // no clap
  DAT0 = 0b00000000;
  DEC_ = 0b00000000;
  writeData();
    
  controlByte = 0b10010010; // listening for touch. 2-1-2
  DAT1 = 0b00000000; // no touch
  DAT0 = 0b00000000;
  DEC_ = 0b00000000;
  writeData();

  return;
}

void getAndPrintDiagnostics(){
	readData();
	// controlByte = 0b0;
	while (controlByte!=0b10110010){//2-3-2 SS incAngle
		switch (controlByte){  
			case 0b10100010:{  // 2-2-2 - MDPS  rotation
				rotAngle = (DAT1<<8)|(DAT0);
				rotDir = (DEC_==2);
				// int angle = (DAT1<<8)|(DAT0);
				// bool dir = (DEC_==2);
				lcd.setCursor(11,0);
				if (rotDir){
				lcd.print("rot:+   ");
				} else {
				lcd.print("rot:-   ");
				}
        lcd.setCursor(15,0);
				lcd.print(rotAngle);
				// lcd.print(char(223));
				break;
			}
			case 0b10100011:{// 2-2-3 MDPS speed
				lcd.setCursor(0,2);
				lcd.print("Vl:  ");
				v_l = DAT0;
				lcd.setCursor(3,2);
				lcd.print(v_l);

				lcd.setCursor(8,2);
				lcd.print("Vr:  ");
				v_r = DAT1;
				lcd.setCursor(11,2);
				lcd.print(v_r);

				lcd.print(" mm/s");
				break;
			}
			case 0b10100100:{//2-2-4 MDPS distance
				dist = (DAT1<<8)|(DAT0);
				// int dist = (DAT1<<8)|(DAT0);
				lcd.setCursor(0,3);
				lcd.print("DIST:          ");
        lcd.setCursor(5,3);
				lcd.print(dist);
				lcd.print(" mm");
				break;
			}
			case 0b10110001:{//Colors: 2-3-1 SS
				printColours();
				break; 
			}
			case 0b10110011:{//2-3-3 SS EOM
				////ignore
				// setNextState(IDEL);
				// lcd.clear();
				// return;
				break;      
			}
		}
		readData();
	}
	// done for receiving 2-3-2
	lcd.setCursor(11,1);
	lcd.print("INC:   ");
  lcd.setCursor(15,1);
	incAngle = DAT1;
	lcd.print(incAngle);
}


//states
void IDLE_State(){
  touched = false;

  if (firstTime){
    controlByte = B00010000;//In IDLE state, touch not yet detected.
    DAT1 = B00000000;
    DAT0 = B00000000;
    DEC_ = B00000000;
    writeData();
  }
  
  while(!touched){
  }
  touched = false;
  controlByte = B00010000;
  DAT1 = B00000001;
  DAT0 = B00000000;
  DEC_ = B00000000;
  writeData();
  setNextState(CAL);
  return;
}

void CAL_State(){
  if (firstTime){
    firstTime = false;
    controlByte = B01010000;
    DAT1 = B00000000;
    DAT0 = B00000000;
    DEC_ = B00000000;
    writeData();
  }
  // get eoc from SS:
  //ctrl: 01 11 0000 (1-3-0) DATA, DEC_ is don't cares
  while(controlByte!=0b01110000){// SS EoC
    readData();
  }
  // lcd.setCursor(0,1);
  // lcd.print("SS EoC");

  // ctrl 1-2-0, MDPS EoC message
  while (controlByte!=0b01100000){// MDPS EoC, also gets wheel speeds.
    readData();
  }
  // lcd.setCursor(7,1);
  // lcd.print("MDPS EoC");

  lcd.setCursor(0,2);
  lcd.print("Vl:");
  lcd.print(DAT0);
  lcd.setCursor(8,2);
  lcd.print("Vr:");
  lcd.print(DAT1);
  lcd.print(" mm/s");
  while (controlByte!=0b01110001){//1 3 1
  // weird. HUB sends this message two times,
  // so this loop is here twice as well, idk
    readData();
  }
  controlByte = 0b0;// so second loop can execute.
  while (controlByte!=0b01110001){//1 3 1
    readData();
  }

  printColours();//shows colours on LCD. calls the getColours func as well!

  
  while(!touched){//wait for a touch

  }
  touched = false;
  //tell the hub that a touch was made.
  controlByte = B01010000;
  DAT1 = B00000001;
  DAT0 = B00000000;
  DEC_ = B00000000;
  writeData();
  setNextState(MAZE);// next state transitions.
  return;
}

long listeningTime = 2000;// time to listen for clap/snap etc in maze state
void MAZE_State(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("STATE:MAZE");
  bool firstNavcon = true;
  while ((!touched) && (!clapped)){
    // lcd.clear();
    // listen for a clap,touch immediately after
    lcd.setCursor(19,3);
    lcd.print("C");
    delayMicroseconds(listeningTime*1000);
    if (clapped){
      controlByte = 0b10010001; // listening for clap. 2-1-1
      DAT1 = 0b00000001; // clap
      DAT0 = 0b00000000;
      DEC_ = 0b00000000;
      writeData();
      setNextState(SOS);
      return;
    } else {
      if (firstTime){
      controlByte = 0b10010001; // listening for clap. 2-1-1
      DAT1 = 0b00000000; // no clap
      DAT0 = 0b00000000;
      DEC_ = 0b00000000;
      writeData();
      }
  }
    lcd.setCursor(19,3);
    lcd.print("T");
    delayMicroseconds(listeningTime*1000);
    if (touched){      
      controlByte = 0b10010001; // listening for clap. 2-1-1
      DAT1 = 0b00000000; // no clap
      DAT0 = 0b00000000;
      DEC_ = 0b00000000;
      writeData();

      controlByte = 0b10010010; // listening for touch. 2-1-2
      DAT1 = 0b00000001; // touch
      DAT0 = 0b00000000;
      DEC_ = 0b00000000;
      writeData();
      setNextState(IDEL);
      DAT1 = 0b00000000;
      return;
    } else {
        if (firstTime){
          firstTime = false;
          controlByte = 0b10010010; // listening for touch. 2-1-2
          DAT1 = 0b00000000; // no touch
          DAT0 = 0b00000000;
          DEC_ = 0b00000000;
          writeData();
        }
    }
    lcd.setCursor(19,3);
    lcd.print(" ");
  // navcon stuff here

    if (firstNavcon){
      firstNavcon = false;
      controlByte = 0b10010011;//2-1-3//i am in navcon
      DAT1 = v_op;
      DAT0 = v_op;
      DEC_ = 0;
      writeData();
    }

    
    getAndPrintDiagnostics();

    // NAVCON Stuff below
    /*
    QTP 1:
      Traverse Green/Red at 𝜃𝑖 ≤ 5°
      𝜃𝑖 ≤ 5°? yes = fwd. no?
      𝜃𝑖 ≤ 45° = stop, reverse, stop, rotate, fwd
    QTP 2:
      Detect green/red at 𝜃𝑖 >  45°
      Stop, reverse, stop, rotate, fwd until 𝜃𝑖 ≤ 45°
    */ 

    lcd.setCursor(16,3);
    lcd.print(incAngle);

    // getColours();
    if (colors[0]=='W'&&colors[1]=='W'&&colors[2]=='W'&&colors[3]=='W'&&colors[4]=='W'){ // handles QTP 5
      // flagBKforNextLine = false;
        if (endFlag){
          endFlag = false;
          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = 0;//stop
          DAT0 = 0;
          DEC_ = 0;
          writeData();
          for (int k=0;k<6;k++){
            readData();
          }

          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = 1;//rotate
          DAT0 = 104;
          DEC_ = 3;        
          writeData();
          for (int k=0;k<6;k++){
            readData();
          }

          //go to IDLE State
          setNextState(IDEL);
          return;
      } else {
        tellNoTouchNoClap();
        controlByte = 0b10010011;
        DAT1 = v_op;//fwd
        DAT0 = v_op;
        DEC_ = 0;
        writeData();
      }
    }
    if (colors[1]=='R'&&colors[2]=='R'&&colors[3]=='R'){// QTP 5
      endFlag = true;
    }
    if (colors[0]=='R'||colors[0]=='G'||colors[1]=='R'||colors[1]=='G'||colors[3]=='R'||colors[3]=='G'||colors[4]=='R'||colors[4]=='G'){//QTP 1 and 2
      // if red or green was detected:
      if (flagBKforNextLine){
        if (colors[0]=='G'||colors[1]=='G'||colors[3]=='G'||colors[4]=='G' || colors[0]=='W'||colors[1]=='W'||colors[3]=='W'||colors[4]=='W')
          flagBKforNextLine = false;
          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = v_op;//fwd
          DAT0 = v_op;
          DEC_ = 0;
          writeData();
          continue;
      }
      // QTP 2: R/G, theta>45
      if (colors[0]=='R'||colors[0]=='G'){
        bool onlyLeft = true;
        for (int k=1;k<5;k++){
          if (colors[k]=='R'||colors[k]=='G'){
            onlyLeft = false;
          }
        }
        if (onlyLeft){
          if (bigLeftRG){
            bigLeftRG = false;
            //stop, reverse, stop, CCW 5, fwd, use continue statement to go to next while loop
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();

            for (int k=0;k<6;k++){
              readData();
            }
            // getAndPrintDiagnostics();


            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//reverse
            DAT0 = v_op;
            DEC_ = 1;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//rotate
            DAT0 = 5;
            DEC_ = 2;   
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
          } else {
            bigLeftRG = true;
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
          }
          continue;
        }
      }
      bigLeftRG = false;

      if (colors[4]=='R'||colors[4]=='G'){
        bool onlyRight = true;
        for (int k=0;k<4;k++){
          if (colors[k]=='R'||colors[k]=='G'){
            onlyRight = false;
          }
        }
        if (onlyRight){
          if (bigRightRG){
            bigRightRG = false;
            //stop, reverse, stop, CCW 5, fwd, use continue statement to go to next while loop
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();

            for (int k=0;k<6;k++){
              readData();
            }
            // getAndPrintDiagnostics();


            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//reverse
            DAT0 = v_op;
            DEC_ = 1;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//rotate
            DAT0 = 5;
            DEC_ = 3;   
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
          } else {
            bigRightRG = true;
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
          }
          continue;
        }
      }
      bigRightRG = false;
     
      if (incAngle<=5){
        tellNoTouchNoClap();
        controlByte = 0b10010011;
        DAT1 = v_op;
        DAT0 = v_op;
        DEC_ = 0;
        writeData();
        // for (int k=0;k<6;k++){
        //   readData();
        // }
        // getAndPrintDiagnostics();
      } else {
        if (incAngle<=45 && incAngle>5){
          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = 0;//stop
          DAT0 = 0;
          DEC_ = 0;
          writeData();

          for (int k=0;k<6;k++){
            readData();
          }
          // getAndPrintDiagnostics();
          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = v_op;//reverse
          DAT0 = v_op;
          DEC_ = 1;
          writeData();
          for (int k=0;k<6;k++){
            readData();
          }

          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = 0;//stop
          DAT0 = 0;
          DEC_ = 0;
          writeData();
          for (int k=0;k<6;k++){
            readData();
          }

          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = incAngle>>8;//rotate
          DAT0 = incAngle;
          if (colors[0]=='R'||colors[0]=='G'||colors[1]=='R'||colors[1]=='G'){
            DEC_ = 2;//on left side, set DEC to 2 for CCW
          } else {
            DEC_ = 3;
          }          
          writeData();
          for (int k=0;k<6;k++){
            readData();
          }


          tellNoTouchNoClap();
          controlByte = 0b10010011;
          DAT1 = v_op;//fwd
          DAT0 = v_op;
          DEC_ = 0;
          writeData();
          // for (int k=0;k<6;k++){
          //   readData();
          // }
          // getAndPrintDiagnostics();
        }
      }
    }
    if (colors[0]=='B'||colors[0]=='K'||colors[1]=='B'||colors[1]=='K'||colors[3]=='B'||colors[3]=='K'||colors[4]=='B'||colors[4]=='K'){
      if (flagBKforNextLine){
        flagBKforNextLine = false;

        tellNoTouchNoClap();
        controlByte = 0b10010011;
        DAT1 = 0;//stop
        DAT0 = 0;
        DEC_ = 0;
        writeData();

        for (int k=0;k<6;k++){
          readData();
        }
        // getAndPrintDiagnostics();


        tellNoTouchNoClap();
        controlByte = 0b10010011;
        DAT1 = v_op;//reverse
        DAT0 = v_op;
        DEC_ = 1;
        writeData();
        for (int k=0;k<6;k++){
          readData();
        }

        tellNoTouchNoClap();
        controlByte = 0b10010011;
        DAT1 = 0;//stop
        DAT0 = 0;
        DEC_ = 0;
        writeData();
        for (int k=0;k<6;k++){
          readData();
        }

        tellNoTouchNoClap();
        controlByte = 0b10010011;
        DAT1 = 0;//rotate
        DAT0 = 180;
        DEC_ = 3;   
        writeData();
        for (int k=0;k<6;k++){
          readData();
        }
        tellNoTouchNoClap();
        controlByte = 0b10010011;
        DAT1 = v_op;//fwd
        DAT0 = v_op;
        DEC_ = 0;
        writeData();
      }

      //QTP 4
      // If it is only the far left sensor, and it is the first time seeing this, go fwd. else, do:
      // stop, reverse, stop, turn, fwd, 
      if (colors[0]=='B'||colors[0]=='K'){
        bool onlyLeft = true;
        for (int k=1;k<5;k++){
          if (colors[k]=='B'||colors[k]=='K'){
            onlyLeft = false;
          }
        }
        if (onlyLeft){
          if (bigLeftBK){
            bigLeftBK = false;
            //stop, reverse, stop, CW 5, fwd, use continue statement to go to next while loop
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();

            for (int k=0;k<6;k++){
              readData();
            }
            // getAndPrintDiagnostics();


            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//reverse
            DAT0 = v_op;
            DEC_ = 1;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//rotate
            DAT0 = 5;
            DEC_ = 3;   
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
          } else {
            bigLeftBK = true;
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
          }
          continue;
        }
      }
      bigLeftBK = false;

      if (colors[4]=='B'||colors[4]=='K'){
        bool onlyRight = true;
        for (int k=0;k<4;k++){
          if (colors[k]=='B'||colors[k]=='K'){
            onlyRight = false;
          }
        }
        if (onlyRight){
          if (bigRightBK){
            bigRightBK = false;
            //stop, reverse, stop, CCW 5, fwd, use continue statement to go to next while loop
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }
            // getAndPrintDiagnostics();


            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//reverse
            DAT0 = v_op;
            DEC_ = 1;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//rotate
            DAT0 = 5;
            DEC_ = 2;   
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
            bigRightBK = false;
          } else {
            bigRightBK = true;
            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
          }
          continue;
        }
      }
      bigRightBK = false;  


      //QTP 3
      if (incAngle<45){//fix
        flagBKforNextLine = true;
        tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }
            // getAndPrintDiagnostics();


            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//reverse
            DAT0 = v_op;
            DEC_ = 1;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//stop
            DAT0 = 0;
            DEC_ = 0;
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = 0;//rotate
            if (colors[0]=='B'||colors[1]=='B'||colors[0]=='K'||colors[1]=='K'){
              DAT0 = 90-incAngle;
            } else {
              DAT0 = 90+incAngle;
            }
            // DAT0 = 5;
            DEC_ = 3;   
            writeData();
            for (int k=0;k<6;k++){
              readData();
            }

            tellNoTouchNoClap();
            controlByte = 0b10010011;
            DAT1 = v_op;//fwd
            DAT0 = v_op;
            DEC_ = 0;
            writeData();
      }

    }
  }

  // lcd.setCursor(12,3);
  // lcd.print("End Loop");
  // last test to send to next states
  if (clapped){
      controlByte = 0b10010001; // listening for clap. 
      DAT1 = 0b00000001; // clap
      DAT0 = 0b00000000;
      DEC_ = 0b00000000;
      writeData();
      setNextState(SOS);
      return;
    } else {
      controlByte = 0b10010001; // listening for clap. 
      DAT1 = 0b00000000; // clap
      DAT0 = 0b00000000;
      DEC_ = 0b00000000;
      writeData();
      // setNextState(SOS);
      // return;
    }
    if (touched){
      controlByte = 0b10010010; // listening for touch. 
      DAT1 = 0b00000001; // touch
      DAT0 = 0b00000000;
      DEC_ = 0b00000000;
      writeData();
      setNextState(IDEL);
      DAT1 = 0b00000000;
      return;
    }
}

void SOS_State(){
  clapped = false;
  // 3-1-0, in SOS State, listening for clap.
  controlByte = 0b11010000;
  DAT1 = 0b00000000;// no clap yet
  DAT0 = 0b00000000;
  DEC_ = 0b00000000;
  writeData();

  lcd.setCursor(19,0);
  lcd.print("C");
  while (!clapped){
    
  }
  clapped = false;

  controlByte = 0b11010000;
  DAT1 = 0b00000001;
  DAT0 = 0b00000000;
  DEC_ = 0b00000000;
  writeData();
  
  setNextState(MAZE);
  DAT1 = 0;
  return;
}