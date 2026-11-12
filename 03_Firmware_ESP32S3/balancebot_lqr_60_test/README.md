# LQR 60-degree test profile
Separate experimental sketch; original firmware and LQR-only remain unchanged.
Open balancebot_lqr_60_test.ino with all sibling files present.
Only LQR runs: k0=-23.793396834761598, k1=-2.7542335398052704.
Existing mechanical parameters are retained, not remeasured or revalidated.
Removed the 4-degree START/Arming pitch gate. Low angular rate for 1 second is still required.
FALL limit is 60 degrees of zero-corrected pitch, checked during Arming and Run.
START above 60 degrees is rejected. No automatic START after reset or boot.
TestSafetyConfig.h is the explicit source of this profile override; GeneratedConfig.h is unchanged.
IMU validity, DEADLINE, SPEED_RAIL (0.6 m/s for 300 ms), STOP/ESTOP/RESET are retained.
This does NOT fix SPEED_RAIL or prove recovery at 60 degrees; the existing linear LQR has not been qualified there.
Use a restrained test rig; do not release a tilted robot onto the floor.
Motor power off for uploading. Disconnect bridge/Serial Monitor from the upload port.
ESP32S3 Dev Module, Flash16MB, OPI PSRAM, Hardware CDC/JTAG, CDC On Boot Enabled.
GET marker: EVT:CONTROL,LQR_60_TEST,PID_DISABLED,ARM_ANGLE_GATE_OFF,FALL_DEG_60.
Base configuration hash is unchanged: identify this profile by the GET marker and source manifest.
NVS zero is shared with balancebot; SAVE can affect the original image when restored.
No need to change zero/calibration guards; these are deliberately preserved.
Rollback: upload ../balancebot_lqr_only/balancebot_lqr_only.ino.
