# Скрипты для сборки F-Stack/DPDK

## Старт

1. **Настройка переменных:**
```bash
nano 00_config.sh  # Измените под свою систему
```
2. **Полная сборка**
```bash
chmod +x *.sh
./build_all.sh
```
3. **Запуск**
```bash
# Терминал 1
cd $APP_DIR && ./server_app

# Терминал 2
./08_setup_hugepages.sh
./09_run_client.sh
```
