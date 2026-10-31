# TVT RainMaker (Flutter)

Ứng dụng Flutter đa nền tảng thay cho `app_android`. Phần dùng chung viết bằng
Dart: đăng nhập RainMaker, đọc node Cloud và điều khiển `Light` (Power,
Brightness, Hue, Saturation). Provisioning BLE Security 1 dùng SDK Espressif
native, vì vậy chỉ có trên Android và iOS. Web/desktop vẫn đăng nhập bằng email
và điều khiển những ESP đã được thêm vào RainMaker Cloud.

## Cấu hình đã giữ lại

- RainMaker Cloud: `https://api.rainmaker.espressif.com`
- OAuth client/redirect URI cũ và Android package `com.espressif.rainmaker`
- BLE prefix `PROV_`, Security 1, endpoint `cloud_user_assoc`
- Logo RainMaker tại `assets/images/tvt_rainmaker_logo.png`

Không đổi package id nếu chưa đăng ký OAuth client mới, nếu không đăng nhập
Google/GitHub có thể không quay về app.

## Chạy trên Windows

```powershell
cd D:\PBL5\book-esp32c3-iot-projects-main\phone_app\app_dnt
D:\software\flutter\bin\flutter.bat pub get
D:\software\flutter\bin\flutter.bat run -d chrome
```

## Chạy/cài Android

1. Bật **Developer Mode** của Windows (Settings → Privacy & security → For developers → Developer Mode). Flutter cần quyền symbolic-link cho plugin BLE.
2. Cài **JDK 17 đến 25**. Máy hiện có `D:\AndroidStudio\jre` là JDK 11 nên không dùng được với Gradle 9 của Flutter.
3. Đặt JDK mới cho Flutter, ví dụ:

```powershell
D:\software\flutter\bin\flutter.bat config --jdk-dir "D:\software\jdk-17"
D:\software\flutter\bin\flutter.bat doctor --android-licenses
D:\software\flutter\bin\flutter.bat run
```

Điện thoại cần bật USB debugging. Khi chạy lần đầu, chấp nhận quyền Bluetooth
và Vị trí để quét thiết bị `PROV_...`.

## iOS, macOS, Linux và Windows

- iOS/macOS: mở folder này trên máy macOS có Xcode, rồi chạy `flutter run`.
- Windows desktop: cài Visual Studio với workload **Desktop development with C++**.
- Linux: build trên máy Linux có toolchain CMake/GTK.

`flutter analyze` và `flutter test` đã chạy thành công. Bản web release đã
được tạo trong `build/web`.
