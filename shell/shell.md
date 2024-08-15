# Lab: shell

### Búsqueda en $PATH

**¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?**
La diferencia entre la syscall execve y la familia de wrappers de C exec es que reciben distintos argumentos y tienen diferentes funcionalidades, 
además de ejecutar el binario indicado, como hacen todas estas funciones. 
Por ejemplo, la syscall execve recibe la ruta al binario, un array de argumentos y un array de variables de entorno, y se ejecuta ese binario 
con esos parámetros indicados. 
En cambio, el wrapper de C execvp solamente recibe el nombre del binario y un array de argumentos, por lo que hay que setear variables de entorno 
temporarias si se quieren usar en este binario. 
En execvp, otra funcionalidad es que si el nombre del binario no incluye un slash (/), se busca el binario en una lista de paths que hay en la
 variable de entorno PATH, algo que execve no hace.
Como podemos ver, todas estas funciones cumplen con la funcionalidad básica de ejecutar otro binario, es decir que todos los wrappers de C terminan 
llamando a la syscall para que el kernel ejecute el binario deseado.
La diferencia es que los wrappers de C agregan funcionalidades extra, o distintas formas de ejecutar un binario.

**¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?**
La llamada a exec puede fallar. En nuestra implementación de la shell, chequeamos el valor de retorno de la llamada a exevp. Si devuelve -1, 
significa que hubo un error en exec, y entonces imprimimos un mensaje de error a stderr. La shell no se interrumpe, sino que se hace una salida 
forzosa del exec_cmd y se espera a que el usuario ingrese nuevos comandos.

---

### Comandos built-in

**¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, 
cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)**
Los comandos `cd` y `pwd` podrían ser implementados sin necesidad de ser built-in, sin embargo, esto sería menos 
eficiente debido a que cada vez que se ejecutara este comando externo la shell debería crear un nuevo proceso para 
ejecutar el comando, costando más recursos que si fuese built-in.
Además, se hacen built-in debido a que muchos de estos comandos se utilizan en conjunto con otros comandos para ejecutar tareas más especificas.

---

### Variables de entorno adicionales
**¿Por qué es necesario hacerlo luego de la llamada a fork(2)?**
El fork al que hace referencia la pregunta, se trata del fork que se hace en la función run_cmd() del archivo runcmd.c. Si la asignación de variables 
de entorno temporales se hiciera previo al fork, lo que ocurriría es que dichas variables no serían temporales. Formarían parte de toda la shell, 
y no únicamente del programa que las requiere. En cambio, si la asignación ocurre luego del fork (y en la rama del proceso hijo), las variables vivirán 
únicamente en el contexto del programa y dejarán de existir una vez que el programa finalice (lo que las hace temporales).

**En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una
     lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de 
     utilizar setenv(3) por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de exec(3).**
**¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.**
**Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.**
Estos wrappers al recibir un arreglo de variables de entorno por parámetro, limitan al programa a poder usar únicamente dichas variables. Es decir, el 
programa no podrá utilizar las variables de entorno que estaban definidas por anterioridad (definidas por la shell). Si alguien utiliza uno de estos 
wrappers, debe considerar que las variables de entorno que dispondrá el programa a ejecutar serán únicamente las formen parte del arreglo que recibe el exec.
Definiendo las variables de entorno manualmente con setenv, uno se asegura que el programa a ejecutar seguirá teniendo acceso a todas las variables de 
entorno definidas previamente (más las que defina). Si se quiere "replicar" esta lógica lo que se debería hacer es cargar al arreglo TODAS las variables 
de entorno: las temporales y las que fueron creadas por la shell, y pasarle este arreglo a alguno de los wrappers exec (finalizados con la letra e).

---

### Procesos en segundo plano

**Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.**
Cuando se ejecuta un comando con el caracter `&`, luego del parseo de la linea a ejecutar se crea una instancia del `struct cmd` de tipo BACK. 
Cuando run_cmd hace un fork y llama a ejecutar al comando en el proceso hijo, se chequea con un switch en exec_cmd que el tipo de cmd sea BACK. 
En ese caso, se castea el struct cmd a un struct backcmd. Luego, se llama recursivamente a exec_cmd con el struct cmd que hay dentro del struct backcmd, 
para que se ejecute el comando que se desea que corra en background (ahora el switch entra en el case EXEC y se hace el execvp del binario indicado). 
Una vez hecho esto se vuelve a run_cmd, y ahora en el proceso padre se chequea si el comando es un BACK, y en ese caso se imprime información sobre este 
proceso (su PID). Por último se ejecuta el wait para que no queden procesos background en estado zombie. Para hacer esto está la línea 
`while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)` que hace wait a todos los procesos zombie que haya en la shell, que pueden ser varios ya que se pueden 
correr varios procesos background al mismo tiempo.

