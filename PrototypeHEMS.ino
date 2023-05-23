#include <Wire.h>
#include <LiquidCrystal.h>
#include <DHT.h>

#define FIS_TYPE float
#define FIS_RESOLUSION 101
#define FIS_MIN -3.4028235E+38
#define FIS_MAX 3.4028235E+38
typedef FIS_TYPE(*_FIS_MF)(FIS_TYPE, FIS_TYPE*);
typedef FIS_TYPE(*_FIS_ARR_OP)(FIS_TYPE, FIS_TYPE);
typedef FIS_TYPE(*_FIS_ARR)(FIS_TYPE*, int, _FIS_ARR_OP);

#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE); 
#define ldrPin A2
#define irSensor1 A0
#define irSensor2 A1

const int rs = 6, en = 7, d4 = 8, d5 = 9, d6 = 10, d7 =  11;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
uint8_t second, minute, hour, wday, day, month, year, ctrl;
int PirPin = 4;
int pirState = LOW; 
int val = 0;
const int lampPin = 3; //Pin pour la lampe
const int fanPin = 2; // Pin pour le ventilateur
float temperature;
float humidity;
float heure;
float luminosity;
int numberPerson = 0;

// Number of inputs to the fuzzy inference system
const int fis_gcI = 5;
// Number of outputs to the fuzzy inference system
const int fis_gcO = 2;
// Number of rules to the fuzzy inference system
const int fis_gcR = 95;

FIS_TYPE g_fisInput[fis_gcI];
FIS_TYPE g_fisOutput[fis_gcO];

void IN()
{
  numberPerson++;
  lcd.clear();
  lcd.print("Person In Room:");
  lcd.setCursor(0,1);
  lcd.print(numberPerson);
  delay(1000);
}
void OUT()
{
  numberPerson--;
  lcd.clear();
  lcd.print("Person In Room:");
  lcd.setCursor(0,1);
  lcd.print(numberPerson);
  delay(1000);
}

// Setup routine runs once when you press reset:
void setup()
{
    Serial.begin(9600);
    lcd.begin(16, 2);
    dht.begin();
    
    // initialize the pins for input.
    pinMode(irSensor1 , INPUT);
    pinMode(irSensor2 , INPUT);
    pinMode(ldrPin , INPUT);
    pinMode(PirPin , INPUT);

    // initialize the pins for output.
    pinMode(lampPin , OUTPUT);
    pinMode(fanPin , OUTPUT);

}

// Loop routine runs over and over again forever:
void loop()
{
    // Lecture des données du capteur DHT22
    // Récupération de la température et de l'humidité relative
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    // Read Input: Time_of_day
    read_ds1307();
    heure = hour;
    // Read Input: Current_illuminance
    luminosity = analogRead(ldrPin);

    if(digitalRead(irSensor1)){
      IN();
    }
    if(digitalRead(irSensor2)){
      OUT();
    }    
    
    // Read Input: Temperature
    g_fisInput[0] = temperature;
    // Read Input: Relativ_humidity
    g_fisInput[1] = humidity;
    // Read Input: Time_of_day
    g_fisInput[2] = heure;
    // Read Input: Current_illuminance
    g_fisInput[3] = luminosity;
    // Read Input: Current_illuminance
    g_fisInput[4] = numberPerson;

    g_fisOutput[0] = 0;
    g_fisOutput[1] = 0;

    fis_evaluate();

    // Affichage des données de température et d'humidité sur la console série
    Serial.print("Humidity: ");
    Serial.print(g_fisInput[1]);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(g_fisInput[0]);
    Serial.println(" *C");
    Serial.print("Nombre de personne: ");
    Serial.print(g_fisInput[4]);
  
    // Affichage des données de l'heure et du niveau de lumiosité 
    Serial.print("Luminosity: ");
    Serial.print(g_fisInput[3]);
    Serial.print(" LUX");
    Serial.print("Heure: ");
    Serial.print(g_fisInput[2]);
    Serial.println(" H");

    Serial.print("Fan Speed=  ");
    Serial.print(g_fisOutput[0]);
    Serial.print("RPM");
    Serial.println(" ");

    Serial.print("Light intensity =  ");
    Serial.print(g_fisOutput[1]);
    Serial.print("Lumen");
    Serial.println(" ");

    Serial.println("=======================================");
    lcd.setCursor(0,0);
    lcd.print("T=");
    lcd.print(g_fisInput[0]);
    lcd.print("C ");
    lcd.print("H=");
    lcd.print(g_fisInput[1]);
    lcd.println("%");
    lcd.setCursor(9,0);
    
    lcd.setCursor(0,1);
    lcd.print("He=");
    lcd.print(g_fisInput[2]);
    lcd.print("H ");
    lcd.setCursor(10,1);
    lcd.print("L=");
    lcd.print(g_fisInput[3]);
    lcd.print("Lx");

    lcd.print("S=");
    lcd.print(g_fisOutput[0]);
    lcd.print("RM. I=");
    lcd.print(g_fisOutput[1]);
    lcd.print("Lm");
   
    delay(2000);
    lcd.clear();

    // Set output vlaue: Fan_speed
    analogWrite(lampPin , g_fisOutput[0]);
    // Set output vlaue: Lighting_output
    analogWrite(fanPin , g_fisOutput[1]);

}

