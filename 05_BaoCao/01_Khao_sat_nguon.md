# Khảo sát nguồn và truy vết thông số

Ngày khảo sát: 10/11/2026. Phạm vi: đọc nguồn đã giải nén, đọc từng ô Excel đã trích xuất và đọc XML bên trong SLX; không nạp firmware, không chạy xe và không coi chú thích trong mã là kết quả đo phần cứng.

## 1. Kết luận sử dụng nguồn

Có ba nguồn với vai trò khác nhau: tài liệu LQR cho động cơ DC có encoder; bộ tích hợp ESP32-S3 dùng động cơ bước và web; bảng thông số xe NEMA17. Không thể ghép trực tiếp gain hoặc đầu vào của ba nguồn. Bảng `Thong_so` cho một bộ số SI đủ để dựng **mô hình tạm**, nhưng phiếu đo cơ khí còn trống cột số đo thực tế và có mâu thuẫn. Chưa có cơ sở gọi bộ số đó là thông số đã xác nhận của xe thật.

Bộ danh định tạm dùng để tái lập mô phỏng là `mb=1.141 kg`, `mw=0.033 kg/bánh`, `r=0.033 m`, `l=0.035 m`, `h=0.14 m`, `Ir=3.6e-6 kg·m²/rotor`, `W=0.228 m`, `200 full-step/vòng`, `microstep=16`, truyền động trực tiếp giả định tỷ số 1:1. Giữ trạng thái **chưa kiểm chứng**, không tự động nâng trạng thái khi số liệu trùng firmware.

## 2. Danh mục và khả năng tái lập

`00_Nguon/source_manifest.json` ghi nguồn và SHA-256:

| Nguồn gốc | Dung lượng | SHA-256 | Nội dung sử dụng |
|---|---:|---|---|
| `C:\Users\USER\Downloads\Bang_do_thong_so_thiet_ke_lai_xe_NEMA17.xlsx` | 27,588 byte | `25a463d5afdbfa88b7cf762604d8d542b3ecd576fd61638c4aecec2e760eb671` | Bảng tham số và phiếu đo; bản trích ô: `00_Nguon/workbook_cells.json` |
| `C:\Users\USER\Downloads\Two--wheeled-self-balancing-robot-main.zip` | 21,379,458 byte | `7db45aa5d3980fcc31ae6caf95a6d4a4c8fb7ed098d74783915985221fc5b96d` | `00_Nguon/lqr_reference/LQR` |
| `D:\TLHT\HTDKTD_Xe_Can_Banh.zip` | 313,279,185 byte | `7247c3267eb65dfe8118c266a5193f66242bbd7211d9b47aa40b3d95dddef7b1` | `00_Nguon/integration_reference` |

Các giá trị hash trên được đọc từ manifest; đây không phải tuyên bố đã kiểm lại toàn bộ archive trong lần khảo sát này. Script `tools/source_parameter_audit.py` chỉ đọc dữ liệu nguồn để tái hiện những ô then chốt, các thông số dẫn xuất và khác biệt SLX/MATLAB.

Video tham chiếu: <https://youtube.com/watch?v=eeaqPCHMAXg&t=1918s>. Lần truy xuất video trong phiên làm việc không thành công. `1918 s = 31 phút 58 giây` chỉ là phép đổi tham số URL; chưa xác minh hình ảnh, lời nói, sơ đồ, thông số hoặc diễn biến tại mốc đó. Báo cáo không gán kết luận kỹ thuật nào cho video.

## 3. Bảng truy vết Excel sang SI

Quy tắc đọc: `01_Danh_sach_can_do!E4` là **Tham khảo ban đầu**, `F4` là **Đơn vị**, `G4` là **Số đo thực tế**, `K4` là **Trạng thái**. Nhiều ô F chứa cả con số hoặc ghi chú thay vì chỉ đơn vị. Vì thế không được coi một chuỗi có `mm` trong cột F là số đo được xác nhận. `00_Huong_dan!A2` nói rõ số tham khảo là giả định từ trao đổi/ảnh; `B14` yêu cầu phân biệt Chưa đo / Đã đo / Cần đo lại / Đã xác nhận.

