# Bộ part robot hai bánh — bàn giao 14/11/2026

Mỗi part có file SolidWorks 2026 `.SLDPRT` và bản trao đổi `.STEP` cùng tên. Đơn vị thiết kế: mm. Đây là các part rời, chưa có assembly và bản vẽ gia công.

## Danh mục

| STT | Tên | Phạm vi |
|---|---|---|
| 01 | Tam_nhom_tren | Tấm 70 × 125 × 2; 8 lỗ xuyên; 4 rãnh 20 × 5 |
| 02 | Tam_nhom_duoi | Tấm 70 × 125 × 2; 12 lỗ xuyên; 4 rãnh 20 × 5 |
| 03 | Banh_xe_THAM_KHAO | Mô hình giản lược Ø66, rộng 25, lỗ tâm Ø12; không mô tả nan, hoa lốp, phần nhô moay-ơ và phân chia lốp/vành |
| 04 | Dong_co_THAM_KHAO | Mô hình giản lược thân 42 × 42 × 34, trục Ø5 nhô 25; chưa có mặt vát D, gờ định tâm, ren bắt gá, giắc điện và cơ cấu bên trong |
| 05 | Vong_dem_M3 | Ø ngoài 7, Ø trong 3,2, dày 0,5 |
| 06 | Dai_oc_M3 | Ngang hai mặt 5,5, dày 2,4; ren trong M3×0,5 |
| 07 | Vit_M3x5 | Đầu trụ lục giác chìm; chiều dài dưới đầu 5; ren M3×0,5 |
| 08 | Vit_M3x10 | Đầu trụ lục giác chìm; chiều dài dưới đầu 10; ren M3×0,5 |
| 09 | Tru_luc_giac_M3_42 | Dài 42, ngang hai mặt 4; ren cái M3×0,5 thông suốt |
| 10 | Ga_L_CHUA_DU_LO | Thân 50 rộng × 55 cao, chân sâu ngoài 45, dày 2; lỗ giữa Ø22 tâm cao35; bốn lỗ Ø3,4 bước31×31. Chưa cắt hai lỗ chân vì thiếu vị trí dọc |

Part 03–04 chỉ dùng bố trí và minh họa, không dùng chế tạo hoặc tính khối lượng/trọng tâm thực. Không gán mác vật liệu cụ thể khi chưa được cung cấp; khối lượng mặc định trong CAD không đại diện cho xe thật.

## Quy ước và dữ liệu hai tấm

Gốc tại tâm hình chữ nhật. Trong file part: X theo cạnh 70, Y theo cạnh 125, Z theo chiều dày. Đây là hệ tọa độ riêng của part, chưa phải hệ tọa độ điều khiển robot.

- Kích thước và bố trí lấy từ hai bản phác của người dùng cùng các xác nhận trong chat.
- Hai tấm dày 2, vật liệu thực là nhôm.
- 8 lỗ ngoài: X = ±30; Y = ±55 và ±37,5.
- Tấm dưới thêm 4 lỗ: X = ±12,5; Y = ±42,5, tương ứng cách mép đầu 20.
- 4 rãnh: tâm X = ±25; Y = ±17,5. Tâm rãnh cách mép dài 10.
- Rãnh dài tổng cộng 20, rộng 5, hai đầu bán kính 2,5. Tâm hai cung cách nhau 15.
- Mép đầu tới hàng lỗ ngoài cùng = 7,5, suy ra từ đối xứng và kích thước 125/110.
- Lỗ cho vít M3 được người dùng xác nhận là lỗ xuyên, không ren. Người dựng chọn Ø3,4 theo cỡ lỗ lắp thông thường; đây không phải đường kính đo trên mẫu. Nếu cần sao chép đúng lỗ hiện có, phải kiểm tra lại đường kính thực.
- Nguồn đối chiếu cỡ lỗ: https://www.albanycountyfasteners.com/media/6b/b8/g0/1764944339/clearance-hole-chart.pdf?ts=1764944339

Các sketch dùng kích thước/quan hệ hình học, không khóa hàng loạt bằng Fix. Thân rãnh và đầu tròn được cắt thành hai feature; hình học cuối là rãnh đầu bán nguyệt. Sketch đầu tròn có thể được điều khiển hoàn toàn bằng quan hệ với thân rãnh, nên bộ kiểm tra tổng quát có thể báo `review_required` vì không có kích thước riêng.

## Chưa vẽ / chưa hoàn thiện

