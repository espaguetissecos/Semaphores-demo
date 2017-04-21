# Semaphores-demo
Dos breves y simples programas que sirven como programas de prueba para ver el uso de semáforos y colas de mensajes en procesos.

## Cadenas de montaje

Para éste programa he creado 2 colas de mensajes, una que conecte el proceso A con el B, y otra que conecte el B con el C. El A lee del fichero, y se lo envía al proceso B, cuando termina de leer le envía un mensaje con otro valor para que termine el proceso B. El proceso B si recibe un mensaje para terminar, se encarga de enviar otro mensaje al C para que termine, si no coge el mensaje del A y lo transforma en mayusculas, y lo envía al C. El C lo único que hace es escribir en el archivo de salida, o terminar si se le ordena. 

Hubo un primer planteamiento con una sola cola, pero me di cuenta de que con tanto tamaño de mensaje se llenaba la cola y se interbloqueaban los procesos. 


## Aulas de examen

La lógica para hacer el ejercicio salió casi inmediata, creo una cola de mensajes para que los
hilos estudiantes y profesores se comuniquen entre ellos, ademas de 2 semáforos para controlar
el acceso a la zona de memoria compartida como variables globales, que eran un array de “sitios”
libres del aula, y un contador del numero de alumnos que hay en ella en ese momento.

Empezamos creando todos los recursos, y pidiendo el numero de sitios en el aula ademas de los
alumnos que harán el examen. Después el Gestor pone en marcha a la alarma cuando todos los
alumnos están en el aula para que termine el examen en 5 min si no han salido todos los alumnos,
después espera a todos a que terminen y libera los recursos.


El alumno pide al profesor entrar al aula y si hay espacio el alumno hará el examen y pedirá salir, si
no pedirá entrar en el otro aula hasta que pueda hacer el examen y salir.
El profesor recibe todo tipo de mensajes, el gestor le indica por ellos cuando terminar, y los
alumnos le indican si quieren salir o entrar del aula, si desean entrar activan el mutex lo ponen en
el aula, y le indican si lo han podido poner en el aula o si no, después desactivan el mutex. Si
desean salir activan el mutex, lo sacan y le mandan un mensaje para que salga.
La alarma saca los alumnos del aula, hecha a los profesores, y libera recursos después termina el
proceso.