| Đại lượng | Sheet, ô và giá trị gốc | Giá trị SI dùng tạm | Trạng thái và ảnh hưởng |
|---|---|---:|---|
| Khối lượng thân `mb` | `Thong_so!C2=1.141`, `D2=kg`, `E2=Nên cân lại cả xe có pin`; `01_Danh_sach_can_do!G56` trống, `K56=Chưa đo` cho toàn xe | 1.141 kg | Có số nhưng cần cân lại; phải xác định thân có gồm pin, motor, khung và bo mạch chưa. Nếu 1.141 đã là toàn xe, cộng hai bánh sẽ tính trùng. Chi phối mômen trọng lực và quán tính. |
| Khối lượng mỗi bánh `mw` | `Thong_so!C3=0.033`, `D3=kg`, `E3=Kiểm tra xem là 1 bánh hay 2 bánh` | 0.033 kg/bánh | Giả định đây là một bánh theo nhãn A3, chưa xác nhận. Sai một/hai bánh làm sai khối lượng tịnh tiến và quán tính bánh. |
| Bán kính `r` | `Thong_so!C4=0.033`, `D4=m`; phiếu `E5=Ø80 (đã trao đổi)`, `F5=66mm`, `G5` trống, `K5=Chưa đo` | 0.033 m | Bộ SI tương ứng đường kính 66 mm; tham khảo 80 mm tương ứng r=0.040 m. Giữ cả hai ứng viên, không chọn bằng cách xóa mâu thuẫn. Ảnh hưởng xung/mét, lực/mômen, vận tốc và quán tính. |
| COM cao hơn trục `l` | `Thong_so!C5=0.035`, `D5=m`, `E5=Cần đo kiểm chứng (xem mục B)`; phiếu `G57` trống, `K57=Chưa đo` | 0.035 m | Chưa đo. Đây là khoảng cách từ trục bánh, không phải từ mặt đất. Ảnh hưởng trực tiếp cực không ổn định, gain và trim cân bằng. |
| Chiều cao thân `h` | `Thong_so!C6=0.14`, `D6=m`; phiếu `G34` trống, `K34=Chưa đo` | 0.14 m | Số nhập tạm; chiều cao bao không chứng minh phân bố khối lượng đồng đều. Chỉ dùng để xấp xỉ quán tính khi chưa có CAD/mass properties. |
| Quán tính rotor `Ir` | `Thong_so!C7=3.6e-6`, `D7=kg·m²`, `E7=Khớp với datasheet NEMA17` | 3.6e-6 kg·m²/rotor | Nguồn chỉ có lời ghi chú, chưa chỉ ra hãng, mã motor, số trang datasheet hay trục quy chiếu. NEMA17 là cỡ mặt bích, không đủ xác định rotor. Giữ dạng giả định kỹ thuật. |
| Track `W` | `Thong_so!C8=0.228`, `D8=m`; phiếu `E32=160 (đã trao đổi trước đó)`, `F32=20.5mm`, `G32` trống, `K32=Chưa đo` | 0.228 m | Ba con số không đồng nhất: 228 mm, tham khảo 160, và chuỗi 20.5mm. Không đổi 20.5mm thành 20.5cm. Chi phối vi sai quay; không tham gia trực tiếp LQR dọc ba trạng thái của web cũ. |
| Full step/vòng | `Thong_so!C9=200`, `D9=step`, `E9=1.8°` | 200 full step/vòng | Thông số danh định, cần xác minh mã motor/đo vòng quay. Không đồng nhất full step với xung STEP ở vi bước. |
| Vi bước | `Thong_so!C10=16`, `E10=Phải khớp jumper MS1..MS3` | 16 | Cần quan sát jumper/thanh ghi và driver cụ thể. `03_Linh_kien!D7=A4988 (nếu đang dùng)` chưa xác nhận driver thật. |
| Đảo chiều trái/phải | `Thong_so!C11=0 / 1`, `E11=Cần kiểm tra lại bằng thử bánh lơ lửng` | cờ 0 / 1 | Chỉ cấu hình dự kiến; phải thử từng bánh trên giá đỡ. Dấu sai gây phản hồi dương. |
| Khối lượng pin | Phiếu `F44=12.5/2`, `G44` trống, `K44=Chưa đo` | Không quy đổi | Không rõ đại lượng và đơn vị; không được tính 6.25 g/kg hay suy đây là phép cân. Không dùng để tính COM. |
| Vị trí pin | Phiếu `F43=mm`, `G43` trống, `K43=Chưa đo` | Chưa có | Không đủ tính COM theo tổng khối lượng thành phần. |

