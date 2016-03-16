
#include "FreqPeriod.h"       // http://interface.khm.de/index.php/lab/interfaces-advanced/frequency-measurement-library/
#include <MIDI.h> // http://playground.arduino.cc/Main/MIDILibrary    
#include <Adafruit_QDTech.h> // Hardware-specific library
#define sclk 13  // Don't change
#define mosi 11  // Don't change
#define cs   9
#define dc   8
#define rst  12  // you can also connect this to the Arduino reset
Adafruit_QDTech display = Adafruit_QDTech(cs, dc, rst);  // Invoke custom library
//GLOBALS
const int buttonPin = 2;
const int chans = 10;
int notedetection = 10;
const int arraysize = 32;
int8_t notebank[chans][arraysize];  //main 2d note array
int8_t notemem[chans][4];           //sequence note-off memory array
int8_t melodymem[chans][3];         //melody array
int8_t chanctrl[chans][2];          //volume and pan array
int8_t chanplay[chans];             //channel play flag
int col1 = 0;
int index = 0;
int counter = 0;
int8_t note = 0;
int cycle;
int TC = 0;
double lfrq;
long int pp;
int scale = 0;
int channel = 0;
int drums = 1;
int articulations = 1;   // set to 1 if kontakt requires them
int beat = 3;
int tempodelay = 15;
int8_t triad[3] = {0, 4, 7};

void setup() {
  display.init();
  display.setRotation(1);  // 0 - Portrait, 1 - Lanscape
  display.setTextSize(4);
  display.setTextColor(QDTech_GREEN, QDTech_BLACK);
  display.fillScreen(QDTech_BLACK);
  MIDI.begin(4);                  // Start serial port at 31250 baud
  FreqPeriod::begin();            // Start the comparitor for period detection
  randomSeed(analogRead(0));      // Just to randomize the random
  // Send out note off for any hung notes :)
  for (int ch = 0; ch < chans; ch++) {
    MIDI.sendControlChange(123, 0, ch + 1);
    delay(10);
    MIDI.sendControlChange(121, 0, ch + 1);
    delay(10);
    chanctrl[ch][0] = 64; // volumes
    chanctrl[ch][1] = 64; // pans
    chanplay[ch] = 1;
    delay(10);
  }
  // Intro Screen Display
  for (int n = 6; n > 1; n--) {
    display.setTextSize(n + 1);
    for (int x = 0; x < display.height(); x += n) {
      display.drawFastHLine(0, x, display.width(), random(0xFFFF));
      display.setTextColor(random(0xFFFF)*x % display.height());
      BarMessage(35 + (random(3) - 2), display.height() / 2 - 40 + (random(3) - 2), "MIDI");
      BarMessage(35 + (random(3) - 2), display.height() / 2 + (random(3) - 2), "GRID");
      int nut = x % 56 + 36;
      MIDI.sendNoteOn(nut, random(70, 100), x % chans );
      delay(10);
      MIDI.sendNoteOff(nut, random(70, 100), x % chans );
    }
  }
  pinMode(notedetection, OUTPUT);
  pinMode(buttonPin, INPUT);
  display.fillScreen(QDTech_BLACK);
  display.setTextSize(1);
  //Set up starting display grid
  for (int ch = 0; ch < chans; ch++) {
    for (int i = 0; i < arraysize; i++) {
      display.drawFastVLine(ch * 16, 0, display.height(), 0x632C);
      display.drawFastHLine(0, i * 4, display.width(), 0x632C);
      display.fillRect((ch * 16), (i * 4) + 1, 16, 3, random(0xFFFF));

    }
  }
  // end of setup
}


