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
from PySide6.QtCore import QUrl  # noqa: E402
from PySide6.QtQml import QQmlApplicationEngine  # noqa: E402


class GrabBar(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QGuiApplication.instance() or QGuiApplication([])

    def setUp(self):
        Path(fds.CONF_PATH).parent.mkdir(parents=True, exist_ok=True)
        Path(fds.CONF_PATH).write_text("# keep this\nGRAB_BAR_PADDING_Y=2 # keep this too\n")
        self.b = fds.Backend()
        self.b._running = True
        self.asked = []
        self.b._ask_screens = lambda text, then=None: self.asked.append(text)
        self.backend = patch.object(fds.ft_layout, "backend", return_value="screens")
        self.backend.start()
        self.addCleanup(self.backend.stop)

    def test_default_and_live_setting(self):
        self.assertEqual(self.b.grabBarPaddingY, 2)
        self.assertEqual(self.b.grabBarPaddingX, 2)
        for cm in (3, 2.7, 5, 50, 0):
            self.b.setGrabBarPaddingY(cm)
            self.assertEqual(self.b.grabBarPaddingY, cm)
            self.assertEqual(self.asked[-1], f"handsetting GRAB_BAR_PADDING_Y {cm:g}")
        text = Path(fds.CONF_PATH).read_text()
        self.assertIn("# keep this", text)
        self.assertIn("# keep this too", text)

    def test_bad_persisted_values_use_default(self):
        for value in ("nan", "inf", "junk", "-1", "51"):
            Path(fds.CONF_PATH).write_text(f"GRAB_BAR_PADDING_Y={value}\n")
            self.assertEqual(self.b.grabBarPaddingY, 2)

    def test_setter_bounds_and_nonfinite_values(self):
        for value, want in ((-1, 0), (60, 50), (float("nan"), 2), (float("inf"), 2)):
            self.b.setGrabBarPaddingY(value)
            self.assertEqual(self.b.grabBarPaddingY, want)

    def test_saved_without_running_screens(self):
        self.b._running = False
        self.b.setGrabBarPaddingY(3)
        self.assertEqual(self.b.grabBarPaddingY, 3)
        self.assertEqual(self.asked, [])
        self.b._running = True
        with patch.object(fds.ft_layout, "backend", return_value="gamescope"):
            self.b.setGrabBarPaddingY(2)
        self.assertEqual(self.asked, [])

    def test_ui_exposes_the_setting(self):
        qml = (Path(fds.HERE) / "main.qml").read_text()
        self.assertIn("value: backend.grabBarPaddingY", qml)
        self.assertIn("onMoved: backend.setGrabBarPaddingY(value)", qml)
        self.assertIn("onMoved: backend.setGrabBarPaddingX(value)", qml)
        self.assertIn("from: 0; to: 50; stepSize: 0.1", qml)
        self.assertNotIn("grabBarHitScale", qml)
        self.assertNotIn("grabBarHitWidth", qml)
        for setter in ("setGrabBarPreview", "setGrabBarPreviewAlways", "setGrabBarPreviewColor",
                       "setGrabBarOutlineColor", "setGrabBarPreviewOpacity", "setGrabBarOutlineOpacity",
                       "setGrabBarOutlineWidth"):
            self.assertIn(f"backend.{setter}(", qml)

    def test_axes_are_independent_in_centimetres(self):
        self.b.setGrabBarPaddingY(50)
        self.b.setGrabBarPaddingX(2.5)
        self.assertEqual(self.b.grabBarPaddingY, 50)
        self.assertEqual(self.b.grabBarPaddingX, 2.5)
        self.assertEqual(self.asked[-1], "handsetting GRAB_BAR_PADDING_X 2.5")

    def test_preview_switches_and_style_are_saved_and_sent(self):
        for prop, setter, key, value in (
            ("grabBarPreview", "setGrabBarPreview", "GRAB_BAR_PREVIEW", True),
            ("grabBarPreviewAlways", "setGrabBarPreviewAlways", "GRAB_BAR_PREVIEW_ALWAYS", True),
            ("grabBarPreviewOpacity", "setGrabBarPreviewOpacity", "GRAB_BAR_PREVIEW_OPACITY", 0.2),
            ("grabBarOutlineOpacity", "setGrabBarOutlineOpacity", "GRAB_BAR_OUTLINE_OPACITY", 0.8),
            ("grabBarOutlineWidth", "setGrabBarOutlineWidth", "GRAB_BAR_OUTLINE_WIDTH", 3),
        ):
            getattr(self.b, setter)(value)
            self.assertEqual(getattr(self.b, prop), value)
            self.assertEqual(self.asked[-1], f"handsetting {key} {value:g}")
        self.b.setGrabBarPreview(False)
        self.assertFalse(self.b.grabBarPreview)
        # Disabling the preview doesn't erase the user's style or always-visible choice.
        self.assertTrue(self.b.grabBarPreviewAlways)
        self.assertEqual(self.b.grabBarOutlineWidth, 3)

    def test_colors_validate_without_allowing_extra_commands(self):
        for setter, prop, key in ((self.b.setGrabBarPreviewColor, "grabBarPreviewColor", "GRAB_BAR_PREVIEW_COLOR"),
                                  (self.b.setGrabBarOutlineColor, "grabBarOutlineColor", "GRAB_BAR_OUTLINE_COLOR")):
            setter("#AABBCC")
            self.assertEqual(getattr(self.b, prop), "#aabbcc")
            self.assertEqual(self.asked[-1], f"handsetting {key} #aabbcc")
            count = len(self.asked)
            for bad in ("red", "#123", "#11223344", "#12345z", "#112233\nclick native"):
                setter(bad)
                self.assertEqual(getattr(self.b, prop), "#aabbcc")
            self.assertEqual(len(self.asked), count)

    def test_preview_style_bounds(self):
        self.b.setGrabBarPreviewOpacity(-1)
        self.b.setGrabBarOutlineOpacity(2)
        self.b.setGrabBarOutlineWidth(30)
        self.assertEqual(self.b.grabBarPreviewOpacity, 0)
        self.assertEqual(self.b.grabBarOutlineOpacity, 1)
        self.assertEqual(self.b.grabBarOutlineWidth, 8)

    def test_qml_page_loads_with_color_dialogs(self):
        engine = QQmlApplicationEngine()
        engine.rootContext().setContextProperty("backend", self.b)
        engine.rootContext().setContextProperty("startPage", "visibility")
        engine.load(QUrl.fromLocalFile(str(Path(fds.HERE) / "main.qml")))
        self.assertTrue(engine.rootObjects(), "Display Settings QML failed to load")
        for root in engine.rootObjects():
            root.close()
        engine.deleteLater()
        self.app.processEvents()

    def test_resize_corner_defaults_and_live_settings(self):
        self.assertEqual(self.b.resizeCornerStyle, "native")
        self.assertEqual(self.b.resizeCornerColor, "#f6f9ff")
        self.assertEqual(self.b.resizeCornerLength, 4.7)
        self.assertEqual(self.b.resizeCornerWidth, 5.1)
        self.assertEqual(self.b.resizeCornerOpacity, 0.27)
        self.assertEqual(self.b.resizeCornerGap, 3.1)
        self.assertEqual(self.b.resizeCornerPadding, 2.4)
        self.assertEqual(self.b.resizeCornerExclusion, 0.5)
        self.assertEqual(self.b.resizeCornerReach, 20)
        self.assertEqual(self.b.resizeCornerRoundness, 100)
        for prop, setter, key, value in (
            ("resizeCornerStyle", "setResizeCornerStyle", "RESIZE_CORNER_STYLE", "glow"),
            ("resizeCornerColor", "setResizeCornerColor", "RESIZE_CORNER_COLOR", "#abcdef"),
            ("resizeCornerLength", "setResizeCornerLength", "RESIZE_CORNER_LENGTH", 5.5),
            ("resizeCornerWidth", "setResizeCornerWidth", "RESIZE_CORNER_WIDTH", 1.5),
            ("resizeCornerOpacity", "setResizeCornerOpacity", "RESIZE_CORNER_OPACITY", 0.8),
            ("resizeCornerGap", "setResizeCornerGap", "RESIZE_CORNER_GAP", 3.5),
            ("resizeCornerPadding", "setResizeCornerPadding", "RESIZE_CORNER_PADDING", 1.5),
            ("resizeCornerExclusion", "setResizeCornerExclusion", "RESIZE_CORNER_EXCLUSION", 0.7),
            ("resizeCornerReach", "setResizeCornerReach", "RESIZE_CORNER_REACH", 25.5),
            ("resizeCornerHoverColor", "setResizeCornerHoverColor", "RESIZE_CORNER_HOVER_COLOR", "#00ffff"),
            ("resizeCornerRoundness", "setResizeCornerRoundness", "RESIZE_CORNER_ROUNDNESS", 50.5),
            ("resizeCornerStyle", "setResizeCornerStyle", "RESIZE_CORNER_STYLE", "native"),
        ):
            getattr(self.b, setter)(value)
            self.assertEqual(getattr(self.b, prop), value)
            self.assertEqual(self.asked[-1], f"handsetting {key} {value}")
            self.assertIn(f"backend.{setter}(", (Path(fds.HERE) / "main.qml").read_text())

    def test_resize_corner_validation(self):
        self.b.setResizeCornerStyle("glow\nclick native")
        self.b.setResizeCornerColor("#123456\nclick native")
        self.assertEqual(self.asked, [])
        for setter, prop, value, want in (
            ("setResizeCornerLength", "resizeCornerLength", -1, 1),
            ("setResizeCornerLength", "resizeCornerLength", 20, 10),
            ("setResizeCornerWidth", "resizeCornerWidth", 0, 0.5),
            ("setResizeCornerWidth", "resizeCornerWidth", 20, 6),
            ("setResizeCornerOpacity", "resizeCornerOpacity", 2, 1),
            ("setResizeCornerWidth", "resizeCornerWidth", float("nan"), 5.1),
            ("setResizeCornerGap", "resizeCornerGap", -1, 0),
            ("setResizeCornerGap", "resizeCornerGap", 50, 10),
            ("setResizeCornerPadding", "resizeCornerPadding", 0, 0.1),
            ("setResizeCornerExclusion", "resizeCornerExclusion", -1, 0),
            ("setResizeCornerExclusion", "resizeCornerExclusion", 20, 5),
            ("setResizeCornerReach", "resizeCornerReach", 100, 50),
            ("setResizeCornerPadding", "resizeCornerPadding", float("inf"), 2.4),
            ("setResizeCornerRoundness", "resizeCornerRoundness", -1, 0),
            ("setResizeCornerRoundness", "resizeCornerRoundness", 120, 100),
        ):
            getattr(self.b, setter)(value)
            self.assertEqual(getattr(self.b, prop), want)
        Path(fds.CONF_PATH).write_text("RESIZE_CORNER_STYLE=junk\nRESIZE_CORNER_WIDTH=nan\n")
        self.assertEqual(self.b.resizeCornerStyle, "native")
        self.assertEqual(self.b.resizeCornerWidth, 5.1)

    def test_resize_corner_preview_and_all_persistent_combinations(self):
        self.assertFalse(self.b.resizeCornerPreview)
        self.assertEqual(self.b.resizeCornerAlways, [False, False, False, True])
        self.b.setResizeCornerPreview(True)
        self.assertTrue(self.b.resizeCornerPreview)
        self.assertEqual(self.asked[-1], "handsetting RESIZE_CORNER_PREVIEW 1")
        for mask in range(16):
            for corner, key in enumerate(("TL", "TR", "BL", "BR")):
                on = bool(mask & (1 << corner))
                self.b.setResizeCornerAlways(corner, on)
                self.assertEqual(self.asked[-1], f"handsetting RESIZE_CORNER_ALWAYS_{key} {int(on)}")
            self.assertEqual(self.b.resizeCornerAlways, [bool(mask & (1 << c)) for c in range(4)])
        count = len(self.asked)
        for bad in (-1, 4, 100):
            self.b.setResizeCornerAlways(bad, True)
        self.assertEqual(len(self.asked), count)
        # Hiding diagnostics must not erase persistent-corner choices or a saved thickness.
        self.b.setResizeCornerWidth(4)
        self.b.setResizeCornerPreview(False)
        self.assertEqual(self.b.resizeCornerAlways, [True] * 4)
        self.assertEqual(self.b.resizeCornerWidth, 4)

    def test_old_square_size_is_not_misread_as_shape_padding(self):
        Path(fds.CONF_PATH).write_text("RESIZE_CORNER_TARGET=50\n")
        self.assertEqual(self.b.resizeCornerPadding, 2.4)
        self.assertEqual(self.b.resizeCornerExclusion, 0.5)

    def test_legacy_selection_preserves_modern_preferences_and_haptics(self):
        self.assertTrue(self.b.resizeCornerHaptics)
        self.b.setResizeCornerWidth(4)
        self.b.setResizeCornerPreview(True)
        self.b.setResizeCornerAlways(0, True)
        self.b.setResizeCornerStyle("glow")
        self.assertEqual(self.b.resizeCornerStyle, "glow")
        self.assertEqual(self.b.resizeCornerWidth, 4)
        self.assertTrue(self.b.resizeCornerPreview)
        self.assertEqual(self.b.resizeCornerAlways, [True, False, False, True])
        self.b.setResizeCornerHaptics(False)
        self.assertEqual(self.asked[-1], "handsetting RESIZE_CORNER_HAPTICS 0")
        self.b.setResizeCornerStyle("native")
        self.assertFalse(self.b.resizeCornerHaptics)
        self.assertEqual(self.b.resizeCornerWidth, 4)
        self.b.setResizeCornerHaptics(True)
        self.assertEqual(self.asked[-1], "handsetting RESIZE_CORNER_HAPTICS 1")

    def test_example_config_matches_new_resize_defaults(self):
        props = ("resizeCornerStyle", "resizeCornerLength", "resizeCornerWidth", "resizeCornerOpacity",
                 "resizeCornerGap", "resizeCornerPadding", "resizeCornerExclusion", "resizeCornerRoundness",
                 "resizeCornerReach", "resizeCornerPreview", "resizeCornerAlways", "resizeCornerHaptics")
        defaults = {prop: getattr(self.b, prop) for prop in props}
        example = fds.ft_conf.read(str(Path(fds.HERE).parent / "session" / "frametop.conf.example"))
        with patch.object(fds.ft_layout, "read_conf", return_value=example):
            self.assertEqual({prop: getattr(self.b, prop) for prop in props}, defaults)


if __name__ == "__main__":
    unittest.main()
