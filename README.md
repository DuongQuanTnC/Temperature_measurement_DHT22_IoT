# Temperature_measurement_DHT22_IoT
# ESP32 IoT Sensor Node với ThingsBoard

Dự án này sử dụng ESP32 kết hợp các cảm biến DHT22, LDR, NTC, và nút nhấn để thu thập dữ liệu môi trường, gửi dữ liệu telemetry và trạng thái thiết bị lên ThingsBoard thông qua giao thức MQTT.

Tính năng
- Đọc dữ liệu nhiệt độ & độ ẩm từ DHT22.
- Đọc ánh sáng từ LDR và nhiệt độ từ NTC (qua ADC).
- Lọc dữ liệu cảm biến bằng trung bình cộng
- Gửi dữ liệu telemetry định kỳ lên ThingsBoard (mỗi 5 giây).
- Đọc ngưỡng `Threshold` được chia sẻ từ ThingsBoard (shared attributes).
- Cho phép thay đổi trạng thái thiết bị (`On` / `Off`) bằng nút nhấn và gửi lên ThingsBoard (client attributes).
- Tự động reconnect MQTT khi mất kết nối.
- Hiển thị thông tin cảm biến và trạng thái trên Serial Monitor.

Phần cứng cần thiết
| Linh kiện        | Số lượng | Ghi chú |
|------------------|----------|--------|
| ESP32 DevKit     | 1        | Kết nối WiFi |
| Cảm biến DHT22   | 1        | Nhiệt độ + độ ẩm |
| LDR              | 1        | Đo cường độ ánh sáng |
| Điện trở NTC 10k | 1        | Đo nhiệt độ |
| Điện trở 10k     | 1        | Cho mạch LDR hoặc NTC |
| Nút nhấn         | 1        | Điều khiển trạng thái |
| Breadboard + dây | -        | Kết nối mạch |

Cấu hình mạng và ThingsBoard
1. WiFi  
   Thay SSID và Password WiFi trong code:
   ```cpp
   const char* ssid = "Wokwi-GUEST";
   const char* password = "";
