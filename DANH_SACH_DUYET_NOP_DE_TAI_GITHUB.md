# Danh sách chờ duyệt: nộp đề tài và đưa lên GitHub

Ngày rà soát: 16/11/2026. Đây là đề xuất, chưa xóa, di chuyển hay sửa cấu hình dự án.

Phân biệt ba việc: giữ trên máy, đưa vào bộ nộp, đưa vào GitHub. Không đưa lên GitHub không đồng nghĩa với xóa khỏi máy. Bộ nộp đề xuất gồm source, mô hình, CAD, tài liệu và bằng chứng kết quả; cần điều chỉnh nếu trường có mẫu riêng.

## A. Giữ trong source GitHub và bộ nộp

| Mã | File/thư mục | Đề xuất và lý do |
|---|---|---|
| A01 | `03_Firmware_ESP32_S3/balancebot/` toàn bộ 20 file | Giữ sketch, `.cpp`, `.h` và hướng dẫn. Đây là firmware chính. |
| A02 | `GeneratedConfig.h`, `GeneratedLqrModel.h`, `PhonePage.h` trong sketch | Giữ dù được sinh tự động: cần để người nhận build sketch ngay; giữ kèm script sinh. |
| A03 | `shared/` toàn bộ 14 file | Giữ cấu hình, thiết kế LQR, MATLAB và `controller_vectors.csv` dùng đối chiếu. |
| A04 | `01_LQR/LQR_CanBang.slx`, `run_lqr.m`, `run_all.m`, `README.md` | Giữ mô hình và cách chạy. |
| A05 | `02_PID/PID_ChuyenDong.slx`, `run_pid.m`, `run_all.m`, `README.md` | Giữ mô hình và cách chạy. |
| A06 | `4_Web/public/` trừ `index.backup-20261105-001615.html` | Giữ HTML/CSS/JS, icon, manifest và `qrcode.min.js` đang dùng. Không xóa thư viện chỉ vì là file minified. |
| A07 | `4_Web/web_server.js`, `auth.js`, `lqr.js`, `qr_console.js`, `phone-adapter.js`, `phone-drive.js`, `phone-drive.html`, `phone-drive.css` | Giữ mã server, xác thực và giao diện điện thoại. |
| A08 | `4_Web/package.json`, `package-lock.json`, `.gitignore` | Giữ để cài lại dependency và loại dữ liệu tài khoản. |
| A09 | `4_Web/Mo_Web.cmd`, `Mo_Web.ps1`, `start_server.bat`, `Cho_Phep_Dien_Thoai.cmd`, `Cho_Phep_Dien_Thoai.ps1` | Giữ công cụ chạy trên Windows và cấu hình kết nối điện thoại. |
| A10 | `4_Web/*.md`, `03_Firmware_ESP32_S3/README.md` | Giữ sau khi đồng bộ hướng dẫn với firmware hiện tại. |
| A11 | `tests/` trừ cache và các mục D01 | Giữ C++ tests, Python tests, mocks. Không xóa mocks vì tưởng thư viện thừa. |
| A12 | `4_Web/tests/` cả 4 file | Giữ kiểm thử API, browser, LAN và phone. |
| A13 | `tools/` trừ cache và hai script D01 | Giữ công cụ thiết kế, mô phỏng, sinh web/header, build và đối chiếu nguồn. Sửa đường dẫn cũ trước khi bàn giao. |
| A14 | `README.md`, `THU_XE.md`, `CHUC_NANG_VAN_HANH.md`, `TOM_TAT_TOAN_BO_DE_TAI.md`, `DANH_MUC_FILE.md` | Giữ thông tin hữu ích; sửa phần lỗi thời, rút gọn phần trùng. Chưa đề nghị xóa nguyên file. |
| A15 | `.gitattributes` | Giữ quy định xuống dòng, ảnh hưởng hash cấu hình. |
| A16 | `05_Bao_Cao/Bang_do_thong_so_mo_hinh.xlsx`, `00_Thiet_ke_va_ke_hoach.md`, `01_Khao_sat_nguon.md`, `02_Mo_hinh_va_dieu_khien.md`, `06_Trang_thai_kiem_chung.md`, `07_So_do.md`, `08_Ket_qua_Python.md` | Giữ bảng số liệu và tài liệu đề tài; cập nhật trạng thái kiểm chứng. Chưa kiểm tra nội dung workbook trong đợt phân loại này. |
| A17 | `Solidworks/part_robot/Chi_tiet/*.SLDPRT`, `*.STEP` | Giữ cả CAD gốc và bản trao đổi. Hai định dạng không phải bản trùng có thể tùy tiện xóa. Ngoại lệ cần xem thêm D03. |
| A18 | `Solidworks/part_robot/Lap_rap/*.SLDASM` | Tạm giữ cả hai assembly; chỉ chọn bản cuối sau khi mở lại kiểm tra tham chiếu. |
| A19 | `Solidworks/*.py` (10 script) | Giữ nguồn dựng/sửa CAD, nhưng ghi rõ thứ tự chạy, dependency và đường dẫn. Không chạy lại tự động để dọn file. |

