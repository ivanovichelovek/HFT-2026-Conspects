# Проблема сырых сокетов
При использовании сырых сокетов на клиенте следует заблокировать RST пакеты от порта, с которого мы хотим обслуживать соединение

Linux style
```
sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST --sport 54321 -j DROP

sudo iptables -D OUTPUT -p tcp --sport 54321 --tcp-flags RST RST -j DROP # Удалить
```

MacOS style

```
echo "block out proto tcp from any port 54321 flags R/R" | sudo pfctl -f -

sudo pfctl -f /etc/pf.conf # Чтобы вернуть
```

Происходит это поскольку ядро ОС не знает о том, что на порту есть TCP сокет (мы заботимся о нем сами), поэтому по стандарту RFC 793 (TCP) стэк должен отправить RST пакет на сервер, сказав "Я не открывал этого соединения, закрой его".