"""Drag-bar target setting: persistence, validation and live IPC, with no headset input."""
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

os.environ["HOME"] = tempfile.mkdtemp(prefix="ft-display-bar-")
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import ft_display_settings as fds  # noqa: E402
from PySide6.QtGui import QGuiApplication  # noqa: E402


class GrabBar(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QGuiApplication.instance() or QGuiApplication([])

    def setUp(self):
        Path(fds.CONF_PATH).parent.mkdir(parents=True, exist_ok=True)
        Path(fds.CONF_PATH).write_text("# keep this\nGRAB_BAR_HIT_SCALE=1 # keep this too\n")
        self.b = fds.Backend()
        self.b._running = True
        self.asked = []
        self.b._ask_screens = lambda text, then=None: self.asked.append(text)
        self.backend = patch.object(fds.ft_layout, "backend", return_value="screens")
        self.backend.start()
        self.addCleanup(self.backend.stop)

    def test_default_and_live_setting(self):
        self.assertEqual(self.b.grabBarHitScale, 1)
        for scale in (3, 2.75, 5, 1):
            self.b.setGrabBarHitScale(scale)
            self.assertEqual(self.b.grabBarHitScale, scale)
            self.assertEqual(self.asked[-1], f"handsetting GRAB_BAR_HIT_SCALE {scale:g}")
        text = Path(fds.CONF_PATH).read_text()
        self.assertIn("# keep this", text)
        self.assertIn("# keep this too", text)

    def test_bad_persisted_values_use_default(self):
        for value in ("nan", "inf", "junk", "0", "6"):
            Path(fds.CONF_PATH).write_text(f"GRAB_BAR_HIT_SCALE={value}\n")
            self.assertEqual(self.b.grabBarHitScale, 1)

    def test_setter_bounds_and_nonfinite_values(self):
        for value, want in ((0, 1), (20, 5), (float("nan"), 1), (float("inf"), 1)):
            self.b.setGrabBarHitScale(value)
            self.assertEqual(self.b.grabBarHitScale, want)

    def test_saved_without_running_screens(self):
        self.b._running = False
        self.b.setGrabBarHitScale(3)
        self.assertEqual(self.b.grabBarHitScale, 3)
        self.assertEqual(self.asked, [])
        self.b._running = True
        with patch.object(fds.ft_layout, "backend", return_value="gamescope"):
            self.b.setGrabBarHitScale(2)
        self.assertEqual(self.asked, [])

    def test_ui_exposes_the_setting(self):
        qml = (Path(fds.HERE) / "main.qml").read_text()
        self.assertIn("value: backend.grabBarHitScale", qml)
        self.assertIn("onMoved: backend.setGrabBarHitScale(value)", qml)


if __name__ == "__main__":
    unittest.main()