void loop() {
  channel = random(chans);
  if (counter % 8 == 0) {
    tempodelay = analogRead(4);
  }
  //tempo control via pot on A4
  delay(tempodelay);
  //count the activity on the channel track
  TC = TrackCount(channel);

  digitalWrite(notedetection, HIGH);

  //Drums
  if (drums) {
    if (index % (random(2, 4)) == 0) { // HH
      MIDI.sendNoteOn(42, random(40, 70), 10);
      //MIDI.sendNoteOff(42, random(40, 70), 10);
    }
    if (index % 4 == 0) {  // KICK
      MIDI.sendNoteOn(36, random(70, 110), 10);
      //MIDI.sendNoteOff(36, random(70, 110), 10);
    }
    if (index % (random(8, 10)) == 0) { // SNARE
      MIDI.sendNoteOn(38, random(70, 100), 10);
      //MIDI.sendNoteOff(38, random(70, 100), 10);
    }
  }

  display.drawFastHLine(0, index * 4, display.width(), QDTech_GREY + (cycle << 4));
  display.drawFastVLine(channel * 16, 0, display.height(), QDTech_GREY + (QDTech_YELLOW * (cycle << 4)));
  for (int ch = 0; ch < chans; ch++) {

    notemem[ch][0] = notebank[ch][index];
    //Display Note Detection Squares

    FixScale(scale, ch);

    if (random(10) > 7) {
      MIDI.sendNoteOff(notemem[ch][1], 100, ch + 1);
    }

    if (random(10) > 7) {
      MIDI.sendNoteOff(notemem[ch][2], 100, ch + 1);
    }

    MIDI.sendNoteOff(notemem[ch][3], 100, ch + 1);

    if (chanplay[ch] == 1 ) {
      if (drums) {
        MIDI.sendNoteOn(notemem[ch][0], random(20, 127), ch + 1);
      }
      notemem[ch][3] = notemem[ch][2];
      notemem[ch][2] = notemem[ch][1];
      notemem[ch][1] = notemem[ch][0];

      //acivity display of midi outs

      if (notemem[ch][0] > 20) {
        if (chanplay[ch]) {
          display.fillRect((ch * 16), (index * 4) + 1, map(chanctrl[ch][0], 0, 127, 0, 16), 3, notemem[ch][0] * 0xFF0 * (cycle + 1));
          display.fillRect(((beat - 1) * 16), 1, 16, 1, 0xFFFF * (index % 2));
        }
        else {
          display.fillRect((ch * 16), (index * 4) + 1, map(chanctrl[ch][0], 0, 127, 0, 16), 3, 0x00FF);
        }
      }
      else {
        notebank[ch][index] = 0;
        display.fillRect((ch * 16) + 1, (index * 4) + 1, 15, 3, 0);
        display.fillRect(((beat - 1) * 16), 1, 16, 1, 0xFFFF * (index % 2));

      }
    }
    //end of the channel scan for loop
  }


  //incidental melody line

  int ccc = index % chans;
  if (counter % (beat / 2) == 0 && TC < (arraysize / 4)) {
    melodymem[ccc][0] = (notebank[ccc][index] + TC) ;
    //additional 'on beat' notes by listening every 4th cycle
    if (counter > 0  && TC < (arraysize / 4)
        && notebank[channel][index] == 0
        && counter % beat == 0) {
      //&& cycle % 2 == 0 ) {
      note = GetNote();
      //if (note > 0) {
      notebank[channel][index] = note;
      melodymem[ccc][index] = note;
      //}
    }
    int tempstore = notemem[ccc][0];
    notemem[ccc][0] = melodymem[ccc][0];
    FixScale(scale, ccc);
    melodymem[ccc][0] = notemem[ccc][0];
    notemem[ccc][0] = tempstore;
    //melodymem[channel][0] = notemem[random(chans)][0] + 12;
    MIDI.sendNoteOff (melodymem[ccc][2], 0, ccc + 1);
    if (notebank[ccc][index] = melodymem[ccc][0]) {
      MIDI.sendNoteOff (melodymem[ccc][0], 0, ccc + 1);
    }

    if (chanplay[ccc] == 1) {
      MIDI.sendNoteOn (melodymem[ccc][0], random(70, 120), ccc + 1);
      melodymem[ccc][2] = melodymem[ccc][1];
      melodymem[ccc][1] = melodymem[ccc][0];
    }
  }
  else {
    MIDI.sendNoteOff (melodymem[ccc][1], 0, ccc + 1);
  }




  //increment then check
  index++;
  if (index > arraysize - 1) {
    index = 0;
    note = GetNote();
    if (note > 20) {
      notebank[channel][cycle] = note + cycle;
    }
    else {
      notebank[channel][cycle] = 0;
    }
    cycle++;
    if (cycle > arraysize - 1) {
      cycle = 0;
    }
  }



  CheckStuff();



  //incriment the main counter
  counter++;

} // end of main loop

