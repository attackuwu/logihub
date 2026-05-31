<div align="center">

# Logihub for Linux

**Управление RGB-подсветкой Logitech G102 на Linux — без G Hub, без демонов, без root.**

[![GitHub](https://img.shields.io/badge/GitHub-attackuwu%2Floihub-181717?style=for-the-badge&logo=github)](https://github.com/attackuwu/loihub)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-CachyOS%20%2F%20Linux-1793D1?style=for-the-badge&logo=linux&logoColor=white)](https://www.kernel.org/)
[![GTK4](https://img.shields.io/badge/GTK-4.10%2B-7AAD0C?style=for-the-badge&logo=gnome&logoColor=white)](https://www.gtk.org/)
[![Version](https://img.shields.io/badge/version-1.0-success?style=flat-square)](RELEASE.md)

</div>

---

## О проекте

**Logihub for Linux** — нативное приложение на GTK4 для управления подсветкой мыши **Logitech G102 LIGHTSYNC** под Linux.

Программа работает напрямую с мышью через `hidraw` и протокол **HID++ 2.0**.  
Logitech G Hub не нужен.

> **Платформа:** проект разрабатывался и тестировался на **CachyOS**.  
> На других дистрибутивах Linux программа *может* работать, но стабильность не гарантируется.

---

## Возможности

| | |
|---|---|
| 🎨 | **Цветовой круг** — кольцо оттенка + квадрат насыщенности/яркости |
| 🌈 | **4 режима:** статичный цвет, дыхание, перелив цветов, волна |
| 🔆 | **Яркость** 0–100 % |
| ⏱️ | **Скорость** анимированных эффектов |
| ↔️ | **Направление** для режима «волна» |
| ⚡ | **Мгновенное применение** — изменения сразу уходят на мышь |
| 🔑 | **Авто-установка udev** — при первом запуске программа сама попросит пароль и настроит доступ |

---

## Поддерживаемые устройства

| Модель | Статус |
|---|:---:|
| **Logitech G102 LIGHTSYNC** | ✅ 100 % |
| Logitech G203 LIGHTSYNC и похожие мыши на той же технологии | ⚠️ частично |

> «Частично» — устройства с тем же HID++-протоколом и похожим PID могут работать,
> но не тестировались так же тщательно, как G102.

---

## Быстрый старт

### Вариант 1 — готовый бинарник (рекомендуется)

1. Открой [Releases](https://github.com/attackuwu/loihub/releases)
2. Скачай архив **`logihub-1.0-linux-x86_64.tar.gz`**
3. Распакуй и запусти:

```sh
tar xzf logihub-1.0-linux-x86_64.tar.gz
cd logihub-1.0
./logihub
```

При первом запуске появится запрос пароля — программа **сама установит правила udev**.  
Если после этого доступ не появился, переподключи мышь.

> В архиве релиза должны лежать **`logihub`** и папка **`udev/`** рядом с ним.

### Вариант 2 — сборка из исходников

**Зависимости (CachyOS / Arch):**

```sh
sudo pacman -S gtk4 meson gcc pkgconf polkit
```

**Сборка:**

```sh
git clone https://github.com/attackuwu/loihub.git
cd loihub
meson setup build --buildtype=release
meson compile -C build
./build/logihub
```

Правила udev установятся автоматически при первом запуске (через `pkexec`).

---

## Что нужно знать

- Просто **запусти программу** — больше ничего настраивать не нужно
- Либо **скачай готовый бинарник** из Releases, либо **собери из исходников**
- Нужен **Polkit** (`pkexec`) для одноразовой установки udev-правил
- Поддерживается **только проводное USB-подключение**
- Настройки **не сохраняются в память мыши** — после перезагрузки ПК нужно запустить программу снова

---

## Структура проекта

| Файл | Назначение |
|---|---|
| `src/main.c` | Интерфейс + цветовой круг |
| `src/device.c` | HID++ 2.0 — управление подсветкой |
| `src/udev_install.c` | Авто-установка правил udev |
| `udev/99-logitech-hub.rules` | Правила доступа к `/dev/hidraw*` |
| `install-udev.sh` | Ручная установка udev (если нужно) |

---

## Лицензия

Проект распространяется под **[GNU GPL v3](LICENSE)**.

---

<div align="center">

**[Список изменений → RELEASE.md](RELEASE.md)**

</div>