Các số hình học phụ cũng chỉ là ứng viên: tấm dưới `F26/F27/F28=125mm/70mm/2mm`, tấm trên `F29/F30/F31=125mm/70mm/2mm`, trụ `F39=42mm`, pin `F42=75x60x20mm`, motor `F11=42mm`, `F12=34mm`, `F13=5mm`. G tương ứng đều trống. Không dùng các kích thước này như bản vẽ chế tạo đã duyệt hay dùng chúng để suy ra chiều cao COM.

Phiếu `C08` tại hàng 33 còn có `F33=18.8mm` cho khoảng hai mặt trong bánh. Kết hợp `F32=20.5mm` cho tâm bánh và `F6=25mm` cho bề rộng bánh tạo quan hệ hình học đáng ngờ: hai bánh bằng nhau thường cho `track ≈ khoảng hai mặt trong + bề rộng bánh`. Cả cách hiểu literal mm lẫn tự nhân 10 đều không tự giải quyết được các con số. Cần đo lại đúng ba đại lượng; không tự sửa bảng.

## 4. Đại lượng dẫn xuất, trục quán tính và phạm vi giả định

Với bộ danh định tạm và truyền động trực tiếp:

| Đại lượng | Công thức | Kết quả |
|---|---|---:|
| Tổng khối lượng nếu `mb` loại trừ hai bánh | `mb+2mw` | 1.207 kg |
| Xung/vòng bánh | `200×16` | 3200 xung/vòng |
| Góc/xung | `2π/3200` | 0.0019634954 rad/xung |
| Quãng đường/xung | `2πr/3200` | 6.47953485e-5 m/xung |
| Xung/mét | `3200/(2πr)` | 15433.2066 xung/m |
| Quán tính một bánh dạng đĩa đặc | `Iw=0.5mw r²` | 1.79685e-5 kg·m² |
| Quán tính thân quanh COM, xấp xỉ thanh đều | `I_COM=mb h²/12` | 0.0018636333 kg·m² |
| Quán tính thân quanh trục bánh | `J_axle=I_COM+mb l²` | 0.0032613583 kg·m² |

`Thong_so!B20/C20` ghi `1.80e-5 kg·m²`, công thức `½·mw·r²`: đây là ước lượng, không phải đo. `Thong_so!B21/C21` ghi `≈3.3e-3 kg·m²`, công thức `mb·(h²/12+l²)`: đây là quán tính **quanh trục bánh**. Khi phương trình dùng `I_COM+mb l²`, phải đưa vào 0.0018636333, không đưa 0.0032613583 rồi cộng thêm `mb l²` lần nữa. Tên `Jψ` ở các nguồn khác nhau không đảm bảo cùng trục quy chiếu.

Mô hình thanh đều bỏ qua chiều sâu thân, phân bố pin/motor và khối lượng tập trung. Bánh thật có cao su và hub nên hệ số `Iw/(mw r²)` có thể khác 0.5. Rotor cần phân biệt quay tương đối với thân và chuyển động của stator; không mặc nhiên sao chép quán tính quy đổi của hộp số DC sang NEMA17.

Khoảng khảo sát đề nghị dưới đây là **kịch bản kỹ thuật để đánh giá nhạy**, không phải khoảng tin cậy thống kê hay số đo:

| Tham số | Danh định | Khoảng/kịch bản khảo sát | Lý do |
|---|---:|---|---|
| `mb` | 1.141 kg | 0.9–1.4 kg; thêm trường hợp 1.141 là tổng xe và thân còn 1.075 kg | Chưa cân lại và chưa rõ cách phân nhóm |
| `mw` mỗi bánh | 0.033 kg | 0.0165, 0.033, 0.050 kg | Kiểm tra giả thuyết 0.033 là cả cặp và mức tăng quán tính |
| `r` | 0.033 m | 0.032–0.040 m, bắt buộc ca 0.040 m | Chứa hai ứng viên đường kính 66/80 mm, thêm dung sai lăn |
| `l` | 0.035 m | 0.02–0.07 m | COM chưa đo; không thay bằng nửa chiều cao thân |
| `I_COM` | 0.0018636333 kg·m² | 0.5–2 lần danh định | Phân bố khối lượng chưa biết; tránh chỉ quét chiều cao bao |
| `Ir` | 3.6e-6 kg·m² | 0.5–2 lần danh định | Chưa có datasheet định danh |
| `W` | 0.228 m | 0.160 và 0.228 m; 0.0205 m chỉ là dữ liệu mâu thuẫn phải loại qua đo | 160 và 228 là hai ứng viên khác nhau; không tạo 0.205 m bằng suy đoán đơn vị |
| Microstep | 16 | kiểm thử cấu hình 8/16 như lỗi cấu hình, không như sai số nhỏ | Sai hệ số 2 làm toàn bộ odometry/lệnh vận tốc sai hệ số 2 |

