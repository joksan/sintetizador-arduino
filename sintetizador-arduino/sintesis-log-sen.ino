#ifdef SINTESIS_LOG_SEN

#include "tonos.h"

//Tabla de funcion exponencial
byte tablaExp[256];
//Tabla de seno logaritmico
byte tablaLogSen[256];

//Duracion del periodo de ataque
const float duracionAtq = 0.01;
//Duracion del periodo de decaida
const float duracionDec = 0.05;
//Duracion maxima del periodo de sostenimiento (en caso no
//se libere la nota)
const float duracionSos = 1.50;
//Duracion maxima del periodo de liberacion (en caso la nota
//se libere de inmediato)
const float duracionLib = 0.20;

//Intensidad del ataque en unidades de amplitud logaritmicas.
//A mayor valor, menor volumen. Puede ser 0 para volumen
//maximo (maximo recomendado de 128).
const unsigned long nivelLogAtq = 0.0;
//Intensidad inicial del sostenimiento (logaritmico tambien)
//Debe tener un valor mayor al nivel de ataque
const unsigned long nivelLogSos = 32.0;

//Calculo de los pasos de atenuacion (en punto fijo) para cada
//etapa de sintesis, empleando la formula:
//Paso_Aten_Etapa = Rango_Logaritmico * Tam_Punto_Fijo * Ts /
//                  Duracion_Etapa
const unsigned long pasoAtenAtqPF =
  (255.0 - (float) nivelLogAtq) * 65536.0 /
  duracionAtq / (F_CPU / 512.0);
const unsigned long pasoAtenDecPF =
  ((float) nivelLogSos - (float)nivelLogAtq) * 65536.0 /
  duracionDec / (F_CPU / 512.0);
const unsigned long pasoAtenSosPF =
  (128.0 - (float) nivelLogSos) * 65536.0 /
  duracionSos / (F_CPU / 512.0);
const unsigned long pasoAtenLibPF =
  (128.0 - (float) nivelLogSos) * 65536.0 /
  duracionLib / (F_CPU / 512.0);

//Estados del automata finito asociado a cada voz
enum ESTADO_ENVOLVENTE {
  EE_APAGADA = 0, EE_ATAQUE, EE_DECAIDA, EE_SOSTENIMIENTO,
  EE_LIBERACION,
};

//Estructura con el estado general de cada voz
struct ESTADO_VOZ {
  ESTADO_ENVOLVENTE estadoEnv;
  word posTablaLogSenPF;
  word frecNorm;
  unsigned long atenuacionPF;
  byte tono;
  bool liberarVoz;
};

//Arreglo de estructuras con las voces
const byte totalVoces = 3;
ESTADO_VOZ voz[totalVoces];

