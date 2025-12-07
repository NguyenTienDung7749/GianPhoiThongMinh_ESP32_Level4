# 🌞 Giàn Phơi Thông Minh ESP32 (Smart Drying Rack)

Dự án IoT điều khiển giàn phơi quần áo tự động sử dụng ESP32-S3, tích hợp cảm biến thời tiết và điều khiển từ xa qua Firebase.

## 🎯 Tính năng chính

### 🤖 Chế độ Auto (Tự động)
- ☀️ **Phơi ra tự động**: Khi trời sáng và không có mưa
- 🌙 **Thu vào tự động**: Khi trời tối hoặc phát hiện mưa
- 🧠 Logic thông minh dựa trên cảm biến ánh sáng và mưa

### 🎮 Chế độ Manual (Thủ công)
- 📱 Điều khiển từ xa qua web interface (Firebase)
- 🔘 Lệnh điều khiển: `out` (phơi ra), `in` (thu vào), `stop` (dừng)
- 🌐 Chuyển đổi chế độ Auto/Manual từ xa

### 📊 Hiển thị và giám sát
- 🖥️ Màn hình OLED hiển thị:
  - Trạng thái: "Dang Phoi" / "Thu Xong"
  - Nhiệt độ (°C)
  - Độ ẩm (%)
  - Chế độ hoạt động (Auto/Manual)
- 💡 LED chỉ thị trạng thái:
  - LED Đỏ (GPIO 4): Đang thu vào
  - LED Xanh (GPIO 5): Đang phơi ra

### 🔥 Firebase Realtime Database
- ⚡ Đồng bộ dữ liệu realtime (cập nhật mỗi 2 giây)
- 📝 Lưu log lịch sử hoạt động với timestamp
- 📡 Cập nhật trạng thái cảm biến liên tục
- 🔐 Xác thực Anonymous Authentication

## 🔧 Phần cứng sử dụng

| Linh kiện | Model | Chức năng |
|-----------|-------|-----------|
| 🎛️ Vi điều khiển | ESP32-S3 (esp32s3usbotg) | Bộ xử lý chính, kết nối WiFi |
| 🖥️ Màn hình | OLED SSD1306 128x64 | Hiển thị thông tin |
| ⚙️ Động cơ | Servo MG996R | Điều khiển giàn phơi lên/xuống |
| 🌡️ Cảm biến nhiệt độ & độ ẩm | DHT22 | Đo nhiệt độ, độ ẩm không khí |
| ☀️ Cảm biến ánh sáng | LDR Module (Digital Output) | Phát hiện sáng/tối |
| 🌧️ Cảm biến mưa | Rain Sensor (Analog Output) | Phát hiện mưa |
| 💡 LED chỉ thị | LED 5mm x2 | Đỏ (thu) và Xanh (phơi) |

## 📌 Sơ đồ chân kết nối (Pin Mapping)

| Thiết bị | GPIO | Ghi chú |
|----------|------|---------|
| OLED SDA | 8 | I2C Data |
| OLED SCL | 9 | I2C Clock |
| Servo MG996R | 13 | PWM Control |
| DHT22 | 2 | Digital (Data) |
| LDR Module (DO) | 3 | Digital Output |
| Rain Sensor (AO) | 6 | Analog Input |
| LED Thu (Đỏ) | 4 | Digital Output |
| LED Phơi (Xanh) | 5 | Digital Output |

### 🔌 Lưu ý kết nối:
- OLED sử dụng giao thức I2C, địa chỉ: 0x3C
- Servo cần nguồn ngoài 5V (dòng lớn)
- DHT22 cần điện trở kéo lên 10kΩ (thường có sẵn trên module)
- Cảm biến mưa có ngưỡng: < 2000 được coi là có mưa

## 📚 Thư viện sử dụng

Các thư viện được cài đặt qua PlatformIO (xem `platformio.ini`):

```ini
lib_deps =
  adafruit/Adafruit Unified Sensor      # Cảm biến thống nhất
  adafruit/DHT sensor library           # Thư viện DHT22
  adafruit/Adafruit GFX Library         # Đồ họa cơ bản
  adafruit/Adafruit SSD1306             # Điều khiển OLED
  madhephaestus/ESP32Servo              # Điều khiển Servo
  mobizt/Firebase Arduino Client Library for ESP8266 and ESP32  # Firebase
  tzapu/WiFiManager                     # Quản lý WiFi
```

## 🚀 Cài đặt và sử dụng

