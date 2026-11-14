# Ledger bằng chứng — đang hoàn thiện

Các lần làm việc bị ngắt đã giữ lại mã và dữ liệu; chưa coi file tồn tại là PASS.

| Cấp | Trạng thái có bằng chứng |
|---|---|
| Khảo sát nguồn | Đã đọc Excel, mã LQR, web/firmware cũ và XML SLX; nguồn SHA256 trong manifest |
| Python | Đã chạy mô phỏng phi tuyến và4 test số học/bộ trộn; log/results được tái sinh khi thuật toán đổi |
| MATLAB/Simulink | Có giấy phép; đang hoàn tất chạy hai mô hình và đối chiếu vector |
| C++ host | Đang build/test, chưa tuyên bố PASS |
| Web tích hợp | Đang hoàn tất test, chưa tuyên bố PASS |
| Build ESP32-S3 | Chưa có bằng chứng build hoàn chỉnh tại thời điểm ledger này |
| Upload/xe thật/jitter | NOT RUN; chưa chốt module và sơ đồ dây |
| Video31:58 | Không truy xuất được; không gán kết luận kỹ thuật cho video |

Các báo cáo03/04/05 và log của từng công đoạn sẽ ghi kết quả quan sát sau khi
chạy hoàn tất. Giữ trạng thái chưa xác minh nếu bước chạy không thành công.
