# Phân tích log thử thực tế 12/11/2026

Nguồn: robot_1794479194395.json và video 1794479137261_938716928013433771_938716928013433771.mp4 do người dùng cung cấp. Không tự chạy motor, reset hoặc nạp.

200 mẫu, t=2023.013..2031.162 s: 178 RUN/NONE/motion=0; 22 Fault/SPEED_RAIL/motion=0. st trong JSON đã qua adapter web (4 là lỗi khác FALL).

2029.811: theta=0.10475 rad, omega=0.71286 rad/s, left=right=0.12208 m/s, u=3 m/s².
2029.975: left=right=0.6 m/s, theta=0.21639 rad.
2030.262: vẫn 0.6 m/s, theta=0.13443 rad, omega=-1.03673 rad/s.
2030.302: fault=SPEED_RAIL, left=right=u=0; theta=0.09210 rad (~5.28°), omega=-1.12732 rad/s.
Đoạn rail thấy trong telemetry dài ít nhất 0.287 s, phù hợp gate nội bộ 300 ms (telemetry ~41 ms/mẫu). Không tăng ngưỡng/giới hạn để che lỗi.
Trong RUN, góc trong khoảng -9.10..14.49°, không có FALL trong log này. Overruns giữ nguyên 13; tick_max_us=35209 là cực đại tích lũy, không chứng minh deadline trễ ngay lúc fault. PID motion=0 toàn bộ log.

Video dài ~123.47 s, đã trích 16 frame bằng Edge video decoder có hỗ trợ HTTP Range. Nhiều đoạn xe được giữ bằng tay/bánh lơ lửng trên cuốn sách; một số đoạn đặt xuống và giữ thân. Không đồng bộ được thời gian video với telemetry. Khung hình không đủ xác nhận dấu quay bánh so với mặt trước thân hoặc tốc độ rotor. Motor đổi chiều theo góc chưa chứng minh phản hồi đúng dấu trên sàn.

Kết luận: nguyên nhân ngắt trong log này là giữ tốc độ lệnh ở rail 0.6 m/s, không phải ngã vượt góc. Nguyên nhân tích lũy tốc độ trên sàn chưa xác định. Khi bánh không bám sàn, phản hồi cơ học giữ thân của mô hình không tồn tại, LQR tích phân gia tốc có thể đưa tốc độ tới rail. LQR chỉ góc không giữ vận tốc 0. Chưa có bằng chứng chọn thay đổi gain, zero, đảo chiều hoặc giới hạn.
