# Mô hình STEP/DIR và thiết kế hai vòng

Phiên bản `nema17-relative-v1`, tham số trong `shared/robot.json`, ma trận/gain trong
`shared/design.json`. Các phép tính dưới đây là mô hình danh định; chưa đo trên xe.
Nguồn số cơ khí và mâu thuẫn: `01_Khao_sat_nguon.md`. Thuật toán mới không sử dụng
gain, lực hoặc quy ước đơn vị của động cơ DC trong ZIP tham khảo.

## 1. Hệ trục và đại lượng

Trục x hướng trước xe, y hướng trái, z hướng lên; gốc tại trung điểm trục bánh.
Góc pitch θ dương khi thân nghiêng trước (quay quanh +y); yaw ψ dương quay trái
(quanh +z). Bánh quay tiến được quy ước dương ở cả hai bên sau khi áp dụng cờ
đảo DIR riêng. Bánh phải quay nhanh hơn bánh trái tạo yaw dương. Điểm cân bằng
là θ=0, ω=0. Mọi phép tính dùng m, kg, s, rad; chỉ adapter SPEED/TURN dùng cm/s,
deg/s để tương thích web cũ.

| Ký hiệu | Định nghĩa | Đo/ước lượng |
|---|---|---|
| θ, ω=θ̇ | Góc thân, tốc độ pitch | MPU6050: gyro và lọc bù accel; θ là ước lượng, không phải đo góc trực tiếp |
| q | Vận tốc tiếp tuyến bánh tương đối với thân | Đếm xung STEP đã phát, không phải encoder |
| v=q+rω | Vận tốc trục bánh so với đất | Chỉ đúng khi lăn không trượt, không mất bước và đúng r/vi bước |
| x | Vị trí trục bánh | Tích phân v; gốc tự chọn, không đo tuyệt đối |
| ψ̇ | Tốc độ yaw | Gyro z đã hiệu chuẩn, gần thẳng đứng; cần biến đổi trục khi nghiêng lớn |
| d | Nửa chênh lệch vận tốc bánh tương đối | `(q_R−q_L)/2`; ψ̇≈2d/W trong mô hình phẳng |
| u=q̇ | Gia tốc tiếp tuyến tương đối ra lệnh STEP | Đầu vào LQR, đơn vị m/s², không phải lực/mômen/PWM |

Quan hệ quan trọng: góc quay rotor tương đối stator `φ=x/r−θ`. Vì vậy `q=rφ̇`
và `ẋ=q+rθ̇`. Bỏ hạng rθ̇ làm sai vận tốc trong giai đoạn thân lắc. Vận tốc suy ra
từ xung không phát hiện bánh bị kẹt, rotor mất bước hay lốp trượt. Một bộ lọc làm
mượt xung không biến dữ liệu đó thành phép đo thực.

## 2. Cơ học phi tuyến và phản lực động cơ

Giả định: truyền động trực tiếp 1:1, hai bánh cùng bán kính, không trượt, mặt
phẳng ngang, khung cứng; pitch/yaw ghép yếu ở góc và tốc độ yaw nhỏ. m là thân có
motor/pin/bo mạch nhưng loại trừ hai bánh. `Jw=mw*r²/2` là xấp xỉ bánh đĩa đặc.

Để tránh đếm hai lần rotor, lấy `J_locked=m*h²/12` như xấp xỉ quán tính thân
khóa rotor quanh COM, rồi `Jc=J_locked−2Ir`. `Jc` không gồm quán tính spin của hai
rotor. Đây chỉ là quy ước nhất quán cho xấp xỉ hình học, không phải kết quả CAD.
Tấm 125 mm theo chiều x và phân bố pin/motor có thể làm sai xấp xỉ thanh đều đáng
kể; các ca mở rộng quán tính trong kiểm thử không thay thế phép đo.

\[
 M=m+2m_w+\frac{2(J_w+I_r)}{r^2},\qquad J=J_c+ml^2.
\]

T=τL+τR là tổng mômen motor, dương đẩy bánh về trước. Công ảo motor là
`T(dx/r−dθ)` nên phải có phản lực −T lên thân:

\[
 M\ddot x+ml\cos\theta\ddot\theta-ml\sin\theta\omega^2=T/r,
\]
\[
 J\ddot\theta+ml\cos\theta\ddot x-mgl\sin\theta=-T+\tau_{push}.
\]

Đặt `b(θ)=ml cosθ+rM`, `H(θ)=J+2rml cosθ+r²M` và thay `ẍ=u+rθ̈`:

\[
 \dot\theta=\omega,\quad
 \dot\omega=\frac{mgl\sin\theta-b(\theta)u+rml\sin\theta\omega^2+\tau_{push}}{H(\theta)},
 \quad\dot q=u,\quad\dot x=q+r\omega.
\]

`τpush` là mômen ngoại lực tác dụng lên thân, N·m. Ca đẩy dùng 0.025 N·m trong
0.1 s; nếu tay đẩy tại độ cao hF, lực tương đương τpush/hF chỉ đúng với giả định
cách đặt ngoại lực này. Không diễn giải ca đó như một lực tịnh tiến đã đo.

