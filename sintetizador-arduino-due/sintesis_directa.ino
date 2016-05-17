//Estados del automata finito asociado a cada voz
enum ESTADO_ENVOLVENTE {
  EE_APAGADA = 0, EE_ATAQUE, EE_DECAIDA, EE_SOSTENIMIENTO,
  EE_LIBERACION,
};

//Estructura con el estado general de cada voz
struct ESTADO_VOZ {
  ESTADO_ENVOLVENTE estadoEnv;
  uint32_t posTablaSintPF;
  uint32_t posTablaEnvPF;
  uint32_t frecNormPF;
  byte tono;
  bool liberarVoz;
};

//Arreglo con el total de las voces
const byte totalVoces = 10;
volatile ESTADO_VOZ voz[totalVoces];

//Cantidad de muestras para las tablas de sintesis y envolvente
const uint16_t nMuestrasSint = 1024;
const uint16_t nMuestrasEnv = 1024;

//Mascara usada para recortar el conteo de direcciones de la tabla
//de sintesis
const uint32_t mascFreqNormPF = ((nMuestrasSint - 1) << 16)
                                | 0x0000FFFF;

//Posicion final de la tabla de envolvente, en formato entero y de
//punto fijo
const uint16_t nMuestFinEnv = nMuestrasEnv - 1;
const uint32_t nMuestFinEnvPF = (nMuestFinEnv << 16)
                                | 0x0000FFFF;

//Posiciones en formato de punto fijo para las posiciones en la
//tabla de atenuacion que se corresponden con los niveles de
//ataque y sostenimiento
const uint32_t posAtenAtqPF = 0 << 16;
const uint32_t posAtenSosPF = ((nMuestrasEnv / 4) << 16)
                              | 0x0000FFFF;;

//Tiempos de duracion de cada una de las 4 fases de las notas
//(Estos tiempos son maximos, asumiendo que cada fase dura a lo
//largo de todo el recorrido de la tabla de envolvente)
const float tiempoAtq = 0.01;
const float tiempoDec = 0.1;
const float tiempoSos = 6.0;
const float tiempoLib = 0.35;

//Calculo de los ritmos para cada una de las fases
const uint32_t ritmoAtqPF =
  round(float(nMuestFinEnvPF - posAtenAtqPF)
        / frecMuestreo / tiempoAtq);
const uint32_t ritmoDecPF =
  round(float(nMuestFinEnvPF - posAtenAtqPF)
        / frecMuestreo / tiempoDec);
const uint32_t ritmoSosPF =
  round(float(nMuestFinEnvPF - posAtenAtqPF)
        / frecMuestreo / tiempoSos);
const uint32_t ritmoLibPF =
  round(float(nMuestFinEnvPF - posAtenAtqPF)
        / frecMuestreo / tiempoLib);

//Tablas de sintesis, envolvente y frequencia normalizada
int16_t tablaSintesis[nMuestrasSint];
uint16_t tablaEnvLog[nMuestrasEnv];
uint32_t frecNormTonoPF[128];

