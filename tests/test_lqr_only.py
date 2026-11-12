"""Source isolation checks; these do not qualify physical balancing."""
import hashlib
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / '03_Firmware_ESP32S3/balancebot'
BENCH = ROOT / '03_Firmware_ESP32S3/balancebot_lqr_only'


class LqrOnlyTests(unittest.TestCase):
    def test_shared_safety_and_hardware_sources_unchanged(self):
        for name in ('ControlCore.h', 'GeneratedConfig.h', 'HardwareConfig.h',
                     'ImuCalibration.h', 'ImuHal.cpp', 'ImuHal.h',
                     'StepNco.h', 'StepperHal.cpp', 'StepperHal.h'):
            with self.subTest(name=name):
                self.assertEqual((BASE/name).read_bytes(), (BENCH/name).read_bytes())

    def test_only_control_changes_are_motion_gate_and_command_rejection(self):
        original = (BASE/'ControlCore.cpp').read_text()
        bench = (BENCH/'ControlCore.cpp').read_text()
        gate = ('  // Dedicated bench image: outer loops cannot be enabled by any caller.\n'
                '  motion=false;vt=wt=0;\n')
        rejection = ('  if(!std::strcmp(tok[0],"MOTION") || !std::strcmp(tok[0],"SPEED") ||\n'
                     '     !std::strcmp(tok[0],"TURN") || !std::strcmp(tok[0],"CMD") || !std::strcmp(tok[0],"$P")) {\n'
                     '    reply("ERR:LQR_ONLY");return;\n'
                     '  }\n')
        self.assertEqual(bench.count(gate), 1)
        self.assertEqual(bench.count(rejection), 1)
        self.assertEqual(bench.replace(gate, '').replace(rejection, ''), original)

    def test_mode_identification_and_config_hash(self):
        original = (BASE/'Runtime.cpp').read_text()
        bench = (BENCH/'Runtime.cpp').read_text()
        marker = '    emit(source,"EVT:CONTROL,LQR_ONLY,PID_DISABLED");\n'
        self.assertEqual(bench.count(marker), 1)
        self.assertEqual(bench.replace(marker, ''), original)
        digest = hashlib.sha256((ROOT/'shared/robot.json').read_bytes()).hexdigest()
        self.assertIn(digest, (BENCH/'GeneratedConfig.h').read_text())


if __name__ == '__main__':
    unittest.main()