void CheckStuff() {


  //1 in 10 look at channel and if 2/3rds busy force notes to beat value
  if  (random(10) > 8 && TrackCount(channel) > (arraysize / 3)) {
    GoWithTheBeat(channel);
  }


  //1 in 10 look at whole grid if 2/3rds busy force notes to beat value
  if  (beat < 6 && random(10) > 8 && AllTrackCount() > ((arraysize * chans) / 2)) {
    AllGoWithTheBeat();
  }

  if (digitalRead(buttonPin) == HIGH) {
    drums = !drums;

  }


  //Change the key and scale every 4th cycle
  if (counter > 0 && counter % 128 == 0) {
    scale = random(8);
    if (counter % 256 == 0) {
      ShiftTracks(random(9) - 8);
    }
  }


  //Swap two tracks and take one down an octave
  if (random(10) > 8 && counter % 128 == 0) {
    SwapTracks(random(chans), random(chans));
  }

  //Randomly erase the busy tracks 50% full
  if (TC > arraysize - (arraysize / 2) && random(10) > 8) {
    WipeTrack(channel);
  }

  //Random track solo/breakdown on the busy track
  if (random(100) > 95 && TC > arraysize - (arraysize / 3)) {
    TrackSolo(channel);
  }

  //reset all tracks to play on 4 cycles
  if (counter > 0 && counter % 128 == 0) {
    TracksOn();
  }

  //copy the first half of the array to the 2nd
  //and randomise the cycle point
  if (counter > 0 && counter % arraysize == 0 && random(100) > 97 && counter % TC == 0) {
    DupeHalfTracks();
    cycle = random(arraysize);
  }

  //randomise the beat pattern evey 2 cycles
  if (counter > 0 && counter % 64 == 0) {
    beat = 1 + random(chans);
  }

  //Random Articulations for Kontakt (if required)
  if (counter % 8 == 0 && articulations) {
    int articnote = random(23, 29);
    MIDI.sendNoteOn (articnote, 1, channel + 1);
    MIDI.sendNoteOff (articnote, 1, channel + 1);
  }

  //Random mixing // Volumes
  if (counter % 2 == 0) {
  }
  if (chanctrl[channel][0] > 50 ) {
    chanctrl[channel][0] -= random(3);
  }
  if (chanctrl[channel][0] < 90) {
    chanctrl[channel][0] += random(3);
  }
  MIDI.sendControlChange(7, chanctrl[channel][0], channel + 1);
  //Random mixing // PANS
  if (counter % 3 == 0) {
    if (chanctrl[channel][1] > 10 ) {
      chanctrl[channel][1] -= random(8) ;
    }
    if (chanctrl[channel][1] < 110) {
      chanctrl[channel][1] += random(8) ;
    }
    MIDI.sendControlChange(10, chanctrl[channel][1], channel + 1);
  }
}

void GoWithTheBeat(int c) {
  for (int a = 0; a < arraysize; a++) {
    if (a % beat) {
      notebank[c][a] = 0;
    }
  }
}

void AllGoWithTheBeat() {
  for (int c = 0; c < chans; c++) {
    GoWithTheBeat(c);
  }
}

void ShiftTracks(int8_t x) {
  for (int8_t cx = 0; cx < chans; cx++) {
    for (int8_t a = 0; a < arraysize; a++) {
      if (notebank[cx][a] > 20) {
        notebank[cx][a] += x;
      }
    }
  }
}

void SwapTracks(int c1, int c2) {
  int temp = 0;
  for (int a = 0; a < arraysize; a++) {
    if (notebank[c1][a] > 12) {
      //notes
      temp = notebank[c1][a];
      notebank[c1][a] = notebank[c2][a];
      notebank[c2][a] = temp - 12;
    }
  }
  //levels
  temp = chanctrl[c1][0];
  chanctrl[c1][0] = chanctrl[c2][0];
  chanctrl[c2][0] = temp;
}




void DupeHalfTracks() {
  for (int8_t cx = 0; cx < chans; cx++) {
    for (int8_t a = 0; a < (arraysize / 2); a++) {
      if (notebank[cx][a] > 20) {
        notebank[cx][(arraysize / 2) + a] = notebank[cx][a];
      }
    }
  }
}

void TrackSolo(int8_t cn) {
  for (int8_t n = 0; n < chans; n++) {
    if (n != cn) {
      if (random(10) > 7) {
        chanplay[n] = 0;
        MIDI.sendControlChange(123, 0, cn + 1);
      }
    }
    else {
      chanplay[n] = 1;
    }
  }
}

