//#define MODO_CAPACITIVO
#define MODO_RESISTIVO

#if defined(MODO_CAPACITIVO) && defined(MODO_RESISTIVO)
#error "Elija solo uno de los modos de operacion"
#elif !defined(MODO_CAPACITIVO) && !defined(MODO_RESISTIVO)
#error "Elija uno de los modos de operacion"
#endif

#ifdef MODO_CAPACITIVO
#include <CapacitiveSensor.h>
#endif

const byte totalTonos = 16;

const word freqNormTonoPF[totalTonos] = {
  261.63 / (F_CPU / 512.0 / 256) * 256,  //C4
  293.66 / (F_CPU / 512.0 / 256) * 256,  //D4
  329.63 / (F_CPU / 512.0 / 256) * 256,  //E4
  349.23 / (F_CPU / 512.0 / 256) * 256,  //F4
  392.00 / (F_CPU / 512.0 / 256) * 256,  //G4
  440.00 / (F_CPU / 512.0 / 256) * 256,  //A4
  493.88 / (F_CPU / 512.0 / 256) * 256,  //B4
  523.25 / (F_CPU / 512.0 / 256) * 256,  //C5

  523.25 / (F_CPU / 512.0 / 256) * 256,  //C5
  587.33 / (F_CPU / 512.0 / 256) * 256,  //D5
  659.25 / (F_CPU / 512.0 / 256) * 256,  //E5
  698.46 / (F_CPU / 512.0 / 256) * 256,  //F5
  783.99 / (F_CPU / 512.0 / 256) * 256,  //G5
  880.00 / (F_CPU / 512.0 / 256) * 256,  //A5
  987.77 / (F_CPU / 512.0 / 256) * 256,  //B5
  1046.50 / (F_CPU / 512.0 / 256) * 256, //C6
};

signed char tablaSintesis[256];
byte tablaEnvAtq[256];
byte tablaEnvDec[256];
byte tablaEnvLib[256];

//perPasoEnv = tiempo / (Ts * tamTabla) o bien
//perPasoEnv = tiempo / (ICR1 / F_CPU * tamTablaEnv)
word perPasoEnvAtq = 0.01 / (512.0 / F_CPU * 256);
word perPasoEnvDec = 0.25 / (512.0 / F_CPU * 256);
word perPasoEnvLib = 0.25 / (512.0 / F_CPU * 256);

enum ESTADO_ENVOLVENTE {
  EE_APAGADA = 0, EE_ATAQUE, EE_DECAIDA, EE_SOSTENIMIENTO,
  EE_LIBERACION,
};

struct ESTADO_VOZ {
  ESTADO_ENVOLVENTE estadoEnv;
  byte posTablaEnv;
  word fraccPosTablaEnv;
  word posTablaSintPF;
  byte tono;
  bool liberarVoz;
};

const byte totalVoces = 3;
ESTADO_VOZ voz[totalVoces];

struct ESTADO_TECLA {
  bool estAnt;
  bool estAct;
};

ESTADO_TECLA tecla[totalTonos];

const int pinTecla[totalTonos] = {
  2, 3, 4, 5, 6, 7, 8, 10, 12, 11, A0, A1, A2, A3, A4, A5,
};

#ifdef MODO_CAPACITIVO
const int pinSensorTx = 13;

CapacitiveSensor sensorCap[totalTonos] = {
  CapacitiveSensor(pinSensorTx, pinTecla[0]),
  CapacitiveSensor(pinSensorTx, pinTecla[1]),
  CapacitiveSensor(pinSensorTx, pinTecla[2]),
  CapacitiveSensor(pinSensorTx, pinTecla[3]),
  CapacitiveSensor(pinSensorTx, pinTecla[4]),
  CapacitiveSensor(pinSensorTx, pinTecla[5]),
  CapacitiveSensor(pinSensorTx, pinTecla[6]),
  CapacitiveSensor(pinSensorTx, pinTecla[7]),
  CapacitiveSensor(pinSensorTx, pinTecla[8]),
  CapacitiveSensor(pinSensorTx, pinTecla[9]),
  CapacitiveSensor(pinSensorTx, pinTecla[10]),
  CapacitiveSensor(pinSensorTx, pinTecla[11]),
  CapacitiveSensor(pinSensorTx, pinTecla[12]),
  CapacitiveSensor(pinSensorTx, pinTecla[13]),
  CapacitiveSensor(pinSensorTx, pinTecla[14]),
  CapacitiveSensor(pinSensorTx, pinTecla[15]),
};
#endif

void setup() {
  word i;

  Serial.begin(115200);

#ifdef MODO_RESISTIVO
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  for (i = 0; i < totalTonos; i++)
    pinMode(pinTecla[i], INPUT_PULLUP);
#endif

  for (i = 0; i < 256; i++) {
    tablaSintesis[i] =
      (1.0 * sin(2 * PI * float(i) * 1.0 / 256)
       + 0.0 * sin(2 * PI * float(i) * 3.0 / 256)
       + 0.0 * sin(2 * PI * float(i) * 5.0 / 256)) * 127;
      //(0.6 * sin(2 * PI * float(i) * 1.0 / 256)
      // + 0.3 * sin(2 * PI * float(i) * 3.0 / 256)
      // + 0.1 * sin(2 * PI * float(i) * 5.0 / 256)) * 127;

    tablaEnvAtq[i] = i;
    tablaEnvDec[i] = 255 - i / 2;
    tablaEnvLib[i] = 127 - i / 2;
    Serial.println(tablaEnvLib[i]);
  }

  analogWrite(9, 128);

  //Activa el modulo en modalidad PWM rapido, utilizando ICR1L como
  //registro de cuenta tope (periodo)
  TCCR1A = 0x82;
  TCCR1B = 0x19;

  ICR1H = highByte(512);
  ICR1L = lowByte(512);
  OCR1AH = highByte(256);
  OCR1AL = lowByte(256);

  TIMSK1 |= _BV(TOIE1);
}

