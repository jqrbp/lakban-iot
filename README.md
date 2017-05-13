# lakban iot
Mengendalikan relay menggunakan websocket dan http request.

Peralatan:
1. Raspberry pi => sebagai host penyedia webserver, reverse proxy (http dan websocket).
2. Wemos d1     => sebagai host penyedia webserver dan websocket server.

Perangkat lunak:
Library:
1. Golang digunakan pada Raspberry pi
2. Wemos menggunakan library dari arduino
Editor:
1. Platformio untuk pemrograman wemos
2. Text Editor biasa

Wemos dan Raspberry pi disambungkan pada satu network (WIFI) yang sama.
Keduanya memiliki webserver yang dapat diakses lewat ip address-nya.
Webserver pada wemos juga dapat diakses di dalam raspberry pi webserver lewat reverse proxy.
