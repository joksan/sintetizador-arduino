//Configuracion de frecuencia de muestreo
const uint16_t frecMuestreo = 44100;

void setup() {
  //Inicializa el puerto serie
  Serial.begin(115200);

  //Inicializa las tablas de sintesis
  inicializarTablas();

  //Se establece la resolucion del DAC a 12 bits
  analogWriteResolution(12);

  //Se inicializa el timer y la ISR correspondiente
  inicializarTimer();
}

void loop() {
  //El lazo principal solo procesa los comandos midi
  procesarTramaMidi();
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
          //if ((dato & 0x0F) == 0)
          estadoMidi = 1;

        //Se verifica si el comando es 0x90, que corresponde
        //a iniciar una nota
        if ((dato & 0xF0) == 0x90)
          //Solo se obedece el canal 1
          //if ((dato & 0x0F) == 0)
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

void inicializarTimer() {
  //Config. del controlador de administracion de energia (PMC)
  //----------------------------------------------------------
  //Desactiva la proteccion de escritura y activa el clock del
  //timer (se usan los mismos ID para la administracion de clock
  //que para los nombres de las ISR).
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(TC3_IRQn);
  //NOTA: El ATSAM3X8E tiene 3 modulos de timer, nombrados TC0,
  //TC1 y TC2, y cada uno posee 3 canales (channel 0-2). El
  //mapeo de interrupciones de timer es tal que las primeras 3
  //interrupciones son para los tres canales del TC0, luego las
  //siguientes 3 para el TC1 y las ultimas 3 para TC2. En total
  //son 9 canales de interrupcion que se numeran desde TC0_IRQn
  //hasta TC8_IRQn. En este caso, al primer canal del TC1, le
  //toca la interrupcion TC3_IRQn.

  //Configuracion del controlador de timer 1 (TC1)
  //----------------------------------------------
  const int canal = 0;
  //Se realiza la configuracion del canal 0
  TC_Configure(TC1, canal, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC |
               TC_CMR_TCCLKS_TIMER_CLOCK1);
  //Se realiza el calculo del contador de periodo del timer,
  //notese que se divide MCK/2 porque arriba se eligio
  //TIMER_CLOCK1
  uint32_t rc = VARIANT_MCK / 2 / frecMuestreo;
  //Se establecen los registros de conteo del timer
  TC_SetRA(TC1, canal, rc / 2); //50% en alto, 50% en bajo
  TC_SetRC(TC1, canal, rc);
  //Se arranca el timer
  TC_Start(TC1, canal);
  //Habilita la interruppcion de comparacion solamente,
  //deshabilitando el resto
  TC1->TC_CHANNEL[canal].TC_IER = TC_IER_CPCS;
  TC1->TC_CHANNEL[canal].TC_IDR = ~TC_IER_CPCS;

  //Configuracion del controlador de interrupciones (NVIC)
  //------------------------------------------------------
  //Habilita de la interrupcion de timer
  NVIC_EnableIRQ(TC3_IRQn);
}