//**************************************************************************
//Read time of day function
bool read_ds1307()
{

  Wire.beginTransmission(0x68);

  Wire.write(0x00);

  if (Wire.endTransmission() != 0)
    return false;

  Wire.requestFrom(0x68, 8);

  second = bcd2bin(Wire.read());
  minute = bcd2bin(Wire.read());
  hour = bcd2bin(Wire.read());
  wday = bcd2bin(Wire.read());
  day = bcd2bin(Wire.read());
  month = bcd2bin(Wire.read());
  year = bcd2bin(Wire.read());

  ctrl = Wire.read();

  return true;
}

//****************************************************************
//Convert analog read to digital read for arduino
uint8_t bcd2bin(uint8_t bcd)
{
  return (bcd / 16 * 10) + (bcd % 16);
}

//******************************************************************
// Prit time of day
void print_time()
{
  Serial.print("Fecha: ");
  Serial.print(day);
  Serial.print('/');
  Serial.print(month);
  Serial.print('/');
  Serial.print(year);

  Serial.print("  Hora: ");
  Serial.print(hour);
  Serial.print(':');
  Serial.print(minute);
  Serial.print(':');
  Serial.print(second);

  Serial.println();
}

//***********************************************************************

//***********************************************************************
// Support functions for Fuzzy Inference System                          
//***********************************************************************
// Triangular Member Function
FIS_TYPE fis_trimf(FIS_TYPE x, FIS_TYPE* p)
{
    FIS_TYPE a = p[0], b = p[1], c = p[2];
    FIS_TYPE t1 = (x - a) / (b - a);
    FIS_TYPE t2 = (c - x) / (c - b);
    if ((a == b) && (b == c)) return (FIS_TYPE) (x == a);
    if (a == b) return (FIS_TYPE) (t2*(b <= x)*(x <= c));
    if (b == c) return (FIS_TYPE) (t1*(a <= x)*(x <= b));
    t1 = min(t1, t2);
    return (FIS_TYPE) max(t1, 0);
}

// Trapezoidal Member Function
FIS_TYPE fis_trapmf(FIS_TYPE x, FIS_TYPE* p)
{
    FIS_TYPE a = p[0], b = p[1], c = p[2], d = p[3];
    FIS_TYPE t1 = ((x <= c) ? 1 : ((d < x) ? 0 : ((c != d) ? ((d - x) / (d - c)) : 0)));
    FIS_TYPE t2 = ((b <= x) ? 1 : ((x < a) ? 0 : ((a != b) ? ((x - a) / (b - a)) : 0)));
    return (FIS_TYPE) min(t1, t2);
}

FIS_TYPE fis_min(FIS_TYPE a, FIS_TYPE b)
{
    return min(a, b);
}

FIS_TYPE fis_max(FIS_TYPE a, FIS_TYPE b)
{
    return max(a, b);
}

FIS_TYPE fis_array_operation(FIS_TYPE *array, int size, _FIS_ARR_OP pfnOp)
{
    int i;
    FIS_TYPE ret = 0;

    if (size == 0) return ret;
    if (size == 1) return array[0];

    ret = array[0];
    for (i = 1; i < size; i++)
    {
        ret = (*pfnOp)(ret, array[i]);
    }

    return ret;
}


//***********************************************************************
// Data for Fuzzy Inference System                                       
//***********************************************************************
// Pointers to the implementations of member functions
_FIS_MF fis_gMF[] =
{
    fis_trimf, fis_trapmf
};

// Count of member function for each Input
int fis_gIMFCount[] = { 5, 5, 5, 4, 3 };

// Count of member function for each Output 
int fis_gOMFCount[] = { 5, 5 };