void inicializarTablas() {
  word i;

  //Se calcula la base del logaritmo, el cual genera la maxima
  //amplitud de 127 (1/4 del rango dinamico
  const float base = pow(0.5 / 63.0, 1.0 / 255.0);

  /*
    Serial.print(F("base: "));
    Serial.println(base, 10);

    Serial.println(F("[Tabla Exp]"));
  */

  //Se genera la tabla exponencial
  for (i = 0; i < 256; i++) {
    tablaExp[i] = round(63.0 * pow(base, float(i)));
    /*
      Serial.print(i);
      Serial.print(F(" - "));
      Serial.println(tablaExp[i]);
    */
  }

  //  Serial.println(F("[Tabla Log Sen]"));

  //Se genera la tabla de seno logaritmico
  for (i = 0; i < 256; i++) {
    tablaLogSen[i] =
      round(log(abs(sin((float(i) + 0.5) / 256.0 * 2.0 * PI)))
            / log(base));
    /*
      Serial.print(i);
      Serial.print(F(" - "));
      Serial.println(tablaLogSen[i]);
    */
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
    //voz[i].posTablaLogSenPF = 0;
    voz[i].atenuacionPF = 0x00FFFFFF;
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
  word logMuestra;
  byte atenuacion;
  int muestraVoz;
  int muestraFinal = 0;

  for (i = 0; i < totalVoces; i++) {
    //Para cada una de las voces se obtiene el grado de
    //atenuacion de la misma (almacenada en los bits 16-23
    //de la variable de punto fijo) y se suma con el valor
    //actual de la tabla de logaritmo de seno, con lo que se
    //obtiene el logaritmo de la muestra. Como la suma se hace
    //en el dominio del logaritmo, equivaldra a una
    //multiplicacion en el dominio de la amplitud
    atenuacion = (voz[i].atenuacionPF >> 16) & 0xFF;
    logMuestra = atenuacion + tablaLogSen[highByte(voz[i].posTablaLogSenPF)];

    //Se procede segun el estado de envolvente de la voz
    switch (voz[i].estadoEnv) {
      case EE_APAGADA:
        //En caso de estar apagada, se mata la muestra al
        //forzarla al maximo valor de atenuacion
        logMuestra = 256;
        break;
      case EE_ATAQUE:
        //En el caso del ataque, se disminuye la atenuacion
        //(agranda la amplitud) hasta llegar al nivel maximo
        //de ataque, por medio de decrementar la variable de
        //punto fijo
        if (atenuacion > nivelLogAtq)
          voz[i].atenuacionPF -= pasoAtenAtqPF;
        else
          voz[i].estadoEnv = EE_DECAIDA;
        break;
      case EE_DECAIDA:
        //En el caso de la decaida, se aumenta la atenuacion
        //hasta llegar al nivel de sostenimiento
        if (atenuacion < nivelLogSos)
          voz[i].atenuacionPF += pasoAtenDecPF;
        else
          voz[i].estadoEnv = EE_SOSTENIMIENTO;
        break;
      case EE_SOSTENIMIENTO:
        //En el caso del sostenimiento, se atenua lentamente
        //la voz hasta que la atenuacion llegue a un valor
        //maximo deseable o se libere la voz
        if (atenuacion < 128 && !voz[i].liberarVoz)
          voz[i].atenuacionPF += pasoAtenSosPF;
        else
          voz[i].estadoEnv = EE_LIBERACION;
        break;
      case EE_LIBERACION:
        //Para la liberacion se atenua la voz al ritmo normal
        //hasta llegar a la atenuacion maxima deseable. Tras
        //alcanzar este valor se aplica una atenuacion
        //acelerada para eliminar la voz rapidamente. Al llegar
        //a la atenuacion maxima se apaga la voz.
        if (atenuacion < 128)
          voz[i].atenuacionPF += pasoAtenLibPF;
        else if (atenuacion < 255)
          voz[i].atenuacionPF += pasoAtenLibPF * 16;
        else
          voz[i].estadoEnv = EE_APAGADA;
        break;
    }
    //Nota: Cuando suceden los valores altos de atenuacion
    //(Tipicamente en la segunda mitad de la tabla exponencial)
    //comienza a ocurrir una distorsion armonica poco agradable
    //a causa de la baja relacion de señal a ruido con el ruido
    //de cuantizacion. Basicamente la onda sinusoidal se
    //convierte en una version escalonada de baja resolucion
    //con un contenido armonico muy diferente al original. Este
    //es el motivo por el cual las altas atenuaciones son
    //aceleradas en la ultima etapa de la liberacion, para
    //evitar que sean notadas al ocurrir rapidamente.

    //Se hace la busqueda del logaritmo de la muestra en la
    //tabla exponencial, siempre que su atenuacion no sea
    //demasiado alta y exceda la tabla, en cuyo caso la muestra
    //simplemente se hara cero
    if (logMuestra < 256)
      muestraVoz = tablaExp[logMuestra];
    else
      muestraVoz = 0;

    //Como la tabla de logaritmo de seno solo guarda valores
    //absolutos de amplitud, se hace la correccion de signo
    //en esta ultima etapa, de manera que se invierte siempre
    //que se alcanza la segunda mitad de la misma
    if (highByte(voz[i].posTablaLogSenPF) & 0x80)
      muestraVoz = -muestraVoz;

    //Se agrega la muestra recien obtenida a la muestra final
    //(se superponen todas las voces para mezclarlas)
    muestraFinal += muestraVoz;

    //Se avanza la posicion de la tabla de de logaritmo de seno
    //dependiendo de la frecuencia normalizada del tono
    //reproducido. Notese que se elige la cantidad de muestras
    //de la tabla a 256 para evitar calculos de modulo (la
    //posicion de la tabla se desborda intencionalmente), y que
    //tambien se emplean sumas de punto fijo para llevar las
    //fracciones
    voz[i].posTablaLogSenPF += voz[i].frecNorm;
  }

  //Recorte de las muestras en caso de exceder amplitud maxima
  //(innecesario si se controlan las amplitudes)
  //if (muestraFinal > 511) muestraFinal = 511;
  //if (muestraFinal < -511) muestraFinal = -511;

  //Se ajusta el nivel de DC de la muestra final y se envia
  //a los registros de periodo del PWM. El ajuste de DC se
  //hace en la parte alta (Equivale a sumar 256);
  OCR1AH = highByte(muestraFinal) + 1;
  OCR1AL = lowByte(muestraFinal);
}

#endif //SINTESIS_LOG_SEN