void inicializarTablas() {
  uint16_t i;

  //La frecuencia base depende de la frecuencia de muestreo y la
  //longitud de la tabla de sintesis. Las diferentes frecuencias de
  //las notas se consiguen por medio del re-muestreo, alterando
  //esta frecuencia fundamental
  const float frecBase = float(frecMuestreo) / float(nMuestrasSint);

  //Se calculan las frecuencias normalizadas de todas los tonos
  //que soporta el protocolo midi
  for (i = 0; i < 128; i++) {
    frecNormTonoPF[i] =
      pow(2.0, (i - 69.0) / 12.0) * 440.0 / frecBase * 65536.0;
  }

  //Se procede a computar la tabla de sintesis
  for (i = 0; i < nMuestrasSint; i++) {
    //Nota: se sugiere (des)comentar los diferentes bloques para
    //probar los diferentes sonidos

    //Sintesis aditiva de 8 armonicas para imitar un piano
    tablaSintesis[i] =
      (3.7899e+04 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                     1.0 / float(nMuestrasSint))
       + 3.1526e+03 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                       2.0 / float(nMuestrasSint))
       + 2.8812e+03 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                       3.0 / float(nMuestrasSint))
       + 5.3975e+03 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                       4.0 / float(nMuestrasSint))
       + 3.3610e+03 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                       5.0 / float(nMuestrasSint))
       + 7.5544e+03 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                       6.0 / float(nMuestrasSint))
       + 8.9620e+02 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                       7.0 / float(nMuestrasSint))
       + 2.2706e+02 / 3.7899e+04 * sin(2.0 * PI * float(i) *
                                       8.0 / float(nMuestrasSint))
      ) * 511;

    /*
      //Seno, un armonico
      tablaSintesis[i] =
      (0.5 * sin(2.0 * PI * float(i) *
                 1.0 / float(nMuestrasSint))
       + 0.0 * sin(2.0 * PI * float(i) *
                   2.0 / float(nMuestrasSint))
       + 0.0 * sin(2.0 * PI * float(i) *
                   3.0 / float(nMuestrasSint))
      ) * 1023;
    */
    /*
      //Seno, tres armonicos
      for (i = 0; i < nMuestrasSint; i++) {
        tablaSintesis[i] =
        (0.3 * sin(2.0 * PI * float(i) *
                   1.0 / float(nMuestrasSint))
         + 0.1 * sin(2.0 * PI * float(i) *
                     2.0 / float(nMuestrasSint))
         + 0.1 * sin(2.0 * PI * float(i) *
                     3.0 / float(nMuestrasSint))
        ) * 1023;
    */
    /*
      //Onda rectangular
      if (i < nMuestrasSint / 2)
      tablaSintesis[i] = 255;
      else
      tablaSintesis[i] = -255;
    */
    /*
      //Onda triangular
      if (i < nMuestrasSint / 4)
      tablaSintesis[i] = 255.0 * float(i)
                         / float(nMuestrasSint / 4);
      else if (i < nMuestrasSint * 3 / 4)
      tablaSintesis[i] = 255.0 * float(nMuestrasSint / 2 - i)
                         / float(nMuestrasSint / 4);
      else
      tablaSintesis[i] = 255.0 * float(i - nMuestrasSint)
                         / float(nMuestrasSint / 4);
    */
    //Depuracion solamente, descomentar para volcar la tabla
    //Serial.print(i);
    //Serial.print("  -  ");
    //Serial.println(tablaSintesis[i]);
  }

  //Se calculan los diferentes parametros de la tabla de envolvente
  //pseudo exponencial
  const float xi = float(nMuestrasEnv) * 3.0 / 4.0;
  const float b = exp(-1.0 / (float(nMuestFinEnv) - xi));
  const float yi = 65535.0 * pow(b, xi);

  for (i = 0; i < nMuestrasEnv; i++) {
    //Envolvente lineal descendente
    //tablaEnvLog[i] = (65535 - i * 65535 / (nMuestrasEnv - 1));

    //Envolvente pseudo-exponencial (exponencial unida con lineal)
    if (i < round(xi))
      tablaEnvLog[i] = round(65535.0 * pow(b, float(i)));
    else
      tablaEnvLog[i] =
        round(-yi / (float(nMuestFinEnv) - xi) *
              (float(i) - xi) + yi);

    //Depuracion solamente, descomentar para volcar la tabla
    //Serial.print(i);
    //Serial.print("  -  ");
    //Serial.println(tablaEnvLog[i]);
  }
}

void tocarNota(byte tono) {
  byte i;

  //Se deshabilitan las interrupciones para evitar que la ISR
  //altere las variables de estado de las voces
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
    //voz[i].posTablaSintPF = 0;
    voz[i].posTablaEnvPF = nMuestFinEnvPF;
    voz[i].frecNormPF = frecNormTonoPF[tono];
    voz[i].tono = tono;
    voz[i].liberarVoz = false;
  }

  //Al terminar de registrar la voz se rehabilitan las
  //interrupciones
  interrupts();
}

void pararNota(byte tono) {
  byte i;

  noInterrupts();

  //Se busca una voz aun activa que coincida con el tono
  //deseado. Si se encuentra, se marca para liberacion.
  for (i = 0; i < totalVoces; i++) {
    if (voz[i].estadoEnv != EE_APAGADA && voz[i].tono == tono)
      voz[i].liberarVoz = true;
  }

  interrupts();
}

