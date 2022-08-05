Calentador cled

README

Calentador con LED tiene seteado un sistema muy rústico para hacer un indicador de estado con un solo led.

Dos pulsos cortos(-_-_____): No hay cliente conectado
	Nota: La implementación ahora mismo vuelve a este estado porque lo até al evento de desconexión de cliente, y no a la cantidad de clientes == 0.

Un Pulso largo (----____): Cliente conectado, potencia Apagada

Luz prendida (--------): Cliente Conectado, Potencia Prendida

----------------------------

Viendo ahora lo que escribí, la implementación no es la más limpia/razonable, pero creo que el idioma en general del led se podrá entender.