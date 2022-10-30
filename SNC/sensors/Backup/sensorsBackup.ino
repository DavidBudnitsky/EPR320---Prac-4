int input = 9; //pin 2 used for input. Connect cap touch sensor to input pin
int output = 12; //pin 4 used for output

int clapPin = A0;

unsigned long time_lastTouched;
unsigned long time_lastClapped;

int clapSnapFlag = 11;
int capTouchFlag = 10;

bool touched;
bool clapped;
bool was_clapped;
bool clap_out;
bool touch_toggle, clap_toggle;
bool longNoise;

typedef struct Sensor{
  int input;
  int output;
};

Sensor createSensor(int in, int out){
  Sensor s = {in,out};
  pinMode(in,INPUT); 
  pinMode(out,OUTPUT);
  return s;
}

unsigned long measureTime(Sensor* s){
  
  unsigned long start_time = micros(); 

  digitalWrite(s->output,HIGH);
  
  while(digitalRead(s->input) == 0){
    //do nothing
  }
  unsigned long measured_time = micros() - start_time;
  
  digitalWrite(s->output,LOW);
  digitalWrite(s->input,LOW);
  
  return measured_time;
  
}
void setup() {
  // Serial.begin(9600);
  pinMode(clapSnapFlag,OUTPUT);
  pinMode(capTouchFlag,OUTPUT);
  touch_toggle = false;
  touched = false;
  clapped = false;
  was_clapped = false;
  clap_toggle = false;
  clap_out = false;
  longNoise = false;
}

Sensor s = createSensor(input,output);

void loop() {
  // touch stuff
  long dc_time = measureTime(&s);
  // Serial.println(dc_time);
  if (touched){
    // if has been touched, only let another touch register after 2s
    if (millis()-time_lastTouched>2000){
      touched = false;
      }
  } else {
// if hasn't been touched yet
    if (dc_time>40){
      touched = true;
      time_lastTouched = millis();
      // touch_toggle = !touch_toggle;
      digitalWrite(capTouchFlag,HIGH);
      delay(200);           
      digitalWrite(capTouchFlag,LOW); 
    }
  }
  // clap stuff
  float threshold = 3.75;
  float clapValue = analogRead(A0) * (5.0/1023.0);
  // Serial.println(clapValue);
  bool validClap = false;
  if (clapValue>threshold){
    validClap = true;
    delay(100);
    unsigned long startScan = millis();
    while (millis()-startScan<100){
      clapValue = analogRead(A0) * (5.0/1023.0);
      if (clapValue>threshold){
        validClap = false;
        longNoise = true;
        startScan = millis();
      }
    }
  }
  if (validClap){
    digitalWrite(clapSnapFlag,HIGH);
    delay(200);
    digitalWrite(clapSnapFlag,LOW);
    // clap_out = !clap_out;
    // digitalWrite(11, clap_out);
    validClap = false; 
  }
}