## B. Giữ bằng chứng; chọn lọc khi công khai

| Mã | File/thư mục | Bộ nộp | GitHub |
|---|---|---|---|
| B01 | `01_LQR/results/`, `02_PID/results/` | Giữ CSV, PNG, MAT, JSON và log phục vụ đối chiếu kết quả | Giữ kết quả tiêu biểu và dữ liệu dùng trong báo cáo; kết quả đầy đủ có thể đóng gói riêng |
| B02 | `06_Kiem_tra/results/` | Giữ các ca mô phỏng, quét Q/R, sensitivity, metrics, manifest và log | Giữ nếu báo cáo viện dẫn; không xóa CSV chỉ vì có thể chạy lại |
| B03 | `06_Kiem_tra/stop_modes_20261114/` | Giữ nguyên gói minh chứng mốc cập nhật cuối đang có, gồm source ZIP, verification ZIP, hash, log, ảnh và binary | Đưa phần kết quả/ảnh đã rà soát vào repo; ZIP/binary có thể để gói phát hành riêng. Chưa xác nhận mốc này khớp toàn bộ source hiện tại |
| B04 | `06_Kiem_tra/lqr_only/20261112_speed_rail/` | Giữ `telemetry.json`, `PHAN_TICH.md`, `video_frames.png`: dữ liệu phân tích thử xe | Có thể công khai sau khi xem ảnh và nội dung riêng tư |
| B05 | `06_Kiem_tra/standalone_20261114_r4/robot_1794634350468.json`, `log-analysis.json` | Giữ dữ liệu đo và phân tích, không gộp vào rác build | Giữ nếu dùng làm minh chứng, gắn rõ phiên bản firmware |
| B06 | `4_Web/results/` trừ hai server log ở C03 | Giữ ảnh demo, `browser_telemetry.csv`, `browser_commands.json` nếu dùng minh chứng | Chọn ảnh cuối phù hợp giao diện hiện tại; dữ liệu test ghi rõ là mô phỏng |
| B07 | `Solidworks/part_robot/Kiem_tra/` | Giữ báo cáo JSON/Markdown, kết quả kiểm tra độc lập và STL cần minh chứng | Giữ kết quả liên quan CAD cuối; báo cáo tên cũ không mặc nhiên chứng minh part đã sửa |
| B08 | `.arduino/board-profile.md` | Giữ thông tin phần cứng sau khi cập nhật | Có thể chuyển nội dung đã xác minh vào hướng dẫn phần cứng |

Log kết quả build/test và manifest SHA256 phải đi cùng đúng source/binary được kiểm tra. Không đổi tên hoặc bỏ các file bên trong một bộ minh chứng mà vẫn tuyên bố bộ đó còn kiểm tra hash được.

Đã phát hiện `.build/diagnostics/` chứa bản ghi COM17, hash bằng chứng và ảnh chẩn đoán/hiệu chuẩn. Giữ các file này như bằng chứng thực nghiệm; chỉ chuyển ra khỏi `.build` sau khi duyệt, không xóa cùng cache.

## C. Có thể bỏ khỏi bộ nộp/GitHub; có thể xóa bản làm việc sau duyệt