Không tuyên bố bộ điều khiển ổn định trên toàn bộ các khoảng trên trước khi chạy mô phỏng với gain cố định, bão hòa, độ trễ và mô hình actuator tương ứng. Nếu mô phỏng quét h thay vì I, phải công bố công thức chuyển đổi và tránh quét cả hai như hai tham số độc lập khi vẫn ép cùng công thức.

## 5. Đối chiếu LQR gốc: MATLAB, Simulink và Arduino

### 5.1 Mô hình và tham số

Nguồn `lqr_reference/LQR/matlab/CacFunctionTrongSimulink_LQR.m` xây dựng phương trình có `vl`, `vr`, điện trở `Rm`, hằng số `Kb`, `Kt`, tỷ số truyền `n`; đây là mô hình động cơ DC điều khiển điện áp, không phải mô hình tần số STEP/DIR. Khối lượng/bán kính gốc: `m=.033`, `M=.818`, `R=.0325`, `W=.15`, `D=.07`, `H=.095`, `L=.06`; `Jm=.01`, `n=30`, `fw=.18`, `fm=.002`, `Rm=50`, `Kb=.327`, `Kt=.5`, `g=9.81`.

`fw` và `fm` đi vào hạng tỷ lệ vận tốc nên phải xét đơn vị damping trong phương trình; nhãn “hệ số ma sát” không đủ để coi chúng là hệ số ma sát Coulomb không thứ nguyên. `2n²Jm=18 kg·m²` nếu diễn giải Jm theo SI là rất lớn so với robot nhỏ; nguồn không có chứng cứ định danh/đo để chuyển giá trị đó sang xe NEMA17. `Jψ=2ML²/3` và `Jφ=M(W²+D²)/12` là công thức mô hình cũ, không phải kết quả đo xe mới.

Script dùng `lqr(A,B,Q,R__)`, `R__=I₂`, `Q=diag(1000000,1000,10000,100,100000,0.01)` và `lqe` liên tục. `T=.01` có mặt nhưng lời gọi `lqr` không tự trở thành LQR rời rạc. Gain này phụ thuộc đơn vị, thứ tự trạng thái và mô hình đầu vào.

### 5.2 Thứ tự trạng thái, đơn vị và dấu

| Thành phần | MATLAB/SLX gốc | `Code/LQR/LQR.ino` | Hệ quả |
|---|---|---|---|
| Vector phản hồi | `[θ, θdot, ψ, ψdot, φ, φdot]` tương ứng `[x1,x2,x4,x5,x7,x8]` | `[pitch,Gxrate,theta,thetadot,phi,phidot]` tại dòng 152–153 | Hai cặp đầu hoán đổi ý nghĩa |
| `θ` | Góc lăn trung bình bánh theo phương trình có `R²` và `MRL` | `0.55*(leftencoder+righencoder)` tại dòng 144 | Mã không cung cấp số xung/vòng để chứng minh hệ số thành rad; không gọi đây là mét |
| `ψ` / pitch | Góc nghiêng thân; `sin/cos` dùng rad | `atan2(AcX,AcZ)*RAD_TO_DEG`, gyro `/131.0`, `pitch -= (1+offset)` | Pitch độ, gyro độ/s; offset mặc định .5 làm trừ 1.5° |
| `φ` / yaw | Góc quay hướng; đầu vào vi sai `vr-vl` trong phương trình | `0.25*(leftencoder-righencoder)` tại dòng 145 | Hệ số chưa gắn với track/radius/encoder; dấu yaw phải kiểm chứng |
| Gain | Ma trận 2×6 để ra `[vl,vr]` | Một mảng `{26,1.3,.2,.25,2,.5}` dùng giống nhau cho cả u1/u2 | Không phải chép nguyên kết quả MATLAB; không có hai hàng yaw đối dấu |
| Đầu ra | Điện áp hai động cơ theo mô hình DC | `motorL=-u1+manual_turn`, `motorR=-u2-manual_turn`, thêm `min_PWM=7`, chặn 255 | PWM không đồng nhất điện áp lý tưởng; cần nguồn, dead zone và driver |
| Chu kỳ | SLX 10 ms | Điều kiện `micros()-last_time>5000`, dt đo thực | Gần 5 ms nhưng phụ thuộc loop/I2C; không phải fixed step 10 ms |
| Đo bánh | Mô hình trạng thái liên tục | ISR encoder cạnh RISING với quy ước trái/phải khác nhau | Dấu vật lý phụ thuộc lắp và đấu dây |

