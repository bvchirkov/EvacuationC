# EvacuationC

**EvacuationC** -- программа моделирования движения людей в здании


# Сборка

## Интсрументарий
- GNU/Linux \*ubuntu >= 18.04
- cmake >= 3.16
- gcc-10
- [json-c 0.13](https://github.com/json-c/json-c/releases/tag/json-c-0.13.1-20180305)


Клонируйте репозиторий
``` bash
git clone git@github.com:bvchirkov/EvacuationC.git
```
Выполните настройку окружения и сборку проекта
``` bash
cd EvacuationC
mkdir build
cmake -S  . -B build/ && cmake --build build/
```
Готовый к запуску файл расположен в дирректории `build/` -- `EvacuationC`