void TC3_Handler() {
  int32_t amplitud;
  int16_t muestraVoz;
  int16_t muestraFinal = 0; //La mezcla esta inicialmente vacia
  byte i;
  uint32_t posTablaEnvPF;

  //Lee el registro de estado para limpiar la bandera de
  //interrupcion
  TC_GetStatus(TC1, 0);

  //Se recorre el estado de cada una de las voces para actualizar
  for (i = 0; i < totalVoces; i++) {
    //Se lee tanto la posicion de la tabla de envolvente asi como
    //el valor de la misma en esa posicion
    posTablaEnvPF = voz[i].posTablaEnvPF;
    amplitud = tablaEnvLog[posTablaEnvPF >> 16];

    //A continuacion se determina la siguiente posicion de la tabla
    //envolvente asi como el estado de la voz en base al estado
    //actual
    switch (voz[i].estadoEnv) {
      case EE_APAGADA:
        //En caso que la voz este apagada, se corrige la amplitud
        //para forzarla a 0
        amplitud = 0;
        break;
      case EE_ATAQUE:
        //En caso de estar en ataque, se revisa si se alcanzo la
        //posicion final de este estado
        if (posTablaEnvPF - posAtenAtqPF >= ritmoAtqPF)
          //Si no se alcanza, se prosigue avanzando la posicion
          //en sentido inverso
          posTablaEnvPF -= ritmoAtqPF;
        else
          //Si se alcanza, se avanza al siguiente estado
          voz[i].estadoEnv = EE_DECAIDA;
        break;
      case EE_DECAIDA:
        //Se procede de manera similar al estado anterior,
        //verificando si se alcanza la posicion final y avanzando
        //la posicion o cambiando de estado acordemente
        if (posAtenSosPF - posTablaEnvPF >= ritmoDecPF)
          posTablaEnvPF += ritmoDecPF;
        else
          voz[i].estadoEnv = EE_SOSTENIMIENTO;
        break;
      case EE_SOSTENIMIENTO:
        //Aca tambien se procede de forma similar
        if (nMuestFinEnvPF - posTablaEnvPF >= ritmoSosPF)
          posTablaEnvPF += ritmoSosPF ;
        else
          //Esta vez, si la voz se sostiene hasta que se llega
          //al final de la tabla de envolvente, mejor se apaga
          //de inmediato
          voz[i].estadoEnv = EE_APAGADA;

        //En adicion a los chequeos anteriores, si esta marcada la
        //bandera de liberacion, se pasa a ese estado de forma
        //inmediata
        if (voz[i].liberarVoz)
          voz[i].estadoEnv = EE_LIBERACION;
        break;
      case EE_LIBERACION:
        //Se procede de forma similar en el ultimo estado
        if (nMuestFinEnvPF - posTablaEnvPF >= ritmoLibPF)
          posTablaEnvPF += ritmoLibPF;
        else
          voz[i].estadoEnv = EE_APAGADA;
        break;
    }

    //Se guarda la nueva posicion de la tabla de envolvente
    voz[i].posTablaEnvPF = posTablaEnvPF;

    //Se retira el dato siguiente de la tabla de sintesis y se le
    //aplica el valor de envolvente, el proceso de multiplicar
    //genera un resultado en punto fijo que se convierte nuevamente
    //a entero
    muestraVoz =
      (amplitud * tablaSintesis[voz[i].posTablaSintPF >> 16])
      >> 16;

    //Se agrega este valor a la mezcla
    muestraFinal += muestraVoz;

    //Se avanza la posicion de la tabla de sintesis segun la
    //frecuencia normalizada de la voz, luego se recorta
    voz[i].posTablaSintPF += voz[i].frecNormPF;
    voz[i].posTablaSintPF &= mascFreqNormPF;
  }

  //Una vez se mezclan todas las voces se envia la muestra al DAC
  //al mismo tiempo que se le remueve el signo
  analogWrite(DAC0, muestraFinal + 2048);
  analogWrite(DAC1, muestraFinal + 2048);
}