void loop() {
  byte i;

  for (i = 0; i < totalTonos; i++) {
    tecla[i].estAnt = tecla[i].estAct;

#ifdef MODO_RESISTIVO
    tecla[i].estAct = !digitalRead(pinTecla[i]);
#endif

#ifdef MODO_CAPACITIVO
    int capacitancia = sensorCap[i].capacitiveSensor(30);
    Serial.print(capacitancia);
    if (capacitancia > 30)
      tecla[i].estAct = true;
    else
      tecla[i].estAct = false;
#endif

    if (!tecla[i].estAnt && tecla[i].estAct) {
      tocarNota(i);
      Serial.println("H");
    }
    if (tecla[i].estAnt && !tecla[i].estAct) {
      pararNota(i);
      Serial.println("L");
    }
  }

  Serial.println();

  /*
    for (i = 0; i < totalVoces; i++) {
      Serial.print(voz[i].estadoEnv);
    }
    Serial.println();
  */

  delay(10);
}

void tocarNota(byte tono) {
  byte i;

  noInterrupts();

  //Primero se busca si el tono ya se esta reproduciendo en
  //una voz
  for (i = 0; i < totalVoces; i++) {
    //Si se encuentra un tono activo en una voz, se termina
    //el lazo de busqueda con la variable i conteniendo el
    //indice de la voz encontrada
    if (voz[i].estadoEnv != EE_APAGADA && voz[i].tono == tono)
      break;
  }

  //Si la voz no se esta reproduciendo previamente, se busca
  //cualquier voz que este libre
  if (i >= totalVoces) {
    for (i = 0; i < totalVoces; i++) {
      //Si se encuentra una voz libre, se termina el lazo y
      //la variable i indica la voz encontrada
      if (voz[i].estadoEnv == EE_APAGADA)
        break;
    }
  }

  //Si cualquiera de las 2 busquedas anteriores produjo un
  //resultado, se activa la voz encontrada
  if (i < totalVoces) {
    voz[i].estadoEnv = EE_ATAQUE;
    voz[i].posTablaEnv = 0;
    voz[i].fraccPosTablaEnv = 0;
    //voz[i].posTablaSintPF = 0;
    voz[i].tono = tono;
    voz[i].liberarVoz = false;
  }

  interrupts();
}

void pararNota(byte tono) {
  byte i;

  noInterrupts();

  for (i = 0; i < totalVoces; i++) {
    if (voz[i].estadoEnv != EE_APAGADA && voz[i].tono == tono)
      voz[i].liberarVoz = true;
  }

  interrupts();
}

ISR(TIMER1_OVF_vect) {
  byte i;
  int volumen;
  char muestraVoz;
  int muestraFinal = 0;

  for (i = 0; i < totalVoces; i++) {
    switch (voz[i].estadoEnv) {
      case EE_APAGADA:
        volumen = 0;
        break;
      case EE_ATAQUE:
        volumen = tablaEnvAtq[voz[i].posTablaEnv];
        voz[i].fraccPosTablaEnv++;
        if (voz[i].fraccPosTablaEnv >= perPasoEnvAtq) {
          voz[i].fraccPosTablaEnv = 0;
          voz[i].posTablaEnv++;
          if (voz[i].posTablaEnv == 0)
            voz[i].estadoEnv = EE_DECAIDA;
        }
        break;
      case EE_DECAIDA:
        volumen = tablaEnvDec[voz[i].posTablaEnv];
        voz[i].fraccPosTablaEnv++;
        if (voz[i].fraccPosTablaEnv >= perPasoEnvDec) {
          voz[i].fraccPosTablaEnv = 0;
          voz[i].posTablaEnv++;
          if (voz[i].posTablaEnv == 0)
            voz[i].estadoEnv = EE_SOSTENIMIENTO;
        }
        break;
      case EE_SOSTENIMIENTO:
        volumen = 128;
        if (voz[i].liberarVoz)
          voz[i].estadoEnv = EE_LIBERACION;
        break;
      case EE_LIBERACION:
        volumen = tablaEnvLib[voz[i].posTablaEnv];
        voz[i].fraccPosTablaEnv++;
        if (voz[i].fraccPosTablaEnv >= perPasoEnvLib) {
          voz[i].fraccPosTablaEnv = 0;
          voz[i].posTablaEnv++;
          if (voz[i].posTablaEnv == 0)
            voz[i].estadoEnv = EE_APAGADA;
        }
        break;
    }

    muestraVoz =
      highByte(volumen *
               tablaSintesis[highByte(voz[i].posTablaSintPF)]);
    muestraFinal += muestraVoz;
    voz[i].posTablaSintPF += freqNormTonoPF[voz[i].tono];
  }

  muestraFinal += 256;
  OCR1AH = highByte(muestraFinal);
  OCR1AL = lowByte(muestraFinal);
}
