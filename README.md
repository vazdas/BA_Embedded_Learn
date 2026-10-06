# Embedded Systems Learning Repository 🚀

Репозиторій із практичними завданнями та проєктами в межах курсу **Embedded Engineer** від **Beetroot Academy**.

---

## 🛠 Технологічний стек

* **Мікроконтролери:** ESP32, STM32 (Family F1/F4)
* **Фреймворки та бібліотеки:** ESP-IDF (FreeRTOS), STM32 HAL
* **Середовища розробки (IDE):** VS Code (PlatformIO), STM32CubeIDE, STM32CubeMX
* **Мова програмування:** C / C++

---

## 📁 Структура репозиторію

* **`ESP32_IDF/`** — проєкти та лабораторні на базі ESP-IDF та FreeRTOS.
* **`STM32_HAL/`** — проєкти для мікроконтролерів STM32 з використанням HAL драйверів.

---

## 📋 Список виконаних проєктів

| Категорія | Назва проєкту | Опис | Стек |
| :--- | :--- | :--- | :--- |
| **ESP32_IDF** | `01_gpio_blink` | Базова робота з GPIO та тасками FreeRTOS | C, ESP-IDF |
| **STM32_HAL** | `01_timer_pwm` | Генерація PWM сигналу через Таймер 2 | C, STM32CubeMX |

---

## ⚙️ Як зібрати та прошити проєкти

1. **ESP-IDF (VS Code / CLI):**
   ```bash
   cd ESP32_IDF/назва_проєкту
   idf.py build
   idf.py flash monitor
