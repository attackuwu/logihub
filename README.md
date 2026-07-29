# LogiHub for Linux

**Открытая альтернатива Logitech G HUB для управления RGB-подсветкой мышей Logitech под Linux.**  
Нативный C + GTK4. Без Wine. Без проприетарного ПО.

[![Лицензия](https://img.shields.io/badge/лицензия-CC0%201.0-blue?style=flat-square)](LICENSE)
[![Платформа](https://img.shields.io/badge/платформа-Linux%20x86__64-1793d1?style=flat-square&logo=linux&logoColor=white)](https://github.com/attackuwu/logihub/releases)
[![GTK](https://img.shields.io/badge/GTK-4.10+-3584e4?style=flat-square&logo=gtk&logoColor=white)](https://gtk.org)
[![Устройство](https://img.shields.io/badge/устройство-Logitech_G102_LIGHTSYNC-success?style=flat-square)](https://www.logitechg.com)

---

## Возможности

| Возможность | Описание |
|-------------|----------|
| **HSV-цветовое колесо** | Интерактивный выбор цвета: кольцо оттенка + квадрат насыщенности/яркости |
| **4 режима подсветки** | Статичный, Дыхание, Перелив цветов, Волна |
| **Регулировка яркости** | От 0% до 100% |
| **Скорость анимации** | Настраиваемая скорость для Breathing / Cycle / Wave |
| **Направление волны** | Вправо или влево |
| **Выключение подсветки** | Мгновенное отключение всей подсветки |
| **Автоустановка udev** | Установка udev-правил при первом запуске через `pkexec` — ручная настройка не требуется |
| **Переподключение** | Кнопка восстановления связи с устройством при её потере |

---

## Поддерживаемые устройства

**Полная поддержка гарантируется только для Logitech G102 LIGHTSYNC.**  
Остальные модели могут работать, но официально не тестировались.

| Модель | USB ID | Статус |
|--------|:------:|:------:|
| **Logitech G102 LIGHTSYNC** | `046d:c092` | Полная поддержка |
| Logitech G102 (старая ревизия) | `046d:c084` | Вероятно совместима |
| Logitech G203 LIGHTSYNC | `046d:c08b` | Вероятно совместима |
| Другие мыши Logitech | — | Не тестировались |

Инструкция по добавлению нового устройства: [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Поддержка дистрибутивов

| Дистрибутив | Статус |
|-------------|:------:|
| **Fedora Kinoite 44** | Основная платформа разработки — полная поддержка |
| Arch Linux / Manjaro / EndeavourOS | Ожидается работа (тот же набор пакетов) |
| Fedora 39+ | Требуется GTK4 ≥ 4.10 |
| Ubuntu 24.04+ | Требуется GTK4 ≥ 4.10 |
| Прочие дистрибутивы | Совместимость не гарантирована — тестирование приветствуется |

Готовые бинарные сборки компилируются под CachyOS (x86_64). На других системах рекомендуется сборка из исходников.

---

## Установка

### Вариант 1: Готовый бинарник (релиз)

```bash
wget https://github.com/attackuwu/logihub/releases/download/1.0/logihub-1.0-linux-x86_64.tar.gz
tar xzf logihub-1.0-linux-x86_64.tar.gz
cd logihub-1.0
./logihub
```

При первом запуске появится окно `pkexec` — подтвердите установку udev-правил.  
После установки **переподключите мышь** (USB-кабель или приёмник).

### Вариант 2: Сборка из исходников

```bash
git clone https://github.com/attackuwu/logihub.git
cd logihub
meson setup build --buildtype=release
meson compile -C build
./build/logihub
```

---

## Зависимости для сборки

| Пакет | Мин. версия | Назначение |
|-------|:----------:|------------|
| `meson` | ≥ 1.0 | Система сборки |
| `ninja` | — | Исполнитель сборки |
| `gcc` или `clang` | C17 | Компилятор |
| `pkg-config` | — | Поиск библиотек |
| `gtk4` | ≥ 4.10 | Графический тулкит |
| `glib` | (в составе GTK4) | Базовые типы и события |

**CachyOS / Arch Linux:**
```bash
sudo pacman -S meson ninja gcc pkgconf gtk4
```

**Ubuntu / Debian:**
```bash
sudo apt install meson ninja-build build-essential pkg-config libgtk-4-dev
```

---

## Сборка релизного архива

```bash
./package-release.sh
# → release/logihub-1.0-linux-x86_64.tar.gz
```

---

## Известные ограничения

- Поддерживаемые USB PID: только `c092`, `c084`, `c08b`
- Готовые бинарники: только Linux x86_64 (собраны на CachyOS)
- Язык интерфейса: русский (английская локализация в планах)
- Функционал: только RGB-подсветка — без профилей, макросов и настроек DPI
- Требуется GTK4 ≥ 4.10 — может отсутствовать в некоторых LTS-дистрибутивах

---

## Участие в разработке

Приветствуется любой вклад. Наиболее востребованные направления:

- Добавление поддержки новых мышей Logitech
- Сообщения об ошибках с воспроизводимыми шагами и логами
- Улучшение интерфейса (GTK4 + CSS)
- Документация, переводы
- Пакетирование (AUR, Flatpak, COPR)

Подробнее: [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Лицензия

**CC0 1.0 Universal** — полный текст в файле [LICENSE](LICENSE).

```
SPDX-License-Identifier: CC0-1.0

Насколько это возможно по закону, авторы передали эту работу
в общественное достояние согласно CC0 1.0 Universal.
```

---

## Правовая информация

Logitech, G HUB и LIGHTSYNC являются зарегистрированными товарными знаками Logitech International S.A.  
Данный проект не аффилирован с Logitech, не одобрен и не спонсируется ею.

---

<p align="center">
  <a href="https://github.com/attackuwu/logihub">GitHub</a> ·
  <a href="https://github.com/attackuwu/logihub/issues">Сообщить о проблеме</a> ·
  <a href="CONTRIBUTING.md">Стать участником</a>
</p>