// Coefficients for the Input Member Functions
FIS_TYPE fis_gMFI0Coeff1[] = { 0, 0, 12 };
FIS_TYPE fis_gMFI0Coeff2[] = { 8, 14, 20 };
FIS_TYPE fis_gMFI0Coeff3[] = { 16, 21, 26 };
FIS_TYPE fis_gMFI0Coeff4[] = { 24, 30, 35 };
FIS_TYPE fis_gMFI0Coeff5[] = { 30, 35, 40, 40 };
FIS_TYPE* fis_gMFI0Coeff[] = { fis_gMFI0Coeff1, fis_gMFI0Coeff2, fis_gMFI0Coeff3, fis_gMFI0Coeff4, fis_gMFI0Coeff5 };
FIS_TYPE fis_gMFI1Coeff1[] = { 0, 0, 20 };
FIS_TYPE fis_gMFI1Coeff2[] = { 15, 25, 40 };
FIS_TYPE fis_gMFI1Coeff3[] = { 30, 45, 65 };
FIS_TYPE fis_gMFI1Coeff4[] = { 55, 70, 85 };
FIS_TYPE fis_gMFI1Coeff5[] = { 70, 90, 100, 100 };
FIS_TYPE* fis_gMFI1Coeff[] = { fis_gMFI1Coeff1, fis_gMFI1Coeff2, fis_gMFI1Coeff3, fis_gMFI1Coeff4, fis_gMFI1Coeff5 };
FIS_TYPE fis_gMFI2Coeff1[] = { 0, 0, 6 };
FIS_TYPE fis_gMFI2Coeff2[] = { 4, 6, 8 };
FIS_TYPE fis_gMFI2Coeff3[] = { 6, 12, 18 };
FIS_TYPE fis_gMFI2Coeff4[] = { 16, 18, 20 };
FIS_TYPE fis_gMFI2Coeff5[] = { 18, 21, 24, 24 };
FIS_TYPE* fis_gMFI2Coeff[] = { fis_gMFI2Coeff1, fis_gMFI2Coeff2, fis_gMFI2Coeff3, fis_gMFI2Coeff4, fis_gMFI2Coeff5 };
FIS_TYPE fis_gMFI3Coeff1[] = { 0, 0, 250 };
FIS_TYPE fis_gMFI3Coeff2[] = { 150, 300, 500 };
FIS_TYPE fis_gMFI3Coeff3[] = { 400, 600, 800 };
FIS_TYPE fis_gMFI3Coeff4[] = { 700, 850, 1000, 1000 };
FIS_TYPE* fis_gMFI3Coeff[] = { fis_gMFI3Coeff1, fis_gMFI3Coeff2, fis_gMFI3Coeff3, fis_gMFI3Coeff4 };
FIS_TYPE fis_gMFI4Coeff1[] = { 0, 0, 3 };
FIS_TYPE fis_gMFI4Coeff2[] = { 2, 4, 6 };
FIS_TYPE fis_gMFI4Coeff3[] = { 5, 7, 8, 8 };
FIS_TYPE* fis_gMFI4Coeff[] = { fis_gMFI4Coeff1, fis_gMFI4Coeff2, fis_gMFI4Coeff3 };
FIS_TYPE** fis_gMFICoeff[] = { fis_gMFI0Coeff, fis_gMFI1Coeff, fis_gMFI2Coeff, fis_gMFI3Coeff, fis_gMFI4Coeff };

// Coefficients for the Output Member Functions
FIS_TYPE fis_gMFO0Coeff1[] = { 0, 0, 400 };
FIS_TYPE fis_gMFO0Coeff2[] = { 600, 800, 1000 };
FIS_TYPE fis_gMFO0Coeff3[] = { 1100, 1300, 1500, 1500 };
FIS_TYPE fis_gMFO0Coeff4[] = { 300, 500, 700 };
FIS_TYPE fis_gMFO0Coeff5[] = { 900, 1100, 1300 };
FIS_TYPE* fis_gMFO0Coeff[] = { fis_gMFO0Coeff1, fis_gMFO0Coeff2, fis_gMFO0Coeff3, fis_gMFO0Coeff4, fis_gMFO0Coeff5 };
FIS_TYPE fis_gMFO1Coeff1[] = { 0, 0, 250 };
FIS_TYPE fis_gMFO1Coeff2[] = { 150, 350, 550 };
FIS_TYPE fis_gMFO1Coeff3[] = { 450, 650, 900 };
FIS_TYPE fis_gMFO1Coeff4[] = { 800, 1000, 1200 };
FIS_TYPE fis_gMFO1Coeff5[] = { 1100, 1300, 1500, 1500 };
FIS_TYPE* fis_gMFO1Coeff[] = { fis_gMFO1Coeff1, fis_gMFO1Coeff2, fis_gMFO1Coeff3, fis_gMFO1Coeff4, fis_gMFO1Coeff5 };
FIS_TYPE** fis_gMFOCoeff[] = { fis_gMFO0Coeff, fis_gMFO1Coeff };

