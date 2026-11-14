import hashlib,json,unittest,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ConfigurationTests(unittest.TestCase):
    def test_requested_standalone_thresholds(self):
        c=json.loads((ROOT/'shared/robot.json').read_text())
        self.assertAlmostEqual(c['safety']['arm_rad'],math.radians(10))
        self.assertAlmostEqual(c['safety']['fall_rad'],math.radians(40))
        self.assertEqual(c['safety']['auto_rearm_s'],5)
        header=(ROOT/'03_Firmware_ESP32S3/balancebot/GeneratedConfig.h').read_text()
        self.assertIn('constexpr double auto_rearm_s=5;',header)
    def test_hashed_config_checkout_preserves_lf(self):
        attributes=(ROOT/'.gitattributes').read_text()
        self.assertIn('shared/robot.json text eol=lf',attributes)
        self.assertNotIn(b'\r', (ROOT/'shared/robot.json').read_bytes())
    def test_generated_gain_matches_source_and_header(self):
        c=json.loads((ROOT/'shared/robot.json').read_text())
        d=json.loads((ROOT/'shared/design.json').read_text())
        digest=hashlib.sha256((ROOT/'shared/robot.json').read_bytes()).hexdigest()
        self.assertEqual(d['config_sha256'],digest)
        self.assertEqual(d['state_order'],c['lqr']['state_order'])
        header=(ROOT/'03_Firmware_ESP32S3/balancebot/GeneratedConfig.h').read_text()
        self.assertIn(digest,header)
        for i,k in enumerate(d['K']):self.assertIn(f'k{i}={k:.17g}',header)
    def test_runtime_gain_validation_uses_exported_discrete_model(self):
        d=json.loads((ROOT/'shared/design.json').read_text())
        header=(ROOT/'03_Firmware_ESP32S3/balancebot/GeneratedLqrModel.h').read_text()
        self.assertIn(d['config_sha256'],header)
        for i in range(2):
            for j in range(2):self.assertIn(f'a{i}{j}={d["Ad"][i][j]:.17g}',header)
            self.assertIn(f'b{i}={d["Bd"][i][0]:.17g}',header)
    def test_motor_build_uses_configured_pinmap(self):
        text=(ROOT/'03_Firmware_ESP32S3/balancebot/HardwareConfig.h').read_text()
        self.assertIn('#define ROBOT_HARDWARE_CONFIRMED 1',text)
        self.assertIn('#define ROBOT_REFERENCE_PINMAP 1',text)
if __name__=='__main__':unittest.main()