Đặc biệt, `x1` và `x2` trong ino giống nhau và dùng cùng K, nên khi `manual_turn=0` thì u1=u2. Các hạng yaw trong gain tác động common mode chứ không tạo hiệu điện áp điều khiển yaw như ma trận B gốc. Lệnh tiến/lùi còn sửa trực tiếp biến encoder; cần thiết kế lại tham chiếu thay vì coi đó là đo vị trí thuần túy.

Hai biến `leftencoder_1`, `righencoder_1` được dùng ở dòng 135, 261–267 nhưng không tìm thấy khai báo trong hai file `.ino` được cung cấp. Thư viện `Kalman.h` cũng không nằm trong thư mục nguồn gốc đã trích. Đây là thiếu hụt khả năng build tái lập, chưa phải kết quả của một lần biên dịch thực tế. Không dùng việc có file ino làm bằng chứng firmware gốc đã chạy được nguyên trạng.

### 5.3 Khác biệt nội bộ SLX

Đọc `Mophong.slx` như ZIP xác nhận metadata `R2024a`, sửa cuối `2026-01-17T03:23:23Z`, revision `1.23`; `simulink/configSet0.xml` ghi `SolverName=ode3`, `FixedStep=.01`, `StartTime=0.0`, `StopTime=10`. Model có các khối `Gain=-K`, `A`, `B`, `C`, `Lk`, nguồn noise, saturation và MATLAB Function `Xe_2_Banh_Tu_Can_Bang`.

Có hai sai khác cần xử lý trước khi nói tái lập đúng mô hình gốc:

1. `simulink/stateflow/chart_16.xml` hard-code `Rm=40`; script `.m` dùng `Rm=50`. Chạy script tính gain không tự sửa điện trở trong plant hard-code.
2. Mẫu số `phi_dd` trong chart có `Jm*W² + Jm*W²*n²`; biểu thức yaw trong `.m` có `Jw*W² + Jm*W²*n²`. Vì Jm và Jw khác xa nhau, không coi đây là hai mô hình tương đương. Cần ghi rõ sửa theo phương trình nào nếu xây lại plant.

Khảo sát này chỉ đọc XML; chưa chạy mô hình gốc để xác nhận kết nối, trạng thái manual switch, tín hiệu hoặc đồ thị đầu ra.

## 6. Bộ tích hợp cũ: web và ESP32-S3

### 6.1 Điểm đang khớp trong nguồn

`4_Web/lqr.js` dùng `z=[theta,theta_dot,x_dot]`, đầu vào ảo `F` đơn vị N, quy ước θ dương nghiêng trước; mặc định cùng `mb,mw,r,l,h,Ir` với Excel `Thong_so`. Nó dựng:

```text
Iw = 0.5 mw r²
M = 2mw + 2(Iw+Ir)/r²
I = mb h²/12; J = I + mb l²; Mt = M + mb
D = J - (mb l)²/Mt
a = mb g l/D; b = -(mb l/Mt)/D
c = -(mb l/Mt)a; d = (1-mb l b)/Mt
A = [0 1 0; a 0 0; c 0 0]; B = [0;b;d]
```