| Mã | Đường dẫn cụ thể | Điều kiện |
|---|---|---|
| C01 | `tools/__pycache__/`, `tests/__pycache__/` | Cache Python, tạo lại được. |
| C02 | `01_LQR/slprj/`, `02_PID/slprj/`, `01_LQR/LQR_CanBang.slxc`, `02_PID/PID_ChuyenDong.slxc` | Cache Simulink. Giữ `.slx` và script `.m`. |
| C03 | `4_Web/results/server.log`, `4_Web/results/server.err.log` | Log server làm việc; lưu riêng trước nếu còn dùng chẩn đoán. Không áp dụng cho mọi `.log` trong dự án. |
| C04 | `4_Web/public/index.backup-20261105-001615.html` | Bản backup giao diện, đề xuất loại khỏi bản phát hành; có thể lưu riêng nếu muốn giữ lịch sử. |
| C05 | `4_Web/node_modules/` | Không đưa vào bộ source. Có thể xóa sau khi xác nhận cài lại bằng `npm ci`; xóa sẽ làm web và script sinh phone page cần cài lại thư viện. |
| C06 | File biên dịch/cache trong `.build/`: `.o`, `.d`, `.elf`, `.map`, `.exe`, `.pdb`, cache toolchain và dependency cài cục bộ | Không đưa vào bộ source. Chỉ dọn sau khi bảo toàn nội dung riêng ở D02 và binary cần bàn giao. Không duyệt xóa toàn bộ `.build` theo mục này. |

## D. Lưu trữ riêng hoặc kiểm tra thêm trước khi bỏ

| Mã | File/thư mục | Đề xuất |
|---|---|---|
| D01 | `tools/build_lqr_only.ps1`, `tools/build_lqr_60_test.ps1`, `tests/test_lqr_only.py`, `tests/cpp/lqr60_tests.cpp` | Chuyển sang bộ thử nghiệm lịch sử nếu đề tài cuối chỉ bàn giao bản LQR+PID. Hai sketch tương ứng đang bị xóa trong working tree; không xóa test chỉ để che lỗi chạy. Nếu cần tái lập thí nghiệm LQR riêng, giữ kèm đúng source cũ. |
| D02 | `.build/hold_zero_test.cpp`, `.build/preview-drive-pad.cjs`, `.build/pid-default-before.json`, `.build/video-grid.jpg`, `.build/diagnostics/`, `.build/web-input/` và các source/snapshot nằm trong thư mục build khác | Giữ riêng để rà soát trước. `.build` có nội dung viết tay và chẩn đoán, không chỉ cache. Các file phone HTML gzip có thể sinh lại, nhưng phone tests đọc trực tiếp từ `.build`, nên cần hướng dẫn bước sinh trước test. |
| D03 | `Solidworks/part_robot/Chi_tiet/Ban_goc_truoc_khi_sap_xep/`, `21_Vit_ham_M3_THAM_KHAO.SLDPRT`; hai assembly `Robot_lap_so_bo.SLDASM`, `Robot_lap_rap_da_doi_ten.SLDASM` | Chưa cho phép xóa: phải xác nhận assembly cuối mở đủ part và không tham chiếu bản cũ. Tên file không đủ chứng minh file thừa. |
| D04 | `Solidworks/Bo_part_robot_2026-11-13/` | Bộ CAD lịch sử. Đề xuất lưu ngoài bộ nộp chính sau khi kiểm tra assembly và giữ thông tin nguồn/kiểm chứng cần thiết. Không coi toàn bộ bộ này trùng với `part_robot`. |
| D05 | `06_Kiem_tra/phone_web_20261114/`, `phone_desktop_20261114/`, `phone_drive_20261114/`, `phone_auth_20261114/` | Lưu trữ nguyên bộ lịch sử, không cần đưa tất cả vào repo chính. Chỉ bỏ bản làm việc sau khi đã có bản lưu khác và xác định báo cáo không cần chúng. |
| D06 | `06_Kiem_tra/standalone_20261114/`, `standalone_20261114_r2/`, `standalone_20261114_r3/`, `standalone_20261114_r4/`, `restore_r4_20261114/`, `wifi_restore_20261114/`, `tcp_stability_20261114/`, `pid_default_3102/` | Lưu nguyên bộ lịch sử. Riêng dữ liệu B05 vẫn giữ trong bằng chứng đề tài. Các thư mục `before/`, bản source cũ, ZIP và binary là snapshot, không mặc nhiên là file trùng. |
| D07 | `06_Kiem_tra/lqr_only/` (ngoài B04), `06_Kiem_tra/lqr_60_test/` | Giữ riêng nếu trình bày so sánh/thử nghiệm LQR. Không dùng firmware thử cũ làm firmware bàn giao chính. |
| D08 | `.arduino/goal.md`, `.arduino/next-todo.md`, `05_Bao_Cao/09_Skill_va_nguon_hang.md` | Ghi chú quá trình làm việc; có thể để riêng, rút phần kỹ thuật cần thiết vào tài liệu chính. |
| D09 | `00_Nguon/` toàn bộ 42 file | Giữ trên máy và trong tư liệu nguồn; `source_parameter_audit.py` đang đọc một số file ở đây. Trước khi công khai các bản sao mã nguồn, xác định nguồn và giấy phép. Nếu chỉ công khai trích dẫn/link, phải sửa hướng dẫn công cụ phụ thuộc. Chưa xác minh quyền phân phối trong lần rà soát này. |

