#include <FastLED.h>
#include <fix_fft.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define N_Leds 121
#define LED 13
#define AUDIO A0
#define TINT A1
#define DEF_BR 50
#define min_bright 0
#define max_bright 255
#define yres 8

const int  en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3;

// Define I2C Address 
const int i2c_addr = 0x27;

LiquidCrystal_I2C lcd(i2c_addr, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE);

uint8_t empty[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t one[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
uint8_t two[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F};
uint8_t three[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F};
uint8_t four[] = {
  0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F};
uint8_t five[] = {
  0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
uint8_t six[] = {
  0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
uint8_t seven[] = {
  0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
uint8_t eight[] = {
  0x01F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

int i, j, load;
int x, y, z;

int val;
int brightness = DEF_BR;
int helper = 1;
char im[64], data[64];
char data_avgs[32];
float peaks[32];

CRGB leds[N_Leds];

void setup() {

  Serial.begin(9600);
  FastLED.addLeds<WS2812, LED, RGB>(leds, N_Leds);
  FastLED.setBrightness(DEF_BR);

  lcd.begin(16,2);

  lcd.setCursor(1,0);
  lcd.createChar(9, empty);
  lcd.createChar(1, one);
  lcd.createChar(2, two);
  lcd.createChar(3, three);
  lcd.createChar(4, four);
  lcd.createChar(5, five);
  lcd.createChar(6, six);
  lcd.createChar(7, seven);
  lcd.createChar(8, eight);
  

  for(j = 0; j < 256; j++) {
    for(i = 0; i < N_Leds; i++) {
      leds[i] = UpdateColor((i * 256 / N_Leds + j) % 256);   
    } 
    FastLED.show();
    delay(1);    
  } 

  for (i = 0;i < 100; i++)
  {
    for (load = 0; load < i / 5; load++)
    {
      lcd.setCursor(load, 1);
      lcd.write(1);
    }
    if (load < 1)
    {
      lcd.setCursor(0, 1);
      lcd.write(7);
    }

    lcd.setCursor(load + 1, 1);
    lcd.write((i - i / 5 * 5) + 1);
    for (load = load + 2; load < 16; load++)
    {
      lcd.setCursor(load, 1);
      lcd.write(1);
    }
    lcd.setCursor(0, 0);
    lcd.print(" Connecting ... ");
    delay(5);
  }
  lcd.clear();

  pinMode(AUDIO, INPUT);
  pinMode(TINT, INPUT);
  pinMode(LED, OUTPUT);
}

// Reduce valoarea coloanelor cu 1
void reduce(int toReduce){ 
  if (helper == toReduce){
    helper = 0;
    for (x=0; x < 32; x++) {
      peaks[x] = peaks[x] - 1;  // subtract 1 from each column peaks
    }
  }
  helper++;
}

CRGB UpdateColor(int sound) {
  CRGB color (0,0,0);
  if(sound < 85) {
    color.g = 0;
    color.r = ((float)sound * 0.5f / 85.5f) * 256.0f;
    color.b = 255 - color.r;
  } else if(sound < 170) {
    color.g = ((float)(sound - 85) * 1.0f / 85.0f) * 256.0f;
    color.r = 255 - color.g;
    color.b = 0;
  } else if(sound < 256) {
    color.b = ((float)(sound - 170) * 0.3f/ 85.0f) * 255.0f;
    color.g = 255 - color.b;
    color.r = 1;
  }
  return color;
}

void loop() {  

 // Citeste valorile primite de la input
 for (i=0; i < 64; i++){    
    val = ((analogRead(AUDIO) / 4) - 120);
    data[i] = val;                                      
    im[i] = 0; 
  };

  // Trimite datele prin fft
  fix_fft(data,im,5,0);

  // Se calculeaza valoarea absoluta, pentru a avea doar valori pozitive
  for (i=0; i< 32 ;i++){  
    data[i] = sqrt(data[i] * data[i] + im[i] * im[i]); 
  }

  // Reduce valorile maxime in intervalul [0, 8)
  for (i=0; i<32; i++) {
    data_avgs[i] = (data[i]); 
    data_avgs[i] = constrain(data_avgs[i],0,9); 
    data_avgs[i] = map(data_avgs[i], 0, 9, 0, yres); 
  }

  // Reduce valoarea coloanelor
  reduce(1); 

  // Setare lcd 
  lcd.setCursor(0, 1);
  // Sound
  lcd.print("S"); 

  // Repeta pentru fiecare coloana de pe display
  for (x = 1; x < 16; x++) {  

    // Extrage valoarea curenta
    y = data_avgs[x]; 
        z = peaks[x];
    if (y > z){
      peaks[x] = y;
    }
    y = peaks[x]; 

    if (y <= 8){            
      lcd.setCursor(x, 0); 
      lcd.print(" ");
      lcd.setCursor(x, 1); 
      if (y == 0){
        // sterge o linie
        lcd.print(" "); 
      }
      else {
        lcd.write(y);
      }
    }
  }  
 

  int sound = val;
  int tint = analogRead(TINT);
  
  FastLED.setBrightness(brightness);
  
  // Modifica culoarea de baza
  if (tint < 200) {
    sound = abs(val ) + tint / 2;
  } else if (tint >= 200 && tint <400) {
    sound = abs(val) + tint / 2;
  } else {
    sound = abs(val) + tint / 2 * 1.5;
  }

  // Restrange valorile calculate a sunetului
  sound = constrain(sound, 0, 255);

  // Calculeaza culoarea
  leds[N_Leds / 2] = UpdateColor(sound);
  delay(30);
  FastLED.show();
  
  delay(60);
  fill_solid( leds, N_Leds, leds[(N_Leds/2)]);
   
  FastLED.show();

}