---

### Flujo estándar

**Investigar el significado de 2>&1, explicar cómo funciona su forma general
Mostrar qué sucede con la salida de cat out.txt en el ejemplo.
Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el
comportamiento en bash(1).**

Con el operador > se pueden hacer redirecciones. En este caso estamos redireccionando el fd2 (stderr) hacia el fd1 con
el operador &, correspondiente a stdout.
Usando la syscall dup(2) se pueden escribir en el fd2 para luego ser printeados por pantalla.

``` bash
$ ls -C /home /noexiste >out.txt 2>&1
$ cat out.txt 
ls: cannot access '/noexiste': No such file or directory
/home:
user
```

Orden inverso:

``` bash
$ ls -C /home /noexiste 2>&1 >out.txt
$ cat out.txt 
ls: cannot access '/noexiste': No such file or directory
/home:
user
```

El fd1 apunta a stdout, al hacer el 2>&1 estamos redireccionando el fds2 a stdout.
Luego redireccionamos el fds1 hacia out.txt, pero no volvemos a redireccionar el fds2, por lo
tanto, todo lo que se escriba allí será printeado en pantalla.

---

### Tuberías simples (pipes)
**Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe.**

¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal)
de este comportamiento usando bash. Comparar con su implementación.



Cuando se ejecuta un pipe, el exit code que considera la shell es sobre el último comando que ejecutó el pipe.
Independientemente del resultado previo que se haya ejecutado antes del comando final del pipe.


**BASH**
``` bash
$ ls | ls /somedir
ls: cannot access '/somedir': No such file or directory

$ echo $?
2



$ ls /somedir | ls
ls: aws         Documents  Music     out.txt        project  sql        Videos
cannot access '/somedir'cloud-keys  Downloads  out1.txt  Pictures       Public   studio3t
Desktop     go         out2.txt  postgres-data  snap     Templates
$ echo $?
0

```

**SHELL**
``` bash
$ ls | ls /somedir
ls: cannot access '/somedir': No such file or directory

$ echo $?
0




$ ls /somedir | ls
ls: cannot access '/somedir': No such file or directory
aws  cloud-keys  Desktop  Documents  Downloads  go  Music

$ echo $?
0


```

---

### Pseudo-variables
**Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).**
- `$$`: Devuelve el PID programa en ejecución (en este caso, de la instancia de shell que se está ejecutando).
``` bash
$ echo $$
11015
```

- `$0`: Devuelve el nombre del último comando/script ejecutado.
``` bash
$ echo $0
bash
```
``` bash
$ ./script.sh Hello World
$ echo $0
./script.sh
```

- `$_`: Devuelve el último argumento del último comando/script ejecutado.
``` bash
$ echo Hello World
$ echo $_
World
```

---

### Historial

**¿Cuál es la función de los parámetros `MIN` y `TIME` del modo no canónico? ¿Qué se logra en el ejemplo dado al establecer a `MIN` en 1 y a `TIME` en 0?**
Los parámetros `MIN` y `TIME` del modo no canónico permiten controlar si se recibe input y cuánto tiempo esperar para que esté disponible.
También se pueden utilizar para no tener que esperar, y retornar inmediatamente con el input disponible, o sin input.
Si se establece a `MIN` en 1 y a `TIME` en 0, read espera hasta que haya 1 byte en la cola (si `MIN` es n, se espera hasta que haya n bytes en la cola),
y luego devuelve los caracteres disponibles en la cola, que en este caso es sólo 1. read puede devolver más caracteres que los indicados por MIN
si hay más de `MIN` caracteres en la cola. El parametro `TIME` en 0 indica que no se espera luego de cada input para ver si llega más input.
Fuente: https://www.gnu.org/software/libc/manual/html_node/Noncanonical-Input.html

---