Tổng mômen yêu cầu kiểm tra bằng phương trình tịnh tiến:

\[
 T=r\{M(u+r\ddot\theta)+ml\cos\theta\ddot\theta-ml\sin\theta\omega^2\}.
\]

Phản hồi STEP là giả định bám động học. Không có vòng kín lực/mômen và không
thể cam kết mômen bằng cách đặt tần số. NEMA17 chỉ là cỡ mặt bích; cần mã motor,
dòng cuộn dây, điện cảm, đường cong pull-out tại 12 V và cấu hình A4988 để kiểm
tra bám bước. `0.08 N·m/motor` trong JSON là ngưỡng thử giả định, không là định
mức đã xác minh. Yaw bổ sung yêu cầu `Tdiff≈(2r/W)Jz ψ̈`; cần đo Jz hoặc xác định
từ CAD. Khi có đường cong mômen, phải kiểm tra riêng
`| (T±Tdiff)/2 | <= τ_available(speed)` của từng bánh.

## 3. Tuyến tính hóa, khả điều khiển và quan sát

Với `z=[θ,ω]ᵀ`, `A=[0,1;a,0]`, `B=[0;−b0/H0]`, `C=I₂`, `D=0`,
`a=mgl/H0`, `b0=ml+rM`. Danh định:

\[
 A=\begin{bmatrix}0&1\\54.05536033&0\end{bmatrix},\qquad
 B=\begin{bmatrix}0\\-11.18647582\end{bmatrix}.
\]

`rank([B,AB])=2`. Pitch có thể ổn định hóa; đo góc ước lượng và gyro cho rank
quan sát 2 (ngay cả C=[1,0] lý tưởng cũng cho rank2). Điều này không chứng minh
độ chính xác bộ lọc thực tế. MPU6050 không đo x hay v tuyệt đối.

Mô hình mở rộng `[θ,ω,q,x]` có:

\[
 A_4=\begin{bmatrix}0&1&0&0\\a&0&0&0\\0&0&0&0\\0&r&1&0\end{bmatrix},\quad
 B_4=\begin{bmatrix}0\\-b_0/H_0\\1\\0\end{bmatrix}.
\]

Chỉ dùng IMU, q và x không quan sát được. Xung STEP là đầu vào đã ra lệnh, nên
không cung cấp phép đo độc lập về mất bước. Không thiết kế observer rồi tuyên
bố khôi phục được vận tốc thật trong điều kiện trượt. Chế độ LQR riêng chỉ làm
ổn định phân hệ pitch; hai mode tịnh tiến không được ổn định về gốc. Muốn đứng
yên hơn dùng PID vận tốc với đặt 0; muốn giữ vị trí thật cần encoder/cảm biến
ngoài cùng một vòng vị trí được thiết kế thêm.

Rời rạc hóa chính xác ZOH tại Ts=0.005 s:
`expm([A B; 0 0 0]*Ts)=[Ad Bd;0 0 1]`. Script xuất đủ Ad/Bd/P; MATLAB dùng
`c2d`/`dlqr`, Python dùng `expm`/DARE để đối chiếu độc lập. Không thay gain CARE
liên tục vào firmware rồi gọi đó là LQR rời rạc.

## 4. LQR và giao diện PID

Tối thiểu hóa `Σ(zᵀQz+u²R)`; `u=−Kz`. Bộ cơ sở Q=diag(25,1), R=2. Bộ ứng viên
tinh chỉnh Q=diag(400,4), R=1:

\[
 K=[-23.7933968347616,\;-2.75423353980527],\qquad
 eig(A_d-B_dK)=[0.9499955693,\;0.8939430051].
\]

K có đơn vị tương ứng m/(s²·rad) và m/(s·rad). Dấu âm là đúng với B âm:
nghiêng trước θ>0, ω=0 ⇒ u>0, bánh đuổi theo hướng đổ. Test xác minh dấu riêng.
Gain thực tế phải đọc từ file xuất đầy đủ độ chính xác, không gõ từ bảng làm tròn.

PID vận tốc ở 50 Hz cho `a_ref`; PID yaw cho `alpha_ref`. LQR không sử dụng sai
số vận tốc. Không đặt một LQR ba trạng thái có v bên trong rồi đồng thời ép PID
ngoài sửa cùng sai số v. Tham chiếu pitch ở gia tốc không đổi thỏa:

\[
 mgl\sin\theta_{ref}=(ml\cos\theta_{ref}+rM)a_{ref},
\]
\[
 \theta_{ref}=\operatorname{atan2}(a_{ref},g)+
 \arcsin\frac{rMa_{ref}}{ml\sqrt{g^2+a_{ref}^2}}.
\]