// Input membership function set
int fis_gMFI0[] = { 0, 0, 0, 0, 1 };
int fis_gMFI1[] = { 0, 0, 0, 0, 1 };
int fis_gMFI2[] = { 0, 0, 0, 0, 1 };
int fis_gMFI3[] = { 0, 0, 0, 1 };
int fis_gMFI4[] = { 0, 0, 1 };
int* fis_gMFI[] = { fis_gMFI0, fis_gMFI1, fis_gMFI2, fis_gMFI3, fis_gMFI4};

// Output membership function set
int fis_gMFO0[] = { 0, 0, 1, 0, 0 };
int fis_gMFO1[] = { 0, 0, 0, 0, 1 };
int* fis_gMFO[] = { fis_gMFO0, fis_gMFO1};

// Rule Weights
FIS_TYPE fis_gRWeight[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

// Rule Type
int fis_gRType[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

// Rule Inputs
int fis_gRI0[] = { 1, 1, 0, 0, 1 };
int fis_gRI1[] = { 1, 1, 0, 0, 2 };
int fis_gRI2[] = { 1, 1, 0, 0, 3 };
int fis_gRI3[] = { 2, 1, 0, 0, 1 };
int fis_gRI4[] = { 2, 1, 0, 0, 2 };
int fis_gRI5[] = { 2, 1, 0, 0, 2 };
int fis_gRI6[] = { 3, 1, 0, 0, 1 };
int fis_gRI7[] = { 3, 1, 0, 0, 2 };
int fis_gRI8[] = { 3, 1, 0, 0, 3 };
int fis_gRI9[] = { 4, 1, 0, 0, 1 };
int fis_gRI10[] = { 4, 1, 0, 0, 2 };
int fis_gRI11[] = { 4, 1, 0, 0, 3 };
int fis_gRI12[] = { 5, 1, 0, 0, 1 };
int fis_gRI13[] = { 5, 1, 0, 0, 2 };
int fis_gRI14[] = { 5, 1, 0, 0, 3 };
int fis_gRI15[] = { 1, 2, 0, 0, 1 };
int fis_gRI16[] = { 1, 2, 0, 0, 2 };
int fis_gRI17[] = { 1, 2, 0, 0, 3 };
int fis_gRI18[] = { 2, 2, 0, 0, 1 };
int fis_gRI19[] = { 2, 2, 0, 0, 2 };
int fis_gRI20[] = { 2, 2, 0, 0, 3 };
int fis_gRI21[] = { 3, 2, 0, 0, 1 };
int fis_gRI22[] = { 3, 2, 0, 0, 2 };
int fis_gRI23[] = { 3, 2, 0, 0, 3 };
int fis_gRI24[] = { 4, 2, 0, 0, 1 };
int fis_gRI25[] = { 4, 2, 0, 0, 2 };
int fis_gRI26[] = { 4, 2, 0, 0, 3 };
int fis_gRI27[] = { 5, 2, 0, 0, 1 };
int fis_gRI28[] = { 5, 2, 0, 0, 2 };
int fis_gRI29[] = { 5, 2, 0, 0, 3 };
int fis_gRI30[] = { 1, 3, 0, 0, 1 };
int fis_gRI31[] = { 1, 3, 0, 0, 2 };
int fis_gRI32[] = { 1, 3, 0, 0, 3 };
int fis_gRI33[] = { 2, 3, 0, 0, 1 };
int fis_gRI34[] = { 2, 3, 0, 0, 2 };
int fis_gRI35[] = { 2, 3, 0, 0, 3 };
int fis_gRI36[] = { 3, 3, 0, 0, 1 };
int fis_gRI37[] = { 3, 3, 0, 0, 2 };
int fis_gRI38[] = { 3, 3, 0, 0, 3 };
int fis_gRI39[] = { 4, 3, 0, 0, 1 };
int fis_gRI40[] = { 4, 3, 0, 0, 2 };
int fis_gRI41[] = { 4, 3, 0, 0, 3 };
int fis_gRI42[] = { 5, 3, 0, 0, 1 };
int fis_gRI43[] = { 5, 3, 0, 0, 2 };
int fis_gRI44[] = { 5, 3, 0, 0, 3 };
int fis_gRI45[] = { 1, 4, 0, 0, 1 };
int fis_gRI46[] = { 1, 4, 0, 0, 2 };
int fis_gRI47[] = { 1, 4, 0, 0, 3 };
int fis_gRI48[] = { 2, 4, 0, 0, 1 };
int fis_gRI49[] = { 2, 4, 0, 0, 2 };
int fis_gRI50[] = { 2, 4, 0, 0, 3 };
int fis_gRI51[] = { 3, 4, 0, 0, 1 };
int fis_gRI52[] = { 3, 4, 0, 0, 2 };
int fis_gRI53[] = { 3, 4, 0, 0, 3 };
int fis_gRI54[] = { 4, 4, 0, 0, 1 };
int fis_gRI55[] = { 4, 4, 0, 0, 2 };
int fis_gRI56[] = { 4, 4, 0, 0, 3 };
int fis_gRI57[] = { 5, 4, 0, 0, 1 };
int fis_gRI58[] = { 5, 4, 0, 0, 2 };
int fis_gRI59[] = { 5, 4, 0, 0, 3 };
int fis_gRI60[] = { 1, 5, 0, 0, 1 };
int fis_gRI61[] = { 1, 5, 0, 0, 2 };
int fis_gRI62[] = { 1, 5, 0, 0, 3 };
int fis_gRI63[] = { 2, 5, 0, 0, 1 };
int fis_gRI64[] = { 2, 5, 0, 0, 2 };
int fis_gRI65[] = { 2, 5, 0, 0, 3 };
int fis_gRI66[] = { 3, 5, 0, 0, 1 };
int fis_gRI67[] = { 3, 5, 0, 0, 2 };
int fis_gRI68[] = { 3, 5, 0, 0, 3 };
int fis_gRI69[] = { 4, 5, 0, 0, 1 };
int fis_gRI70[] = { 4, 5, 0, 0, 2 };
int fis_gRI71[] = { 4, 5, 0, 0, 3 };
int fis_gRI72[] = { 5, 5, 0, 0, 1 };
int fis_gRI73[] = { 5, 5, 0, 0, 2 };
int fis_gRI74[] = { 5, 5, 0, 0, 3 };
int fis_gRI75[] = { 0, 0, 1, 1, 0 };
int fis_gRI76[] = { 0, 0, 1, 2, 0 };
int fis_gRI77[] = { 0, 0, 1, 3, 0 };
int fis_gRI78[] = { 0, 0, 1, 4, 0 };
int fis_gRI79[] = { 0, 0, 2, 1, 0 };
int fis_gRI80[] = { 0, 0, 2, 2, 0 };
int fis_gRI81[] = { 0, 0, 2, 3, 0 };
int fis_gRI82[] = { 0, 0, 2, 4, 0 };
int fis_gRI83[] = { 0, 0, 3, 1, 0 };
int fis_gRI84[] = { 0, 0, 3, 2, 0 };
int fis_gRI85[] = { 0, 0, 3, 3, 0 };
int fis_gRI86[] = { 0, 0, 3, 4, 0 };
int fis_gRI87[] = { 0, 0, 4, 1, 0 };
int fis_gRI88[] = { 0, 0, 4, 2, 0 };
int fis_gRI89[] = { 0, 0, 4, 3, 0 };
int fis_gRI90[] = { 0, 0, 4, 4, 0 };
int fis_gRI91[] = { 0, 0, 5, 1, 0 };
int fis_gRI92[] = { 0, 0, 5, 2, 0 };
int fis_gRI93[] = { 0, 0, 5, 3, 0 };
int fis_gRI94[] = { 0, 0, 5, 4, 0 };
int* fis_gRI[] = { fis_gRI0, fis_gRI1, fis_gRI2, fis_gRI3, fis_gRI4, fis_gRI5, fis_gRI6, fis_gRI7, fis_gRI8, fis_gRI9, fis_gRI10, fis_gRI11, fis_gRI12, fis_gRI13, fis_gRI14, fis_gRI15, fis_gRI16, fis_gRI17, fis_gRI18, fis_gRI19, fis_gRI20, fis_gRI21, fis_gRI22, fis_gRI23, fis_gRI24, fis_gRI25, fis_gRI26, fis_gRI27, fis_gRI28, fis_gRI29, fis_gRI30, fis_gRI31, fis_gRI32, fis_gRI33, fis_gRI34, fis_gRI35, fis_gRI36, fis_gRI37, fis_gRI38, fis_gRI39, fis_gRI40, fis_gRI41, fis_gRI42, fis_gRI43, fis_gRI44, fis_gRI45, fis_gRI46, fis_gRI47, fis_gRI48, fis_gRI49, fis_gRI50, fis_gRI51, fis_gRI52, fis_gRI53, fis_gRI54, fis_gRI55, fis_gRI56, fis_gRI57, fis_gRI58, fis_gRI59, fis_gRI60, fis_gRI61, fis_gRI62, fis_gRI63, fis_gRI64, fis_gRI65, fis_gRI66, fis_gRI67, fis_gRI68, fis_gRI69, fis_gRI70, fis_gRI71, fis_gRI72, fis_gRI73, fis_gRI74, fis_gRI75, fis_gRI76, fis_gRI77, fis_gRI78, fis_gRI79, fis_gRI80, fis_gRI81, fis_gRI82, fis_gRI83, fis_gRI84, fis_gRI85, fis_gRI86, fis_gRI87, fis_gRI88, fis_gRI89, fis_gRI90, fis_gRI91, fis_gRI92, fis_gRI93, fis_gRI94 };

// Rule Outputs
int fis_gRO0[] = { 1, 0 };
int fis_gRO1[] = { 4, 0 };
int fis_gRO2[] = { 4, 0 };
int fis_gRO3[] = { 1, 0 };
int fis_gRO4[] = { 4, 0 };
int fis_gRO5[] = { 2, 0 };
int fis_gRO6[] = { 4, 0 };
int fis_gRO7[] = { 2, 0 };
int fis_gRO8[] = { 5, 0 };
int fis_gRO9[] = { 2, 0 };
int fis_gRO10[] = { 2, 0 };
int fis_gRO11[] = { 5, 0 };
int fis_gRO12[] = { 2, 0 };
int fis_gRO13[] = { 5, 0 };
int fis_gRO14[] = { 3, 0 };
int fis_gRO15[] = { 4, 0 };
int fis_gRO16[] = { 4, 0 };
int fis_gRO17[] = { 2, 0 };
int fis_gRO18[] = { 4, 0 };
int fis_gRO19[] = { 2, 0 };
int fis_gRO20[] = { 2, 0 };
int fis_gRO21[] = { 4, 0 };
int fis_gRO22[] = { 2, 0 };
int fis_gRO23[] = { 5, 0 };
int fis_gRO24[] = { 2, 0 };
int fis_gRO25[] = { 5, 0 };
int fis_gRO26[] = { 3, 0 };
int fis_gRO27[] = { 5, 0 };
int fis_gRO28[] = { 3, 0 };
int fis_gRO29[] = { 3, 0 };
int fis_gRO30[] = { 1, 0 };
int fis_gRO31[] = { 4, 0 };
int fis_gRO32[] = { 2, 0 };
int fis_gRO33[] = { 4, 0 };
int fis_gRO34[] = { 2, 0 };
int fis_gRO35[] = { 5, 0 };
int fis_gRO36[] = { 4, 0 };
int fis_gRO37[] = { 2, 0 };
int fis_gRO38[] = { 5, 0 };
int fis_gRO39[] = { 2, 0 };
int fis_gRO40[] = { 5, 0 };
int fis_gRO41[] = { 3, 0 };
int fis_gRO42[] = { 5, 0 };
int fis_gRO43[] = { 3, 0 };
int fis_gRO44[] = { 3, 0 };
int fis_gRO45[] = { 1, 0 };
int fis_gRO46[] = { 4, 0 };
int fis_gRO47[] = { 2, 0 };
int fis_gRO48[] = { 4, 0 };
int fis_gRO49[] = { 2, 0 };
int fis_gRO50[] = { 5, 0 };
int fis_gRO51[] = { 4, 0 };
int fis_gRO52[] = { 2, 0 };
int fis_gRO53[] = { 5, 0 };
int fis_gRO54[] = { 2, 0 };
int fis_gRO55[] = { 5, 0 };
int fis_gRO56[] = { 3, 0 };
int fis_gRO57[] = { 5, 0 };
int fis_gRO58[] = { 3, 0 };
int fis_gRO59[] = { 3, 0 };
int fis_gRO60[] = { 1, 0 };
int fis_gRO61[] = { 4, 0 };
int fis_gRO62[] = { 2, 0 };
int fis_gRO63[] = { 4, 0 };
int fis_gRO64[] = { 2, 0 };
int fis_gRO65[] = { 5, 0 };
int fis_gRO66[] = { 4, 0 };
int fis_gRO67[] = { 2, 0 };
int fis_gRO68[] = { 5, 0 };
int fis_gRO69[] = { 2, 0 };
int fis_gRO70[] = { 5, 0 };
int fis_gRO71[] = { 3, 0 };
int fis_gRO72[] = { 5, 0 };
int fis_gRO73[] = { 3, 0 };
int fis_gRO74[] = { 3, 0 };
int fis_gRO75[] = { 0, 5 };
int fis_gRO76[] = { 0, 5 };
int fis_gRO77[] = { 0, 4 };
int fis_gRO78[] = { 0, 3 };
int fis_gRO79[] = { 0, 4 };
int fis_gRO80[] = { 0, 3 };
int fis_gRO81[] = { 0, 2 };
int fis_gRO82[] = { 0, 1 };
int fis_gRO83[] = { 0, 3 };
int fis_gRO84[] = { 0, 2 };
int fis_gRO85[] = { 0, 1 };
int fis_gRO86[] = { 0, 1 };
int fis_gRO87[] = { 0, 4 };
int fis_gRO88[] = { 0, 3 };
int fis_gRO89[] = { 0, 2 };
int fis_gRO90[] = { 0, 1 };
int fis_gRO91[] = { 0, 5 };
int fis_gRO92[] = { 0, 5 };
int fis_gRO93[] = { 0, 4 };
int fis_gRO94[] = { 0, 3 };
int* fis_gRO[] = { fis_gRO0, fis_gRO1, fis_gRO2, fis_gRO3, fis_gRO4, fis_gRO5, fis_gRO6, fis_gRO7, fis_gRO8, fis_gRO9, fis_gRO10, fis_gRO11, fis_gRO12, fis_gRO13, fis_gRO14, fis_gRO15, fis_gRO16, fis_gRO17, fis_gRO18, fis_gRO19, fis_gRO20, fis_gRO21, fis_gRO22, fis_gRO23, fis_gRO24, fis_gRO25, fis_gRO26, fis_gRO27, fis_gRO28, fis_gRO29, fis_gRO30, fis_gRO31, fis_gRO32, fis_gRO33, fis_gRO34, fis_gRO35, fis_gRO36, fis_gRO37, fis_gRO38, fis_gRO39, fis_gRO40, fis_gRO41, fis_gRO42, fis_gRO43, fis_gRO44, fis_gRO45, fis_gRO46, fis_gRO47, fis_gRO48, fis_gRO49, fis_gRO50, fis_gRO51, fis_gRO52, fis_gRO53, fis_gRO54, fis_gRO55, fis_gRO56, fis_gRO57, fis_gRO58, fis_gRO59, fis_gRO60, fis_gRO61, fis_gRO62, fis_gRO63, fis_gRO64, fis_gRO65, fis_gRO66, fis_gRO67, fis_gRO68, fis_gRO69, fis_gRO70, fis_gRO71, fis_gRO72, fis_gRO73, fis_gRO74, fis_gRO75, fis_gRO76, fis_gRO77, fis_gRO78, fis_gRO79, fis_gRO80, fis_gRO81, fis_gRO82, fis_gRO83, fis_gRO84, fis_gRO85, fis_gRO86, fis_gRO87, fis_gRO88, fis_gRO89, fis_gRO90, fis_gRO91, fis_gRO92, fis_gRO93, fis_gRO94 };

// Input range Min
FIS_TYPE fis_gIMin[] = { 0, 0, 0, 0, 0 };

// Input range Max
FIS_TYPE fis_gIMax[] = { 40, 100, 24, 1000, 8 };

// Output range Min
FIS_TYPE fis_gOMin[] = { 0, 0 };

// Output range Max
FIS_TYPE fis_gOMax[] = { 1500, 1500 };

//***********************************************************************
// Data dependent support functions for Fuzzy Inference System           
//***********************************************************************
FIS_TYPE fis_MF_out(FIS_TYPE** fuzzyRuleSet, FIS_TYPE x, int o)
{
    FIS_TYPE mfOut;
    int r;

    for (r = 0; r < fis_gcR; ++r)
    {
        int index = fis_gRO[r][o];
        if (index > 0)
        {
            index = index - 1;
            mfOut = (fis_gMF[fis_gMFO[o][index]])(x, fis_gMFOCoeff[o][index]);
        }
        else if (index < 0)
        {
            index = -index - 1;
            mfOut = 1 - (fis_gMF[fis_gMFO[o][index]])(x, fis_gMFOCoeff[o][index]);
        }
        else
        {
            mfOut = 0;
        }

        fuzzyRuleSet[0][r] = fis_min(mfOut, fuzzyRuleSet[1][r]);
    }
    return fis_array_operation(fuzzyRuleSet[0], fis_gcR, fis_max);
}

FIS_TYPE fis_defuzz_centroid(FIS_TYPE** fuzzyRuleSet, int o)
{
    FIS_TYPE step = (fis_gOMax[o] - fis_gOMin[o]) / (FIS_RESOLUSION - 1);
    FIS_TYPE area = 0;
    FIS_TYPE momentum = 0;
    FIS_TYPE dist, slice;
    int i;

    // calculate the area under the curve formed by the MF outputs
    for (i = 0; i < FIS_RESOLUSION; ++i){
        dist = fis_gOMin[o] + (step * i);
        slice = step * fis_MF_out(fuzzyRuleSet, dist, o);
        area += slice;
        momentum += slice*dist;
    }

    return ((area == 0) ? ((fis_gOMax[o] + fis_gOMin[o]) / 2) : (momentum / area));
}

//***********************************************************************
// Fuzzy Inference System                                                
//***********************************************************************
void fis_evaluate()
{
    FIS_TYPE fuzzyInput0[] = { 0, 0, 0, 0, 0 };
    FIS_TYPE fuzzyInput1[] = { 0, 0, 0, 0, 0 };
    FIS_TYPE fuzzyInput2[] = { 0, 0, 0, 0, 0 };
    FIS_TYPE fuzzyInput3[] = { 0, 0, 0, 0 };
    FIS_TYPE fuzzyInput4[] = { 0, 0, 0 };
    FIS_TYPE* fuzzyInput[fis_gcI] = { fuzzyInput0, fuzzyInput1, fuzzyInput2, fuzzyInput3, fuzzyInput4, };
    FIS_TYPE fuzzyOutput0[] = { 0, 0, 0, 0, 0 };
    FIS_TYPE fuzzyOutput1[] = { 0, 0, 0, 0, 0 };
    FIS_TYPE* fuzzyOutput[fis_gcO] = { fuzzyOutput0, fuzzyOutput1, };
    FIS_TYPE fuzzyRules[fis_gcR] = { 0 };
    FIS_TYPE fuzzyFires[fis_gcR] = { 0 };
    FIS_TYPE* fuzzyRuleSet[] = { fuzzyRules, fuzzyFires };
    FIS_TYPE sW = 0;

    // Transforming input to fuzzy Input
    int i, j, r, o;
    for (i = 0; i < fis_gcI; ++i)
    {
        for (j = 0; j < fis_gIMFCount[i]; ++j)
        {
            fuzzyInput[i][j] =
                (fis_gMF[fis_gMFI[i][j]])(g_fisInput[i], fis_gMFICoeff[i][j]);
        }
    }

    int index = 0;
    for (r = 0; r < fis_gcR; ++r)
    {
        if (fis_gRType[r] == 1)
        {
            fuzzyFires[r] = FIS_MAX;
            for (i = 0; i < fis_gcI; ++i)
            {
                index = fis_gRI[r][i];
                if (index > 0)
                    fuzzyFires[r] = fis_min(fuzzyFires[r], fuzzyInput[i][index - 1]);
                else if (index < 0)
                    fuzzyFires[r] = fis_min(fuzzyFires[r], 1 - fuzzyInput[i][-index - 1]);
                else
                    fuzzyFires[r] = fis_min(fuzzyFires[r], 1);
            }
        }
        else
        {
            fuzzyFires[r] = FIS_MIN;
            for (i = 0; i < fis_gcI; ++i)
            {
                index = fis_gRI[r][i];
                if (index > 0)
                    fuzzyFires[r] = fis_max(fuzzyFires[r], fuzzyInput[i][index - 1]);
                else if (index < 0)
                    fuzzyFires[r] = fis_max(fuzzyFires[r], 1 - fuzzyInput[i][-index - 1]);
                else
                    fuzzyFires[r] = fis_max(fuzzyFires[r], 0);
            }
        }

        fuzzyFires[r] = fis_gRWeight[r] * fuzzyFires[r];
        sW += fuzzyFires[r];
    }

    if (sW == 0)
    {
        for (o = 0; o < fis_gcO; ++o)
        {
            g_fisOutput[o] = ((fis_gOMax[o] + fis_gOMin[o]) / 2);
        }
    }
    else
    {
        for (o = 0; o < fis_gcO; ++o)
        {
            g_fisOutput[o] = fis_defuzz_centroid(fuzzyRuleSet, o);
        }
    }
}
