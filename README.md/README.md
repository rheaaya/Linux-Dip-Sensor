# Linux Light Dip Sensor (ENSC 351 – Assignment 2)

This project reads an LDR (Light Dependent Resistor) value using Linux sysfs GPIO and detects “dips” in brightness.  
It prints the live sensor values and indicates when a dip occurs.

---

## 🔥 Features
- Reads GPIO values from Linux sysfs
- Detects brightness dips using threshold logic
- Clean modular C code
- CMake build support
- Can run on BeagleBone or any Linux target

---

## 📁 Project Structure
AS2/
├── modules/ # Sensor modules and headers
├── src/ # Main program code
├── noworky/ # Broken or test files (optional)
├── build/ # Build directory (ignored by git)
└── CMakeLists.txt

---

## ⚙️ How to Build
From the project root:

```bash
mkdir build
cd build
cmake ..
make
sudo ./dip_sensor
Author

Riya Sondhi
SFU – ENSC 351 (Embedded Systems)