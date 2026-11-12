"""Read-only traceability check; no hardware access and no source modifications."""
from pathlib import Path
import json
import math
import re
import xml.etree.ElementTree as ET
import zipfile


def main():
    root = Path(__file__).resolve().parents[1]
    cells = json.loads((root / "00_Nguon/workbook_cells.json").read_text(encoding="utf-8"))
    sheet = cells["01_Danh_sach_can_do"]
    evidence = {f"{col}{row}": sheet.get(f"{col}{row}")
                for row in (5, 32, 33, 44, 56, 57) for col in "CEFGK"}
    expected = {"E5": "Ø80 (đã trao đổi)", "F5": "66mm", "G5": None,
                "K5": "Chưa đo", "E32": "160 (đã trao đổi trước đó)",
                "F32": "20.5mm", "G32": None, "K32": "Chưa đo"}
    for key, value in expected.items():
        assert sheet.get(key) == value, f"Source changed: {key}"
    ts = cells["Thong_so"]
    mb, mw, r, ell, h, ir, track, steps, micro = (ts[f"C{i}"] for i in range(2, 11))
    iw = 0.5 * mw * r**2
    icom = mb * h**2 / 12
    jaxle = icom + mb * ell**2
    meq = 2 * mw + 2 * (iw + ir) / r**2
    mt = mb + meq
    denom = jaxle - (mb * ell)**2 / mt
    a = mb * 9.81 * ell / denom
    b = -(mb * ell / mt) / denom
    derived = dict(total_mass_if_body_excludes_wheels=mb + 2*mw,
                   Iw_disk=iw, I_COM_uniform_rod=icom, J_axle=jaxle,
                   track_provisional=track, steps_per_rev=steps*micro,
                   meters_per_step=2*math.pi*r/(steps*micro),
                   steps_per_meter=(steps*micro)/(2*math.pi*r),
                   a=a, b=b, A41=-(mb*ell/mt)*a,
                   B4=(1-mb*ell*b)/mt, FrefGain=-a/b)
    base = root / "00_Nguon/lqr_reference/LQR/matlab"
    source = (base / "CacFunctionTrongSimulink_LQR.m").read_text(encoding="utf-8-sig")
    with zipfile.ZipFile(base / "Mophong.slx") as z:
        chart = ET.fromstring(z.read("simulink/stateflow/chart_16.xml"))
        script = next(p.text for p in chart.iter("P") if p.get("Name") == "script")
        config = ET.fromstring(z.read("simulink/configSet0.xml"))
        solver = {p.get("Name"): p.text for p in config.iter("P")
                  if p.get("Name") in ("SolverName", "FixedStep", "StartTime", "StopTime")}
    resistance = {"matlab_Rm": re.search(r"(?m)^Rm\s*=\s*([^;]+)", source).group(1),
                  "slx_Rm": re.search(r"(?m)^Rm\s*=\s*([^;]+)", script).group(1)}
    assert resistance == {"matlab_Rm": "50", "slx_Rm": "40"}
    assert "Jm*W^2 + Jm*W^2*n^2" in script
    assert "Jw*W^2 + Jm*W^2*n^2" in source
    print(json.dumps(dict(status="source assertions passed; no hardware validation",
                          workbook=evidence, derived_provisional=derived,
                          slx_solver=solver, mismatch=resistance,
                          yaw_denominator_mismatch="SLX Jm*W^2 versus MATLAB Jw*W^2"),
                     ensure_ascii=True, indent=2))


if __name__ == "__main__":
    main()