| Part/cụm | Dữ liệu cần bổ sung |
|---|---|
| Gá động cơ trái/phải | Đã sửa chiều cao thành55, lỗ giữaØ22, tâm motor cao35. Hai tâm lỗ chân cách30, mỗi tâm cách mép bên10. Chỉ còn vị trí dọc hàng lỗ chân tới mép ngoài chân45; bán kính uốn chưa được đo nên mô hình dùng góc sắc giản lược |
| Các trụ/spacer khác | Trụ 42 đã dựng; các chiều dài khác trên xe chưa có số đo |
| Hub nối trục với bánh | Đường kính ngoài, chiều dài, hai đầu lắp, vít hãm và độ nhô |
| Bánh xe chi tiết | Biên dạng nan, hoa lốp, vành, hub và phân chia vật liệu |
| Động cơ chi tiết | Đã nhận mã 42BYGH34-401A; chưa có bản vẽ kích thước đúng phiên bản để chốt gờ định tâm, mặt vát D, độ sâu ren và đầu nối |
| Các tấm mica trung gian | Dài/rộng/dày, tọa độ lỗ và cao độ từng tầng |
| PCB điều khiển, driver, nguồn, IMU, bo trung gian | Mã bo thực, kích thước từng cụm, lỗ bắt và vị trí đầu nối |
| Khay pin, pin, công tắc | Biên dạng và lỗ gá, lỗ panel, cách cố định |
| Vít, đai ốc, vòng đệm khác | Đã dựng M3×5, M3×10, đai ốc và vòng đệm; còn số lượng, vị trí lắp và các cỡ khác |
| Dây điện | Đường đi, đường kính bó, đầu nối và chiều dài |
| Assembly toàn xe | Khoảng cách hai bánh được xác nhận, chiều cao tầng, vị trí các cụm và các part còn thiếu |

## Kiểm tra

Thư mục `Kiem_tra` lưu kết quả đọc lại từ SolidWorks và ảnh xem trước. `Kiem_tra_doc_lap.json` kiểm tra thể tích hai tấm bằng công thức độc lập, số lỗ tròn và số cung đầu rãnh. Kích thước `exact_extrema_envelope_mm` là kích thước bao đo bằng các điểm cực trị; `GetPartBox` trong báo cáo tổng quát là hộp bao xấp xỉ và có thể lệch nhẹ.

Kiểm tra độc lập hai tấm đã đạt với sai số thể tích dưới 0,1 mm³. Ảnh `00_Xem_truoc.png` được dựng từ lưới STL xuất trực tiếp từ bốn part, đã xem kiểm tra bố trí. Chức năng SaveBMP của phiên SolidWorks trả ảnh trắng; các ảnh BMP lỗi được chuyển ra thư mục nhật ký, không dùng làm bằng chứng kiểm tra. Các báo cáo tự động cũ vẫn giữ nguyên để truy vết, kể cả đường dẫn BMP và cảnh báo của bộ kiểm tra sketch. Kết quả hình học độc lập và ảnh từ STL là bằng chứng bổ sung, không phải chứng nhận gia công.

Trước gia công cần chốt đường kính lỗ thực, mác nhôm, dung sai và yêu cầu xử lý mép. Bộ này chưa bao gồm các yêu cầu gia công đó.

## Bổ sung 05–09

Các part mới đã mở lại và kiểm tra; ren thật dạng xoắn có bước 0,5 mm. Kết quả cuối lưu trong `Kiem_tra_bo_sung.json`, thay thế các thông số ren trong báo cáo sơ bộ trước lần sửa. Vòng đệm đạt kiểm tra thể tích độc lập. Ảnh xem trước hiện gồm cả chín part từ STL thực xuất.

Đây là mô hình lắp ráp giản lược: bỏ các vát mép nhỏ và bo chân đầu vít. Ren do SolidWorks tạo có sketch profile chứa quan hệ Fix của thư viện; sketch hình học chính có kích thước điều khiển. Không dùng mô hình này làm dưỡng kiểm ren hay bản vẽ chế tạo vít. Chiều dài vít và kích thước trụ theo người dùng; hình dạng đầu vít chọn loại đầu trụ lục giác chìm.

Kích thước danh nghĩa đối chiếu bảng Bossard: [vít](https://media.distrelec.com/Web/Downloads/_t/ds/1003763_eng_tds.pdf), [đai ốc](https://media.distrelec.com/Web/Downloads/_t/ds/1241613_eng_tds.pdf), [vòng đệm](https://xonstorage.z8.web.core.windows.net/pdf/bossard_1887521_apr22_xonlink.pdf).
