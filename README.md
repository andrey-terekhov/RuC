[![Build Status](https://travis-ci.org/andrey-terekhov/RuC.svg?branch=master)](https://travis-ci.org/andrey-terekhov/RuC)
# ![Logo](https://raw.githubusercontent.com/Victor-Y-Fadeev/RuC-WPF/master/RuC.WPF/Images/Repository.png) Russian C

Этот репозиторий содержит компилятор языка RuC.
Авторство принадлежит [Андрею Николаевичу Терехову](https://github.com/andrey-terekhov),
заведующему кафедрой "Системного программирования" СПбГУ.

## Сборка

При первоначальном использовании необходимо установить некоторые утилиты:
```
$ sudo apt-get -y install cmake clang-tidy clang-format-9
```

Для конфигурирования и сборки репозитория воспользуйтесь следующими командами:
```
$ mkdir build && cd build && cmake .. && cd ..
$ cmake --build build --config Release
```

Так как в сборке используется CMake, имеется возможность генерации проекта для IDE, например Xcode:
```
$ cmake . -G Xcode
```
