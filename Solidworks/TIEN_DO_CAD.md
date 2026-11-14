# Tiến độ CAD — 14/11/2026

## Cập nhật mới nhất (ưu tiên hơn nhật ký bên dưới)

Đã có 9 part 01–09 cùng STEP. 05 vòng đệm, 06 đai ốc, 07 vít M3×5, 08 vít M3×10, 09 trụ AF4 dài42 ren cái thông suốt. Đã sửa lỗi API lưu Pitch=10 mm thành đúng0,5 mm bằng ModifyDefinition, kiểm tra lại sau mở: số vòng 4,8/10/20/84. `Kiem_tra_bo_sung.json` là kết quả cuối, báo cáo sơ bộ trước đó không đại diện bước ren cuối. Ảnh xem trước dựng từ9 STL đã xem. ZIP mới `Bo_part_robot_2026-11-14_bo_sung.zip` hiện9 part, đã testzip.

Đang dựng gá L bằng mode bracket của build_missing_parts.py: bề rộng50, chiều cao MỚI55 (user trả lời khi hỏi mâu thuẫn50), dày2, chân45. Tâm motor cao35, lỗ31×31, lỗ xuyên chọn3,4. User xác nhận hai tâm lỗ chân cách30, mỗi tâm cách mép bên10. Còn hỏi Ø lỗ tâm lớn và khoảng cách hàng lỗ chân đến mép ngoài chân45. Không lấy khoảng cách ngang10 làm chiều dọc. Mode bracket hiện lưu10_Ga_L_CHUA_DU_LO (chưa lỗ giữa/chân); chưa hoàn tất lúc viết checkpoint. Nếu xuất thêm cần sửa package_robot.py số lượng9, render_package.py tiêu đề, README và đóng gói lại.

Motor mã đã chốt42BYGH34-401A; chưa có bản vẽ chính hãng đủ kích thước, vẫn part04 tham khảo. Hub chưa biết kích thước. Không dựng lại01–09. Giữ nguyên file gốc1_Tam_khung.SLDPRT.

Đã dựng và lưu 4 part trong `Bo_part_robot_2026-11-13`: 01 tấm trên, 02 tấm dưới, 03 bánh tham khảo, 04 động cơ tham khảo. Có SLDPRT SolidWorks 2026 và STEP.

Thông số chốt mới: hai tấm 125×70×2; lỗ xuyên cho M3; rãnh 20×5, tâm cách mép dài 10; bốn lỗ phía trong tấm dưới cách mép đầu 20. Đường kính lỗ chọn 3,4 mm, chưa phải số đo trên xe.

Hai tấm đã mở lại và kiểm tra thể tích độc lập, số lỗ 8/12, 8 cung đầu rãnh mỗi tấm. Kích thước cực trị phù hợp. Bánh và động cơ chỉ là hình học giản lược để bố trí. Ảnh xem trước dựng từ STL thực xuất vì SaveBMP bị trắng.

Các phần chưa vẽ và số đo còn thiếu nằm trong `Bo_part_robot_2026-11-13/00_Doc_truoc.md`. Chưa có assembly, bản vẽ gia công hoặc đánh giá trọng tâm thực.

Script `build_robot_parts.py` dùng Python riêng `C:/Users/USER/.codex/runtimes/solidworks-automation/Scripts/python.exe`; COM cần chạy ngoài sandbox. Các chế độ `verify`, `mesh` đọc file đã lưu; không cần chạy lại chế độ tạo part nếu chưa có yêu cầu thay đổi. `render_package.py` dựng ảnh từ STL.

Không sửa file gốc `Solidworks/1_Tam_khung.SLDPRT`. Không cần dựng lại từ đầu khi tiếp tục. Bước tiếp theo sau bàn giao: nhận kích thước gá, hub, trụ và các tấm mica để bổ sung part.

## Đợt bổ sung đang thực hiện

Người dùng yêu cầu hoàn thành các phần đã nhận, không chỉ bốn part đầu.
Xác nhận mới: trụ 42 mm, AF4 mm, hai đầu ren cái M3; tâm motor cách mép dưới gá 35 mm; vít M3 dài 5 và 10 mm. Hub chưa xác định. Motor chỉ có tên NEMA42/thân34, chưa có mã hãng. Đang hỏi độ sâu ren trụ và vị trí hai lỗ chân gá.

Script bổ sung: `build_missing_parts.py`. 05 vòng đệm M3 (7/3,2/0,5), 06 đai ốc M3 (AF5,5/dày2,4/ren3D bước0,5), 07 vít M3×5, 08 vít M3×10. 05 phải tạo lỗ bằng cut riêng, không chỉ extrude hai vòng. 06 đã lưu bản lục giác đúng sau khi bản vẽ line bị auto-constraint làm méo. Chưa được coi hoàn tất tới khi chạy verify, xem STL và đóng gói lại.