## E. Không đưa dữ liệu cá nhân lên GitHub/bộ nộp chung

Đối chiếu SHA256 cho thấy 67 file cùng tên trong hai thư mục `Solidworks/Bo_part_robot_2026-11-13/Kiem_tra/` và `Solidworks/part_robot/Kiem_tra/` giống từng byte. Có thể chỉ mang một bộ vào bản nộp sau khi chỉnh liên kết tài liệu. Kết quả này chỉ áp dụng cho 67 file kiểm tra, không áp dụng cho các part hoặc assembly.

- `4_Web/data/users.json`: có tồn tại, là dữ liệu tài khoản do `auth.js` ghi (tên tài khoản, salt, hash). Giữ riêng trên máy nếu cần đăng nhập tiếp; không sao chép vào bộ nộp và không commit. Không cần hiển thị nội dung để phân loại.
- `.git/`: giữ nguyên trên máy để bảo toàn lịch sử; không nhét vào ZIP nộp đề tài. Không xóa thư mục này để “dọn GitHub”.
- Trước khi công khai, rà lại nội dung log, ảnh, ZIP, đường dẫn máy cá nhân và cấu hình kết nối. Đợt này chưa kiểm tra đầy đủ nội dung từng ảnh/archive hoặc lịch sử Git, nên chưa chứng nhận toàn bộ repo sạch thông tin riêng tư.

## F. Việc cần hoàn thiện trước khi đóng gói, chưa thực hiện

1. Tạo `.gitignore` đúng tên từ nội dung đã chọn; `.gitignore.txt` hiện không có tác dụng của file ignore gốc. Giữ `4_Web/.gitignore`. Không ignore toàn bộ `.csv`, `.json`, `.mat`, `.log` vì có bằng chứng và đầu vào cần giữ.
2. Sửa đường dẫn cũ trong build/test/generator và tài liệu: `03_Firmware_ESP32S3`, `01_LQR_CanBang`, `02_PID_ChuyenDong`, `05_BaoCao`, `06_KiemThu`.
3. Đồng bộ tài liệu về tự cân bằng, LQR2, Wi-Fi và hai chế độ dừng; phân biệt kết quả lịch sử với bản bàn giao.
4. Xác nhận dependency/toolchain có thể cài lại trước khi bỏ dependency trong `.build` và `node_modules`.
5. Mở assembly cuối, kiểm tra đầy đủ tham chiếu; đóng gói kèm part được dùng. Chưa mở SolidWorks trong đợt này.
6. Chạy lại build/test sau dọn trên một bản sao sạch, rồi tạo hash/gói bàn giao mới. Chưa chạy build/test trong lần lập danh sách.
7. Chốt nguồn và thông tin giấy phép cho mã kế thừa/thư viện; không tự gán giấy phép cho toàn bộ dự án chỉ từ trường license của package web.

## Các lựa chọn để duyệt

- [ ] Giữ nhóm A, sửa tài liệu/đường dẫn trước bàn giao.
- [ ] Giữ nhóm B làm bằng chứng, chọn kết quả tiêu biểu đưa lên GitHub.
- [ ] Cho phép dọn C01–C04.
- [ ] Cho phép bỏ dependency/cache C05–C06 sau khi kiểm tra khả năng khôi phục và bảo toàn D02.
- [ ] Lưu trữ nhóm D ngoài bộ source chính; chưa xóa vĩnh viễn.
- [ ] Loại dữ liệu nhóm E khỏi mọi gói công khai, giữ dữ liệu gốc trên máy.

Khuyến nghị: tạo một bản sao bàn giao sạch theo danh sách đã duyệt, giữ nguyên thư mục làm việc gốc. Như vậy có thể kiểm thử bộ nộp và rà soát trước khi quyết định dọn bản gốc.
