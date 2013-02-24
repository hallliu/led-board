const int horizpins[] = {2,3,4,5,6,7,8,9,10};
const int data = A0;
const int mr = A1;
const int clock = A2;
const int latch = A3;
const int oe = A4;
const int pause=A5;
const int vertdat[] = {A0,A1,A2,A3,A4};
const int boardSize = 96;
const unsigned long scroll_delay_ms = 80L;

int lengths[4];
int brightnesses[4];
int scrollspeeds[4];
byte *lightdat[9][4];
byte lights[9][12];
int firstRun;
int rowIter;
byte paused=0;

void setup() {
  for(int i=0; i<9; i++) {
    pinMode(horizpins[i],OUTPUT);
    digitalWrite(horizpins[i],HIGH);
  }
  for(int i=0; i<5; i++) {
    pinMode(vertdat[i],OUTPUT);
  }
  for(int i=0; i<4; i++) {
    lengths[i]=0;
    brightnesses[i]=0;
    scrollspeeds[i]=0;
  }
  pinMode(pause,INPUT_PULLUP);
  Serial.begin(19200);
  firstRun = 1;
  rowIter=0;
}

void loop() {
  if(firstRun==0) {
    for(int m=0; m<4; m++) {
      int scrollsize = boardSize+lengths[m]*8;
      
      for(int i=0; i<9; i++) {
        for(int j=0; j<12; j++) {
          lights[i][j]=0;
        }
      }
      
      for(int i=0; i<scrollsize; i++) {
        for(int j=0; j<9; j++) {
          for(int k=0; k<11; k++) {
            lights[j][k] <<= 1;
            lights[j][k] |= (lights[j][k+1] >> 7);
          }
          lights[j][11] <<= 1;
          if(i/8<lengths[m]) {
            lights[j][11] |= bitRead(lightdat[j][m][i/8],(7-i%8));
          }
        }
        lightup(brightnesses[m],scrollspeeds[m]);
        if(Serial.available()) break;
      }
      if(Serial.available()) break;
    }
  }
  //idle if no input yet
  else {
    for(int i=0; i<9; i++) {
      for(int j=0; j<96; j++) {
        digitalWrite(latch,LOW);
        digitalWrite(oe,HIGH);
        digitalWrite(mr,LOW);
        digitalWrite(mr,HIGH);
        for(int k=0; k<12; k++) {
          if (k==(j/8)) shiftOut(data,clock,MSBFIRST,(1<<(j%8)));
          else shiftOut(data,clock,MSBFIRST,0);
        }
        digitalWrite(oe,LOW);
        digitalWrite(latch,HIGH);
        digitalWrite(horizpins[i],LOW);
        delay(15);
        digitalWrite(horizpins[i],HIGH);
        delay(25);
        if(Serial.available()) return;
      }
    }
  }
}


void serialEvent() {
  firstRun = 0;
  delay(10);
  char params = Serial.read();
  unsigned int msg_ind = int(params>>6);
  if(!firstRun) for(int i=0; i<9; i++) free(lightdat[i][msg_ind]);
  brightnesses[msg_ind]=((int(params & 63) >> 3)+1)*100;
  scrollspeeds[msg_ind]= 40+(7-int(params & 0x07))*10;
  delay(5);
  lengths[msg_ind]=Serial.read();
  delay(50);
  for(int i=0; i<9; i++) lightdat[i][msg_ind] = (byte*) malloc(lengths[msg_ind]);
  for(int i=0; i<9; i++) {
    for(int j=0; j<lengths[msg_ind]; j++) {
      if(Serial.available()) {
        lightdat[i][msg_ind][j] = byte(Serial.read());
      }
      else {
        j--;
        delay(50);
      }
    }
  }
}

void err_dump_num(char k) {
  while(1) {
    digitalWrite(latch,LOW);
    digitalWrite(oe,HIGH);
    digitalWrite(mr,LOW);
    digitalWrite(mr,HIGH);
    for(int j=0; j<12; j++) {
      if(j==0)
        shiftOut(data,clock,LSBFIRST,k);
      else
        shiftOut(data,clock,LSBFIRST,0);
    }
    digitalWrite(oe,LOW);
    digitalWrite(latch,HIGH);
    digitalWrite(horizpins[0],LOW);
    delayMicroseconds(500);
    digitalWrite(horizpins[0],HIGH);
  }
}
  
void lightup(int brightness, int scroll_delay_ms) {
  unsigned long begin_t = millis();
  if(digitalRead(pause)==LOW) paused=1;
  while(millis()<=begin_t+scroll_delay_ms || paused) {
    digitalWrite(latch,LOW);
    digitalWrite(oe,HIGH);
    digitalWrite(mr,LOW);
    digitalWrite(mr,HIGH);
    for(int j=0; j<12; j++) {
      shiftOut(data,clock,LSBFIRST,lights[rowIter][j]);
    }
    digitalWrite(oe,LOW);
    digitalWrite(latch,HIGH);
    digitalWrite(horizpins[rowIter],LOW);
    delayMicroseconds(brightness);
    digitalWrite(horizpins[rowIter],HIGH);
    rowIter = (rowIter+1)%9;
    if(digitalRead(pause)==HIGH && paused) {
      paused=0;
      break;
    }
    delayMicroseconds(1000-brightness);
  }
  return;
}
