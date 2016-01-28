# sintetizador-arduino
Un sintetizador de audio para Arduino Uno

Este sketch de Arduino permite generar sonidos musicales de una calidad muy superior a la tradicional funcion tone, empleando tecnicas de sintesis mediante tablas pre-computadas.

El instrumento que viene configurado por defecto pretende sonar de manera muy similar a un piano, pero existe la posibilidad de cambiar los parametros de sintesis para crear otros instrumentos.

El sketch puede operar de forma individual conectando switches/contactos, sensores resistivos o sensores capacitivos a los pines del Arduino. Tambien puede ser controlado mediane interfase midi al conectarlo a una PC. Se pueden emplear ambas tecnicas de control de forma simultanea.

Para la conexion a una PC se recomienda un software que traslade el protocolo midi al puerto serial del arduino. El software recomendado es Hairless Midi to Serial Bridge, pero pueden usarse otros.
http://projectgus.github.io/hairless-midiserial/

Sin embargo un programa como Hairless Midi to Serial Bridge no puede enviar las notas musicales por si mismo, por lo que se recomienda acompañarlo de un teclado virtual, tal como vmpk.
http://vmpk.sourceforge.net/

Modos de operacion
------------------
- Modo capacitivo:
Permite operar el sintetizador usando objetos metalicos como latas de gaseosa, o bien cualquier cosa que conduzca muy bien la electricidad. Tiene la ventaja que basta con tocar los objetos para activar las notas y no requiere switches ni cables de tierra conectados al cuerpo.  
Esta modalidad requiere la libreria CapacitiveSensor, que se puede encontrar aqui: http://playground.arduino.cc/Main/CapacitiveSensor?from=Main.CapSense  
Nota: Se recomiendan resistencias de 1 Mega Ohm para el circuito, asi las notas solo suenan al tocar directamente los objetos metalicos y no por aproximarse a ellos.  

- Modo resistivo:
Este modo emplea la misma tecnica de sensado resistivo que el makey makey. Tiene la ventaja que se pueden emplear objetos que conduzcan la electricidad aun cuando sea pobremente, tales como frutas, comida, grafito (dibujos hechos en lapiz por ejemplo), pintura conductora, etc. sin embargo requiere que el cuerpo del usuario este conectado al tierra del Arduino (mediante un brazalete por ejemplo o a travez de tocar un cable con las manos).  
Se recomiendan resistencias de 10 Mega Ohm como pull-ups para este modo de operacion.  

- Modo de contacto:
Aca se pueden emplearse botones convencionales conectados a tierra para operar el sintetizador (ej.: switches de fin de carrera) o bien cualquier otra forma de contacto (switches improvisados con clavos, alambres, etc.) que permitan esta operacion. En esta modalidad no hacen falta pull-ups externos, ya que se activan los pull-up internos del Arduino.

Modos de sintesis
-----------------
- Sintesis directa:
Crea los sonidos aplicando multiplicaciones a una tabla de onda sinusoidal. Permite crear sonidos ricos en armonicos pero tiene la limitante que no se pueden atenuar en la fase de sostenimiento. Este es el metodo por defecto y esta pre-configurado para sonar de forma similar a un piano.

- Sintesis por logaritmo de seno:
Crea los sonidos aplicando tablas de funciones exponenciales y logaritmos de seno. Permite atenuar el volumen en la fase de sostenimiento, pero no se pueden agregar armonicos por causa de la eliminacion del signo de la funcion seno. No viene activado por defecto, pero esta pre-configurado para sonar de manera similar a una marimba.

Creado por Joksan Alvarado, distribuido al publico general bajo licencia GPL Version 3