void TracksOn() {
  for (int n = 0; n < chans; n++) {
    chanplay[n] = 1;
  }
}

int AllTrackCount() {
  int tc = 0;
  for (int c = 0; c < chans; c++) {
    tc += TrackCount(c);
  }
  return tc;
}


int TrackCount(int ct) {
  int nc = 0;
  for (int n = 0; n < arraysize ; n++) {
    if (notebank[ct][n] > 20) {
      nc++;
    }
  }
  return nc;
}

void WipeTrack(int cw) {
  for (int n = 0; n < arraysize ; n++) {
    if (notebank[cw][n] > 0) {
      notebank[cw][n] = 0;
    }
  }
}

float GetNote() {
  float gnote = 0.0;
  pp = FreqPeriod::getPeriod();
  if (pp > 100 ) {
    lfrq = 16000000.0 / (pp);  // 16000400 Mhz - fine tuning
    gnote = log(lfrq / 440.0) / log(2) * 12 + 69;
    //float bend = log(lfrq / 440.0) / log(2) * 1200;
    //float bendval = 8192 * bend / 200 ;
    if (gnote > 0 ) {
      digitalWrite(notedetection, LOW);
    }
    return int8_t(gnote);
  }
  delay(2); // to compensate for small delay when reading notes.
  digitalWrite(notedetection, HIGH);
}

void BarMessage(int x, int y, String m) {
  display.setCursor(x, y);
  display.print(m);
}

void FixScale(int scale, int c) {
  switch (scale) {
    case 0: //Harmonic Minor
      if ((notemem[c][0]) % 12 == 4
          || (notemem[c][0]) % 12 == 6
          || (notemem[c][0]) % 12 == 9)
      {
        notemem[c][0]--;
      }
      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 10)
      {
        notemem[c][0]++;
      }
      break;

    case 1: //Harmonic Major
      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 2
          || (notemem[c][0]) % 12 == 6
          || (notemem[c][0]) % 12 == 9)
      {
        notemem[c][0]--;
      }
      if ((notemem[c][0]) % 12 == 10)
      {
        notemem[c][0]++;
      }
      break;

    case 2: //Diminished
      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 4
          || (notemem[c][0]) % 12 == 7
          || (notemem[c][0]) % 12 == 9
          || (notemem[c][0]) % 12 == 11)
      {
        notemem[c][0]--;
      }
      break;

    case 3: //Blues
      if ((notemem[c][0]) % 12 == 2 || (notemem[c][0]) % 12 == 6) {
        notemem[c][0] ++;
      }

      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 4
          || (notemem[c][0]) % 12 == 8
          || (notemem[c][0]) % 12 == 10)
      {
        notemem[c][0]--;
      }
      break;


    case 4: //Major Pentatonic
      if ((notemem[c][0]) % 12 == 6
          || (notemem[c][0]) % 12 == 11)
      {
        notemem[c][0] -= 2;
      }

      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 3
          || (notemem[c][0]) % 12 == 5
          || (notemem[c][0]) % 12 == 8
          || (notemem[c][0]) % 12 == 10)
      {
        notemem[c][0]--;
      }
      break;

    case 5: //Minor Pentatonic
      if ((notemem[c][0]) % 12 == 2
          || (notemem[c][0]) % 12 == 9)
      {
        notemem[c][0] ++;
      }

      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 4
          || (notemem[c][0]) % 12 == 6
          || (notemem[c][0]) % 12 == 8
          || (notemem[c][0]) % 12 == 11)
      {
        notemem[c][0]--;
      }
      break;

    case 6: //Gypsy
      if ((notemem[c][0]) % 12 == 5)
      {
        notemem[c][0] ++;
      }

      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 4
          || (notemem[c][0]) % 12 == 9
          || (notemem[c][0]) % 12 == 11)
      {
        notemem[c][0]--;
      }
      break;
    case 7: //Augmented
      if ((notemem[c][0]) % 12 == 2
          || (notemem[c][0]) % 12 == 6
          || (notemem[c][0]) % 12 == 10)
      {
        notemem[c][0] ++;
      }

      if ((notemem[c][0]) % 12 == 1
          || (notemem[c][0]) % 12 == 5
          || (notemem[c][0]) % 12 == 9)
      {
        notemem[c][0]--;
      }

  }
}


