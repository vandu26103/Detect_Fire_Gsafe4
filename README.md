# Gsafe4 - Tủ Trung Tâm Phát Hiện Cháy

Gsafe4 là hệ thống tủ trung tâm phát hiện cháy thông minh, hỗ trợ kết nối cảm biến và truyền dữ liệu báo cháy qua WiFi hoặc mạng 4G. Dự án hỗ trợ kết nối nhiều tủ, cho phép các tủ không có khả năng truyền thông vẫn có thể gửi dữ liệu đến Server thông qua tủ chính.

## 🔥 Tính Năng Chính

- **Phát hiện và cảnh báo cháy**:
  - Đọc trạng thái từ các cảm biến (khói, nhiệt, lửa...).
  - Khi phát hiện cháy: gửi cảnh báo ngay lập tức lên Server, đồng thời kích hoạt còi và đèn cảnh báo.

- **Giao tiếp Server**:
  - Gửi dữ liệu qua giao thức **MQTT**.
  - **Ưu tiên WiFi**: khi WiFi mất kết nối, hệ thống sẽ tự động chuyển sang sử dụng **4G**.

- **Kết nối tủ phụ**:
  - Hỗ trợ kết nối các tủ khác **không có khả năng truyền thông**.
  - Gsafe4 sẽ thu thập và gửi dữ liệu thay cho các tủ phụ.

## 📡 Giao Thức & Kết Nối
- **MQTT Protocol** để truyền dữ liệu tới Server.
- **WiFi/4G fallback**:
  - Ưu tiên sử dụng WiFi.
  - Tự động chuyển sang 4G nếu WiFi mất kết nối.
- **RF** để đóng mở còi báo động.

[Sản phẩm](https://drive.google.com/drive/folders/13iAudCnwl4yyChVXJk1LQSURCTpbq7Em?usp=sharing)