Tính lại đại số từ bộ số nguồn cho `a=197.65542215`, `b=-16.16250126`, `A41=c=-6.3318594757`, `B4=d=1.31993760295`, `FrefGain=-a/b=12.2292595041`. Các hệ số A41/B4/Fref này khớp hằng mặc định firmware đến số chữ số đã ghi. `Config.h` ghi `K=[-32.36122708,-2.51018007,-3.16227766]`, `Q=diag(100,1,10)`, `R=1`; khảo sát nguồn không tự coi chú thích “đã kiểm chứng” là log kiểm thử mới.

Firmware `Controller.cpp` thực hiện PID vị trí ngoài → `thetaRef`; LQR trong `F=FrefGain*thetaRef-K*[theta-thetaRef,thetaDot,vEst]`; sau đó `acc=A41*theta+B4*F`, tích phân thành `vCmd`, giới hạn gia tốc/vận tốc và phát xung. Điều khiển chạy danh định 200 Hz, `A_MAX=6 m/s²`, `V_MAX=.8 m/s`, vận tốc ước lượng lọc 40 ms. `StepperDrive.cpp` đếm xung đã phát để tạo vị trí trung bình hai bánh; không có encoder đo chuyển động thật. Góc và gyro của nguồn ESP32 dùng rad/rad/s, khác nguồn DC ino dùng độ.

Web gửi `$K,k_theta,k_thetaDot,k_velocity,A41,B4,FrefGain`; parser firmware đọc đúng sáu số theo thứ tự này. Đây là ba gain + ba hệ số mô hình, không phải sáu gain LQR sáu trạng thái. Firmware kiểm tra dấu A41<0, B4>0, Fref>0 và giới hạn. Không dùng cú pháp này để gửi ma trận K mới có kích thước khác.

### 6.2 Giới hạn và lỗi giao diện hợp đồng

- `lqr.js` là mô hình lực trên cart với actuator lý tưởng; NEMA17/A4988 nhận xung/vận tốc, không nhận lực N. Ánh xạ tích phân gia tốc thành tần số bước là một giả định cần đánh giá bằng model actuator và kiểm thử, không chứng minh lực thực bằng F. Mômen phản lực lên thân, mômen theo tốc độ, dòng driver, mất bước, độ trễ và trượt chưa được mô tả đầy đủ.
- Không có yaw state trong LQR ba trạng thái. `TURN` chỉ trộn tốc độ hai bánh bằng track. Không được gọi đây là LQR sáu trạng thái hoặc điều khiển vị trí/hướng đầy đủ.
- Form web có thể đổi bán kính để tính K, nhưng gói `$K` không gửi `WHEEL_RADIUS_M`, `TRACK_M`, `MICROSTEP`. Firmware vẫn dùng macro biên dịch cũ để đổi xung và vận tốc. Do đó thay r trên web có thể làm mô hình/gain và odometry dùng hai bán kính khác nhau. Cần một profile chung có version và xác nhận đọc lại từ firmware.
- `web_server.js::sendKToRobot` đặt `sentToRobot` theo việc ghi được vào link; chưa phải ACK parser đã chấp nhận, đã áp dụng hay đã lưu NVS. Hiển thị phải tách tính xong / gửi xong / xác nhận áp dụng / lưu.
- `dashboard.js` còn nhãn đồ thị `Tín Hiệu (V)` dù telemetry/CSV dùng F(N). Nhãn này cần sửa cùng hợp đồng đại lượng; không đánh đồng F ảo với điện áp hoặc mômen đo.
- `Params.cpp` dùng `MODEL_REVISION=20261108.0f` và lưu float. Float 32 bit không biểu diễn chính xác mọi số nguyên lớn hơn 2²⁴; các ngày revision liền nhau có thể trùng sau làm tròn. Profile version nên dùng số nguyên/chuỗi hoặc hash và xác thực mọi hệ số khi nạp.
- Có lớp IDLE/ARMING/RUN/FALLEN/FAULT, watchdog xung, giới hạn ngã và command timeout trong nguồn. Đó là cấu trúc tốt để tái dùng nhưng chưa có phép đo thời gian thực hay thử ngã phần cứng trong khảo sát này.

### 6.3 Tái dùng và thiết kế lại

