#ifdef SINTESIS_DIRECTA

#include "tonos.h"

//Se calculan los periodos de paso para cada una de las etapas
//de la envolvente, empleando la siguiente formula:
//perPasoEnv = tiempo / (Ts * tamTabla)
//Lo cual equivale a:
//perPasoEnv = tiempo / (ICR1 / F_CPU * tamTablaEnv)
const word perPasoEnvAtq = 0.01 / (512.0 / F_CPU * 256);
const word perPasoEnvDec = 0.15 / (512.0 / F_CPU * 256);
const word perPasoEnvLib = 0.15 / (512.0 / F_CPU * 256);

//Estados del automata finito asociado a cada voz
enum ESTADO_ENVOLVENTE {
  EE_APAGADA = 0, EE_ATAQUE, EE_DECAIDA, EE_SOSTENIMIENTO,
  EE_LIBERACION,
};

//Estructura con el estado general de cada voz
struct ESTADO_VOZ {
  ESTADO_ENVOLVENTE estadoEnv;
  byte posTablaEnv;
  word fraccPosTablaEnv;
  word posTablaSintPF;
  word frecNorm;
  byte tono;
  bool liberarVoz;
};

const byte totalVoces = 3;
ESTADO_VOZ voz[totalVoces];

//Tablas de sintesis y envolvente
signed char tablaSintesis[256];
byte tablaEnvAtq[256];
byte tablaEnvDec[256];
byte tablaEnvLib[256];

void inicializarTablas() {
  word i;

  for (i = 0; i < 256; i++) {
    tablaSintesis[i] =
      //(0.167 * sin(2 * PI * float(i) * 1.0 / 256)
      // + 0.167 * sin(2 * PI * float(i) * 2.0 / 256)
      // + 0.167 * sin(2 * PI * float(i) * 3.0 / 256)) * 127;
      (0.5 * sin(2 * PI * float(i) * 1.0 / 256)
       + 0.0 * sin(2 * PI * float(i) * 2.0 / 256)
       + 0.0 * sin(2 * PI * float(i) * 3.0 / 256)) * 127;

    tablaEnvAtq[i] = i;
    tablaEnvDec[i] = 255 - i / 2;
    tablaEnvLib[i] = 127 - i / 2;
    MSG_DEP_LN(tablaEnvLib[i]);
  }
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
    voz[i].frecNorm = pgm_read_word(frecNormTonoPF + tono);
    voz[i].tono = tono;
    voz[i].liberarVoz = false;
  }

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

ISR(TIMER1_OVF_vect) {
  byte i;
  int volumen;
  char muestraVoz;
  int muestraFinal = 0;

  for (i = 0; i < totalVoces; i++) {
    //Se procede a actualizar cada una de las voces dependiendo
    //de su estado actual
    switch (voz[i].estadoEnv) {
      case EE_APAGADA:
        //Si la voz esta apagada su amplitud se hace 0
        volumen = 0;
        break;
      case EE_ATAQUE:
        //En modalidad de ataque, se busca el valor de amplitud
        //de la tabla de envolvente correspondiente
        volumen = tablaEnvAtq[voz[i].posTablaEnv];

        //A continuacion se hace avanzar la posicion de la
        //tabla de envolvente (en forma fraccionaria)
        voz[i].fraccPosTablaEnv++;
        if (voz[i].fraccPosTablaEnv >= perPasoEnvAtq) {
          //Si la fraccion alcanza el valor de periodo, se
          //avanza a la siguiente posicion de la tabla
          voz[i].fraccPosTablaEnv = 0;
          voz[i].posTablaEnv++;
          //Si la posicion de tabla se desborda, la tabla se
          //agoto y por tanto se pasa al siguiente estado
          if (voz[i].posTablaEnv == 0)
            voz[i].estadoEnv = EE_DECAIDA;
        }
        break;
      case EE_DECAIDA:
        //Se actua de igual forma que en el estado anterior
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
        //En la fase de sostenimiento el volumen es constante
        volumen = 128;
        //Si la voz se marca para liberacion, se pasa al estado
        //correspondiente
        if (voz[i].liberarVoz)
          voz[i].estadoEnv = EE_LIBERACION;
        break;
      case EE_LIBERACION:
        //Se actua de igual forma que en las fases de ataque
        //y decaida
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

    //La muestra para esta voz se obtiene haciendo una
    //busqueda en la tabla de sintesis, luego multiplicando por
    //el volumen encontrado anteriormente
    muestraVoz =
      highByte(volumen *
               tablaSintesis[highByte(voz[i].posTablaSintPF)]);

    //Se agrega la muestra recien obtenida a la muestra final
    //(se superponen todas las voces para mezclarlas)
    muestraFinal += muestraVoz;

    //Se avanza la posicion de la tabla de sintesis dependiendo
    //de la frecuencia normalizada del tono reproducido. Notese
    //que se elige la cantidad de muestras de la tabla a 256
    //para evitar calculos de modulo (la posicion de la tabla
    //se desborda intencionalmente), y que tambien se emplean
    //sumas de punto fijo para llevar las fracciones
    voz[i].posTablaSintPF += voz[i].frecNorm;
  }

  //Recorte de las muestras en caso de exceder amplitud maxima
  //(innecesario si se controlan las amplitudes)
  //if (muestraFinal > 512) muestraFinal = 512;
  //if (muestraFinal < -512) muestraFinal = -512;

  //Se ajusta el nivel de DC de la muestra final y se envia
  //a los registros de periodo del PWM. El ajuste de DC se
  //hace en la parte alta (Equivale a sumar 256);
  OCR1AH = highByte(muestraFinal) + 1;
  OCR1AL = lowByte(muestraFinal);
}

#endif //SINTESIS_DIRECTA
