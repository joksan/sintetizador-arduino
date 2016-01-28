//#define MODO_CAPACITIVO
//#define MODO_RESISTIVO
#define MODO_CONTACTO

//#define SERIE_DEPURACION
#define SERIE_MIDI

#define SINTESIS_DIRECTA
//#define SINTESIS_LOG_SEN

//Chequeo de sanidad para los modos de operacion
#if defined(MODO_CAPACITIVO) + defined(MODO_RESISTIVO) \
  + defined(MODO_CONTACTO) > 1
#error "Elija solo uno de los modos de operacion"
#elif !defined(MODO_CAPACITIVO) && !defined(MODO_RESISTIVO) \
   && !defined(MODO_CONTACTO)
#error "Elija uno de los modos de operacion"
#endif

//Chequeo de sanidad para la operacion del puerto serie
#if defined(SERIE_DEPURACION) && defined(SERIE_MIDI)
#error "No se puede depurar y usar comunicacion MIDI a la vez"
#endif

//Chequeo de sanidad para el metodo de sintesis
#if defined(SINTESIS_DIRECTA) && defined(SINTESIS_LOG_SEN)
#error "Elija solo un metodo de sintesis"
#elif !defined(SINTESIS_DIRECTA) && !defined(SINTESIS_LOG_SEN)
#error "Elija un metodo de sintesis"
#endif

#ifdef SERIE_DEPURACION
//Si se habiltia la depuracion, se definen las macros para emitir mensajes
//por el puerto serie
#define MSG_DEP(m) Serial.print(m)
#define MSG_DEP_LN(m) Serial.println(m)
#else
//Si no se habilita la depuracion, se generan macros nulas que no hacen nada
#define MSG_DEP(m)
#define MSG_DEP_LN(m)
#endif

//Arreglo con el mapeo de I/O asociado a cada tecla
const byte totalTeclas = 16;
const int pinTecla[totalTeclas] = {
  2, 3, 4, 5, 6, 7, 8, 10, 12, 11, A0, A1, A2, A3, A4, A5,
};

//Arreglo con el mapeo de notas asociados a cada tecla
byte notaTecla[totalTeclas] = {
  60, //C4
  62, //D4
  64, //E4
  65, //F4
  67, //G4
  69, //A4
  71, //B4
  72, //C5

  72, //C5
  74, //D5
  76, //E5
  77, //F5
  79, //G5
  81, //A5
  83, //B5
  84, //C6
};

//Estructura con el estado de cada tecla
struct ESTADO_TECLA {
  bool estAnt;
  bool estAct;
};

//Arreglo de estructuras con el estado de todas las teclas
ESTADO_TECLA tecla[totalTeclas];

//En caso de habilitarse el modo capacitivo, se incluye la
//libreria de soporte
#ifdef MODO_CAPACITIVO
#include <CapacitiveSensor.h>

//Pin de transmision de los sensores capacitivos
const int pinSensorTx = 13;

//Arreglo de sensores capacitivos
CapacitiveSensor sensorCap[totalTeclas] = {
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
  byte i;

  //Inicializa el puerto serie a la mas alta velocidad posible
  //para minimizar latencia
  Serial.begin(115200);

#ifdef MODO_RESISTIVO
  //En modo resistivo se colocan todos los I/O como entradas
  for (i = 0; i < totalTeclas; i++)
    pinMode(pinTecla[i], INPUT);
#endif

#ifdef MODO_CONTACTO
  //En modo de contacto se ponen los I/O como entrada pero se
  //activa el pullup interno
  for (i = 0; i < totalTeclas; i++)
    pinMode(pinTecla[i], INPUT_PULLUP);
#endif

  //Se inicializan las tablas de sintesis
  inicializarTablas();

  //Se hace una escritura al PWM para causar que se inicialice
  //el timer
  analogWrite(9, 128);

  //Una vez echado a andar el timer se modifican los registros,
  //activando la modalidad PWM rapido y utilizando ICR1 como
  //registro de cuenta tope (periodo)
  TCCR1A = 0x82;
  TCCR1B = 0x19;
  //TCCR1A: COM1A = 10b  - Modo no invertido
  //        COM1B = 00b  - Operacion de puerto normal
  //TCCR1B: ICNC1 = 0b   - Cancelacion de ruido apagada
  //        ICES1 = 0b   - No importa
  //        CS1   = 001b - CLK_IO / 1 (sin prescala)
  //TCCR1 A+B: WGM1 = 1110b - PWM Rapido, ICR1 = TOP

  //A continuacion se inicializan los registros de cuenta tope
  //(ICR1) y ciclo de trabajo (OCR1A)
  ICR1H = highByte(512);  //Rango dinamico de -256 a +255 y
  ICR1L = lowByte(512);   //Fs = F_OSC / 512 = 31.25KHz
  OCR1AH = highByte(256); //Ciclo de trabajo inicial a 50%
  OCR1AL = lowByte(256);

  //Con el timer reconfigurado se habilitan las interrupciones
  TIMSK1 |= _BV(TOIE1);
}