Gần thẳng đứng `θref≈0.20694*a_ref`; chỉ dùng `atan(a/g)` sẽ bỏ phản lực motor.
Điều khiển `u=−K0 θ−K1 ω + K0 θref+a_ref`. Tách phần cân bằng `ub=−Kz` và phần
chuyển động `um=K0 θref+a_ref` để xử lý giới hạn. Do đặc tính ngược chiều ban đầu,
xe có thể lùi nhẹ trước khi tăng tốc tiến. Không đảo dấu PID để loại quá độ này.

PID dùng đạo hàm trên phép đo (không đá đạo hàm do lệnh đặt), lọc bậc một với
τd=0.08 s, tích phân có chặn ±1 đơn vị sai số·s, chỉ tích lũy nếu chưa bão hòa
hoặc đang thoát bão hòa. Bão hòa tầng chấp hành đóng băng tích phân vòng ngoài
ở lần cập nhật kế tiếp. a_ref lọc τ=0.12 s, chặn ±0.35 m/s²; yaw acceleration
chặn ±2 rad/s². Tham chiếu v giới hạn ±0.2 m/s, ramp 0.12 m/s²; yaw ±0.8 rad/s,
ramp 0.6 rad/s². Gain PID trong JSON là ứng viên đã kiểm thử mô phỏng, chưa tune
trên xe thật. Với tham chiếu biến thiên nhanh, cân bằng tĩnh trên chỉ xấp xỉ;
lọc và giới hạn ramp là một phần bắt buộc của cấu hình.

## 5. Bộ trộn có ưu tiên và STEP

Tích phân u thành q, yaw thành d; `qL=q−d`, `qR=q+d`. Quy đổi:
`fL,R=qL,R*3200/(2πr)` Hz, DIR lấy dấu, cờ đảo chiều xử lý ở HAL. Tần số có dấu
trong mô hình, độ lớn trong bộ phát. Không thêm rω vào STEP vì STEP đã là góc
rotor tương đối stator; chỉ thêm hạng đó khi suy v so với đất.

Giới hạn mỗi bánh |q|<=0.6 m/s, |Δq|/Ts<=3 m/s²; ở r=0.033, fmax sử dụng
khoảng 9260 Hz, thấp hơn giới hạn phần mềm 15000 Hz. Khi gần biên tốc độ, không
được chỉ cắt độc lập hai bánh sau trộn vì sẽ phá phần cân bằng và giới hạn gia tốc.
Thuật toán chọn q_new trước trong miền khả thi tổng hợp từ hai bánh, rồi chọn
d_new gần yêu cầu yaw nhất còn hợp lệ:

```
Llo=max(-V,L-A*Ts); Lhi=min(V,L+A*Ts)
Rlo=max(-V,R-A*Ts); Rhi=min(V,R+A*Ts)
ub_clip=clip(ub,-A,A)
u=ub_clip+clip(um,-A-ub_clip,A-ub_clip)
q_new=clip(q+u*Ts,(Llo+Rlo)/2,(Lhi+Rhi)/2)
d_new=clip(d+W/2*alpha_ref*Ts,
           max(q_new-Lhi,Rlo-q_new), min(q_new-Llo,Rhi-q_new))
```

Test biên q=0.58,d=0.02 phát hiện và sửa lỗi cũ có thể tạo gia tốc một bánh
6 m/s² dù đặt A=3. Sau sửa miền khả thi, mỗi bánh tuân cả tốc độ lẫn gia tốc.
Ưu tiên không có nghĩa luôn cứu được xe khi đã hết tốc độ/mômen; nếu bão hòa kéo
dài hoặc vượt góc ngã, firmware chuyển fault và cần kích hoạt lại có chủ đích.

## 6. Phạm vi mô phỏng và chu kỳ

200 Hz được chọn so với cực kín tương đương khoảng 10.3 và 22.4 rad/s; 5 ms
nhỏ hơn hằng số nhanh khoảng9 lần. PID50Hz cập nhật chậm hơn, tham chiếu còn
được lọc để giảm tương tác. Đây là lựa chọn thiết kế, không chứng minh ESP32-S3
đúng hạn. Mô phỏng có ZOH, tốc độ/gia tốc giới hạn, phản lực motor và các ca trễ
0/5/10 ms, gain bám0.8/1/1.2. Chưa bao trùm toàn bộ phi tuyến mất bước, dao động
rotor, cộng hưởng vi bước, nhiệt, ma sát Coulomb hoặc sụt áp nguồn.

MPU6050 DLPF_CFG=2 trong HAL có độ trễ khoảng2.8ms gyro/3.0ms accel theo register map;
phải cộng với I2C, lịch task và tuổi snapshot khi đo margin. Nguồn:
[TDK register map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf).
Không gọi jitter mục tiêu0.5ms hoặc thời gian tick1ms là đã đo nếu chưa có trace.

Mọi kết quả số, đồ thị và verdict nằm ở `06_KiemThu/results`, hai thư mục
MATLAB/Simulink và báo cáo tổng hợp. Một ca hồi phục15° trong mô phỏng không
cho phép START xe thật ở15°: arm gate vẫn4°, và nguồn số cơ khí chưa xác nhận.
