# sintetizador-arduino
Un sintetizador de audio para Arduino Uno

Este sketch de Arduino permite generar sonidos musicales de una calidad muy superior a la tradicional funcion tone, empleando la tecnica de sintesis mediante tablas pre-computadas.

El instrumento que viene configurado por defecto pretende sonar de manera muy similar a un piano, pero existe la posibilidad de cambiar los parametros de sintesis para crear otros instrumentos.

De momento el sketch opera en forma individual (sin conexion a la PC) pero se tienen planes para implementar una interfase midi que permita operarlo desde la PC.

Modos de operacion
---
- Modo capacitivo
Permite operar el sintetizador usando objetos metalicos como latas de gaseosa, o bien cualquier cosa que conduzca muy bien la electricidad. Tiene la ventaja que basta con tocar los objetos para activar las teclas y no requiere switches ni cables de tierra conectados al cuerpo.
Esta modalidad requiere la libreria CapacitiveSensor, que se puede encontrar aqui: http://playground.arduino.cc/Main/CapacitiveSensor?from=Main.CapSense
Nota: Se recomiendan resistencias de 1 Mega Ohm para el circuito, asi las notas solo suenan al tocar directamente los objetos metalicos y no por aproximarse a ellos.

- Modo resistivo
Este modo emplea la misma tecnica de sensado resistivo que el makey makey. Tiene la ventaja que se puede emplear cualquier objeto que conduzca la electricidad aun cuando sea pobremente, tal como frutas, comida, grafito (dibujos hechos en lapiz por ejemplo), pintura conductora, etc. sin embargo requiere que el cuerpo del usuario este conectado al tierra del Arduino (mediante un brazalete por ejemplo o a travez de tocar un cable con las manos).
Se recomiendan resistencias de 10 Mega Ohm como pull-ups si se usa con objetos conductores.
En adicion a los objetos conductores (pobres) de electricidad, pueden emplearse botones convencionales conectados a tierra para operarlo, en cuyo caso se recomiendan pull-ups de 10 Kilo Ohm. Puede crearse un teclado con switches de finales de carrera o cualquier otra tecnica convencional de circuitos.

Creado por Joksan Alvarado, distribuido al publico general bajo licencia GPL Version 3