### 📋 Yêu cầu:
- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO IDE Extension](https://platformio.org/install/ide?install=vscode)
- Tài khoản Firebase (miễn phí)
- Cáp USB Type-C để nạp code cho ESP32-S3

### 🔑 Bước 1: Cấu hình Firebase

1. Tạo project mới trên [Firebase Console](https://console.firebase.google.com/)
2. Vào **Project Settings** → lấy **API Key**
3. Vào **Realtime Database** → tạo database → lấy **Database URL**
4. Copy file `include/secrets.h.example` thành `include/secrets.h`:
   ```bash
   cp include/secrets.h.example include/secrets.h
   ```
5. Mở `include/secrets.h` và điền thông tin Firebase của bạn:
   ```cpp
   #define API_KEY "YOUR_FIREBASE_API_KEY"
   #define DATABASE_URL "https://YOUR_PROJECT.firebaseio.com"
   ```

### 📡 Bước 2: Cấu hình WiFi

Lần đầu khởi động, ESP32 sẽ tạo Access Point với tên: **SmartDrying-Setup**
1. Kết nối vào WiFi "SmartDrying-Setup"
2. Trình duyệt sẽ tự động mở trang cấu hình (hoặc vào 192.168.4.1)
3. Chọn mạng WiFi nhà bạn và nhập mật khẩu
4. ESP32 sẽ tự động kết nối và lưu thông tin

### 💻 Bước 3: Build và Upload

1. Mở thư mục project trong VS Code
2. PlatformIO sẽ tự động cài đặt các dependencies
3. Nhấn nút **Upload** (→) trên thanh công cụ PlatformIO
4. Mở Serial Monitor (tốc độ: 115200 baud) để xem log

### 🔍 Bước 4: Kiểm tra hoạt động

Sau khi upload thành công:
- Màn hình OLED sẽ hiển thị trạng thái
- Kiểm tra Firebase Console → Realtime Database để xem dữ liệu
- LED sẽ sáng theo trạng thái (Đỏ = Thu, Xanh = Phơi)

## 🗂️ Cấu trúc Firebase Realtime Database

```json
{
  "system": {
    "state": "out" | "in" | "booted",
    "mode": "auto" | "manual",
    "command": "out" | "in" | "stop",
    "sensor": {
      "temperature": 28.5,
      "humidity": 75.0,
      "light": 1,
      "rain": 0
    }
  },
  "logs": {
    "1732470100": {
      "state": "out",
      "mode": "auto",
      "ts": 1732470100,
      "reason": "auto_bright"
    },
    "1732470200": {
      "state": "in",
      "mode": "auto",
      "ts": 1732470200,
      "reason": "auto_rain"
    }
  }
}
```

### 📖 Giải thích các trường:

#### `/system/state`
- `"booted"`: Hệ thống vừa khởi động
- `"out"`: Giàn phơi đang ở ngoài (phơi)
- `"in"`: Giàn phơi đã thu vào

#### `/system/mode`
- `"auto"`: Chế độ tự động
- `"manual"`: Chế độ điều khiển thủ công

#### `/system/command`
Lệnh điều khiển từ web (chỉ có tác dụng khi `mode = "manual"`):
- `"out"`: Phơi ra
- `"in"`: Thu vào
- `"stop"`: Dừng servo

#### `/system/sensor`
- `temperature`: Nhiệt độ (°C)
- `humidity`: Độ ẩm (%)
- `light`: 1 = Sáng, 0 = Tối
- `rain`: 1 = Có mưa, 0 = Không mưa

#### `/logs/{timestamp}`
Lịch sử hoạt động với Unix timestamp (giây)
- `reason`: Lý do thay đổi trạng thái
  - `"auto_bright"`: Tự động phơi vì trời sáng
  - `"auto_dark"`: Tự động thu vì trời tối
  - `"auto_rain"`: Tự động thu vì có mưa
  - `"auto_rain_cleared"`: Tự động phơi vì hết mưa
  - `"manual_out"`: Điều khiển thủ công phơi ra
  - `"manual_in"`: Điều khiển thủ công thu vào

## 🧠 Logic hoạt động

### 🤖 Chế độ Auto (Tự động)

```
┌─────────────────────────────────────┐
│  Đọc cảm biến (mỗi 200ms)          │
│  - Ánh sáng (LDR)                   │
│  - Mưa (Rain Sensor)                │
└──────────┬──────────────────────────┘
           │
           ▼
    ┌──────────────┐
    │ Trời sáng +  │ YES  ┌──────────────┐
    │ Không mưa?   │─────→│ PHƠI RA      │
    └──────────────┘      │ - Servo quay  │
           │ NO           │ - LED Xanh ON │
           ▼              │ - Upload log  │
    ┌──────────────┐      └──────────────┘
    │ Trời tối     │ YES  ┌──────────────┐
    │ hoặc mưa?    │─────→│ THU VÀO      │
    └──────────────┘      │ - Servo quay  │
                          │ - LED Đỏ ON   │
                          │ - Upload log  │
                          └──────────────┘
```

**Chi tiết:**
- ☀️ **Trời sáng + Không mưa** → Phơi ra
  - Servo quay (PWM = 1300) trong 3 giây
  - LED Xanh sáng, LED Đỏ tắt
  - OLED hiển thị "Sang" → "Phoi Ra" → "Dang Phoi"
  - Upload log với reason: `"auto_bright"` hoặc `"auto_rain_cleared"`

- 🌙 **Trời tối HOẶC Có mưa** → Thu vào
  - Servo quay ngược (PWM = 1700) trong 3 giây
  - LED Đỏ sáng, LED Xanh tắt
  - OLED hiển thị "Toi" hoặc "Mua" → "Thu Vao" → "Thu Xong"
  - Upload log với reason: `"auto_dark"` hoặc `"auto_rain"`

### 🎮 Chế độ Manual (Thủ công)

```
┌─────────────────────────────────────┐
│  Đọc Firebase /system/command       │
│  (mỗi 1 giây)                       │
└──────────┬──────────────────────────┘
           │
           ▼
    ┌──────────────┐
    │ command =    │──→ "out"  ──→ PHƠI RA
    │ ?            │
    │              │──→ "in"   ──→ THU VÀO
    │              │
    │              │──→ "stop" ──→ DỪNG SERVO
    └──────────────┘
```

**Chi tiết:**
- Cả 2 LED sáng cùng lúc khi ở chế độ Manual
- OLED luôn hiển thị "Mode / Manual"
- Không tự động phơi/thu dựa trên cảm biến
- Chỉ thực hiện lệnh từ Firebase

### 🖥️ Hiển thị OLED

**Chế độ Auto:**
Xoay vòng mỗi 10 giây:
1. Trạng thái → "Dang Phoi" / "Thu Xong"
2. Nhiệt độ → "Nhiet Do / XX°C"
3. Độ ẩm → "Do Am / XX%"

**Chế độ Manual:**
Luôn hiển thị: "Mode / Manual"

## 🔄 Quy trình hoạt động đầy đủ

1. **Khởi động (Setup)**
   - Kết nối WiFi (WiFiManager)
   - Đồng bộ thời gian NTP (UTC+7)
   - Xác thực Firebase (Anonymous)
   - Khởi tạo OLED, Servo, Cảm biến
   - Ghi trạng thái `"booted"` lên Firebase

2. **Vòng lặp chính (Loop)**
   - Đọc cảm biến mỗi 200ms
   - Đọc DHT22 mỗi 5 giây
   - Đọc Firebase command mỗi 1 giây
   - Upload sensor data mỗi 2 giây
   - Xử lý logic Auto/Manual
   - Cập nhật OLED

3. **Ghi log**
   - Mỗi khi thay đổi trạng thái (phơi/thu)
   - Lưu vào `/logs/{timestamp}` với:
     - state: "out" hoặc "in"
     - mode: "auto" hoặc "manual"
     - ts: Unix timestamp
     - reason: Lý do thay đổi

## 🛠️ Tùy chỉnh

### Thay đổi ngưỡng cảm biến mưa:
```cpp
// Trong main.cpp, dòng 53
int RAIN_THRESHOLD = 2000;  // Giảm xuống nếu nhạy hơn, tăng lên nếu kém nhạy
```

### Thay đổi thời gian servo chạy:
```cpp
// Trong main.cpp, dòng 45
const int MOVE_TIME = 3000;  // milliseconds
```

### Thay đổi tốc độ servo:
```cpp
// Trong main.cpp, dòng 43-44
const int FORWARD_PWM = 1300;  // Phơi ra (giảm = nhanh hơn)
const int REVERSE_PWM = 1700;  // Thu vào (tăng = nhanh hơn)
```

## 🐛 Xử lý lỗi thường gặp

### ❌ Không kết nối được WiFi
- Kiểm tra tên mạng và mật khẩu
- Reset cấu hình: giữ nút BOOT khi khởi động
- Kết nối vào "SmartDrying-Setup" và cấu hình lại

### ❌ Firebase không kết nối
- Kiểm tra `API_KEY` và `DATABASE_URL` trong `secrets.h`
- Đảm bảo Realtime Database đã được tạo và rules cho phép ghi/đọc
- Kiểm tra kết nối internet của ESP32

### ❌ OLED không hiển thị
- Kiểm tra kết nối I2C (SDA=8, SCL=9)
- Thử đổi địa chỉ I2C (0x3C hoặc 0x3D)
- Test bằng I2C scanner

### ❌ DHT22 trả về NaN
- Kiểm tra kết nối điện
- Đảm bảo có điện trở kéo lên 10kΩ
- Đợi ít nhất 2 giây sau khi khởi động

### ❌ Servo không quay
- Kiểm tra nguồn 5V (cần dòng ít nhất 1A)
- Kiểm tra kết nối GPIO 13
- Test bằng code đơn giản (writeMicroseconds)

## 📝 License

Dự án này được phát hành dưới giấy phép MIT. Bạn có thể tự do sử dụng, chỉnh sửa và phân phối.

## 👨‍💻 Tác giả

**NguyenTienDung7749**

---

⭐ Nếu bạn thấy dự án hữu ích, hãy cho một star trên GitHub!

📧 Có vấn đề? Mở [Issue](https://github.com/NguyenTienDung7749/GianPhoiThongMinh_ESP32_Level3/issues) để được hỗ trợ.
