# Dự án 6: đèn WS2812 và quản lý điện năng

Phiên bản này dùng cùng phần cứng như `5_rainmaker`: ESP32-C3, dải 8 LED WS2812 có chân dữ liệu nối GPIO 3 và nút ở GPIO 9. Đèn được điều khiển qua ESP RainMaker hoặc app tương thích của dự án.

Yêu cầu ESP-IDF 5.4.4 và submodule `device_firmware/third_party/esp-rainmaker` (v1.17.0). Nếu dùng checkout khác, đặt `RAINMAKER_PATH` trỏ tới thư mục đó. Flash 4 MB và bảng phân vùng giống dự án 5; không dùng bảng 2 MB của ví dụ ban đầu.

Trong PowerShell đã thiết lập môi trường ESP-IDF, chạy tại thư mục này:

```powershell
idf.py build
idf.py -p COM4 -b 460800 flash monitor
```

Thay COM4 bằng cổng của board. Để thoát monitor, nhấn `Ctrl+]`. Nếu board chưa được cấu hình mạng, dùng app quét QR hoặc nhập tên thiết bị và mã POP hiện trên monitor. Nhấn ngắn nút GPIO 9 để bật/tắt; nhấn giữ để khôi phục cài đặt gốc.

CPU được cấu hình thay đổi tần số 10–80 MHz và cho phép tự động ngủ nhẹ. Dải WS2812 giữ màu sau khi nhận dữ liệu; kênh RMT chỉ bật khi gửi dữ liệu và tắt sau đó. Việc thiết bị có thực sự ngủ còn phụ thuộc Wi-Fi, Bluetooth và các khóa quản lý điện năng khác. Bluetooth trong lúc ghép nối có thể ngăn ngủ nhẹ. Chưa có phép đo dòng điện cho cấu hình này.

Việc thay bảng phân vùng có thể làm mất khả năng truy cập dữ liệu cấu hình cũ. Lưu bản sao flash trước khi chuyển từ firmware có bố cục phân vùng khác.
