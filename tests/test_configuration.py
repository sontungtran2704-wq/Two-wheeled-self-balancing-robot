import hashlib,json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ConfigurationTests(unittest.TestCase):
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
    def test_motor_build_uses_configured_pinmap(self):
        text=(ROOT/'03_Firmware_ESP32S3/balancebot/HardwareConfig.h').read_text()
        self.assertIn('#define ROBOT_HARDWARE_CONFIRMED 1',text)
        self.assertIn('#define ROBOT_REFERENCE_PINMAP 1',text)
if __name__=='__main__':unittest.main()