void loop() {
  byte i;
  static unsigned long tAnt = 0;
  unsigned long tAct;

  //Se actualiza el tiempo
  tAct = millis();

  //Si el tiempo elapsado desde la ultima actualizacion excede
  //10ms, se procede a leer las teclas/sensores
  if (tAct - tAnt >= 10) {
    for (i = 0; i < totalTeclas; i++) {
      //Guarda el estado anterior de la tecla
      tecla[i].estAnt = tecla[i].estAct;

#if defined(MODO_RESISTIVO) || defined(MODO_CONTACTO)
      //En los modos resistivo y de contacto simplemente se
      //leen los I/O
      tecla[i].estAct = !digitalRead(pinTecla[i]);
#endif

#ifdef MODO_CAPACITIVO
      //En modo capacitivo se lee la capacitancia registrada
      int capacitancia = sensorCap[i].capacitiveSensor(30);

      //Se imprimen las capacitancias solo en depuracion
      MSG_DEP(capacitancia);
      if (i == totalTeclas - 1)
        MSG_DEP_LN();

      //Si la capacitancia excede el umbral, se da la tecla
      //por activada
      if (capacitancia > 30)
        tecla[i].estAct = true;
      else
        tecla[i].estAct = false;
#endif

      //Si la tecla se presiona, se toca la nota
      if (!tecla[i].estAnt && tecla[i].estAct) {
        tocarNota(notaTecla[i]);
        MSG_DEP_LN(F("H"));
      }

      //Si la tecla se libera, se para la nota
      if (tecla[i].estAnt && !tecla[i].estAct) {
        pararNota(notaTecla[i]);
        MSG_DEP_LN(F("L"));
      }
    }

    //Registra el tiempo de esta actializacion
    tAnt = tAct;
  }

#ifdef SERIE_MIDI
  //Se procesa la trama midi, la cual tambien puede causar
  //que se toquen o paren notas
  procesarTramaMidi();
#endif
}

void procesarTramaMidi() {
  static int estadoMidi = 0;
  byte dato;

  while (Serial.available()) {
    dato = Serial.read();
    //Serial.write(dato);   //Depuracion solamente
    switch (estadoMidi) {
      case 0:
        //Solo se toman en cuenta los comandos, no los datos
        //en este estado. Los comandos tienen el MSB en alto
        if (!(dato & 0x80)) break;

        //Se verifica si el comando es 0x80, que corresponde
        //a parar una nota
        if ((dato & 0xF0) == 0x80)
          //Este comando solo obedecera al canal 1. Los 4 LSB
          //codifican el canal que va de 1(0) a 16(15)
          if ((dato & 0x0F) == 0)
            estadoMidi = 1;

        //Se verifica si el comando es 0x90, que corresponde
        //a iniciar una nota
        if ((dato & 0xF0) == 0x90)
          //Solo se obedece el canal 1
          if ((dato & 0x0F) == 0)
            estadoMidi = 2;
        break;
      case 1:
        //Se corrobora que el dato recibido no sea un comando
        if (dato & 0x80) {
          //Si lo es, se reinicia el automata
          estadoMidi = 0;
          break;
        }
        //Se para el numero de nota recibida por midi
        pararNota(dato);
        estadoMidi = 3;
        break;
      case 2:
        //Se corrobora que el dato recibido no sea un comando
        if (dato & 0x80) {
          //Si lo es, se reinicia el automata
          estadoMidi = 0;
          break;
        }
        //Se inicia el numero de nota recibida por midi
        tocarNota(dato);
        estadoMidi = 3;
        break;
      case 3:
        //Los parametros adicionales (volumen de nota) son
        //ignorados
        estadoMidi = 0;
        break;
    }
  }
}