| Phần | Quyết định | Điều kiện |
|---|---|---|
| Giao diện hiển thị, log, export, serial/TCP transport | Tái dùng có chỉnh sửa | Thống nhất tên trạng thái, đơn vị, ACK và profile; sửa nhãn V/F |
| State machine, watchdog, kiểm tra IMU, JOG giới hạn | Tái dùng sau kiểm tra | Đối chiếu board, chân, firmware core và timeout thực tế |
| Bộ phát STEP/DIR và đếm xung | Tái dùng như driver/ước lượng lệnh | Kiểm tra tần số/độ rộng DIR setup bằng logic analyzer; không gọi là encoder |
| Hệ số K và solver Riccati JS cũ | Không dùng như gain mặc định của mô hình mới | Sinh lại với đúng state/input/Ts và đối chiếu solver đáng tin cậy, residual, cực |
| Mô hình DC/SLX gốc | Tái dùng làm tài liệu phương trình/tham khảo LQR | Không chuyển trực tiếp Rm/Kt/Kb/n/PWM sang STEP/DIR; xử lý sai khác chart/script |
| Observer, odometry và tham chiếu | Thiết kế theo cảm biến thật | Công bố trạng thái đo được, suy ra, không quan sát được; mất bước không được phát hiện bằng step count |
| Mô hình động cơ bước và lực/mômen | Thiết kế/nhận dạng lại theo phạm vi mô phỏng | Công bố actuator lý tưởng hay có lag/saturation/torque envelope, và tác động phản lực thân |

## 7. Kế hoạch đóng các điểm chưa biết

1. **Xác nhận cấu hình lắp:** chụp tem motor, mã driver/board, jumper MS1–MS3, cách ghép trục/hub, pin nguồn; ghi model cụ thể thay cho chỉ “NEMA17” hoặc “ESP32”.
2. **Bánh và track:** đo hai đường kính bằng thước cặp, lăn đủ nhiều vòng có tải để lấy bán kính lăn hiệu dụng; đo tâm–tâm hai bánh và hai mặt trong, kiểm tra quan hệ hình học. Ghi dụng cụ, độ phân giải, ngày đo vào G, giữ nguyên giá trị tham khảo E/F để truy vết.
3. **Khối lượng:** cân toàn xe có pin/dây, cân riêng mỗi bánh/hub. Xác định `mb=mass_total-mw_left-mw_right` theo đúng phân nhóm mô hình, ghi motor/stator/rotor thuộc phần nào.
4. **COM:** đo vị trí COM bằng cân phản lực/tư thế nghiêng hoặc treo cân bằng có kiểm soát; đưa tọa độ về gốc trục bánh. Không lấy h/2, không cộng bán kính bánh nếu số đã là khoảng cách COM–trục.
5. **Quán tính:** dùng CAD với khối lượng thành phần đã cân, hoặc thử con lắc quanh trục biết vị trí; ghi rõ I_COM hay J_axle. Xác minh rotor từ datasheet đúng model; chưa có thì duy trì khoảng bất định.
6. **Dấu và xung:** thử bánh trên giá đỡ bằng JOG từng bên; một lệnh tiến dương phải cùng tiến ở hai bánh sau inversion. Xác nhận 3200 xung quay một vòng; thử pitch dương/gyro dương theo đúng trục IMU trước khi arm.
7. **Actuator và nguồn:** ghi dòng giới hạn driver, điện áp nguồn, đường mômen–tốc độ, gia tốc không mất bước và trễ tốc độ. Kiểm tra bằng phương tiện đo độc lập; đếm xung lệnh không chứng minh bánh đã đi.
8. **Đóng profile:** cập nhật một bảng tham số duy nhất, sinh lại mô hình/gain/cấu hình firmware/web, đọc lại profile/version khi kết nối. Chỉ chuyển từ “tạm dùng” sang “đã xác nhận” với chứng cứ đo đi kèm.

## 8. Giới hạn chứng cứ của khảo sát

Đã đọc: Excel dưới dạng cell dump; MATLAB nguồn; XML/metadata và MATLAB Function trong SLX; ino DC; firmware ESP32-S3, web solver/server/dashboard. Đã tính lại bằng số học độc lập các hệ số dẫn xuất. Chưa thực hiện: kiểm tra video, kiểm định datasheet đúng model, biên dịch/nạp nguồn DC gốc, chạy SLX gốc, đo phần cứng hoặc chứng minh ổn định trên xe thật. Mọi gain lấy từ mã cũ được ghi là giá trị nguồn, không phải đề nghị nạp lên xe.
