"""Per-app native dashboard routing, without launching windows or touching Steam."""
import importlib.machinery
import importlib.util
import json
from pathlib import Path
import unittest
from unittest.mock import Mock, patch

loader = importlib.machinery.SourceFileLoader("dashroute", str(Path(__file__).resolve().parents[1] / "ft-dashroute"))
spec = importlib.util.spec_from_loader(loader.name, loader)
route = importlib.util.module_from_spec(spec)
loader.exec_module(route)


class DashboardRoute(unittest.TestCase):
    def test_exact_executable_not_arguments(self):
        with patch.object(route.ft_layout, "read_conf", return_value={"DASHBOARD_NATIVE_APPS": "konsole"}):
            for text in ("konsole", "/usr/bin/konsole --separate", '"/usr/bin/konsole" -e bash',
                         "env QT_QPA_PLATFORM=wayland konsole", "env -- /usr/bin/konsole"):
                self.assertTrue(route.native_app(text), text)
            for text in ("", "'unclosed", "konsole-other", "firefox konsole", "echo /usr/bin/konsole",
                         "sh -c konsole", "env", "env SOME_VALUE=konsole firefox"):
                self.assertFalse(route.native_app(text), text)

    def test_no_default_exclusions_and_multiple_names(self):
        with patch.object(route.ft_layout, "read_conf", return_value={}):
            self.assertFalse(route.native_app("konsole"))
        with patch.object(route.ft_layout, "read_conf", return_value={"DASHBOARD_NATIVE_APPS": "konsole kate"}):
            self.assertTrue(route.native_app("/usr/bin/kate"))

    def test_native_launch_bypasses_floatd(self):
        cdp = Mock()
        text = 'konsole --separate --workdir "/home/steamos/My Files"'
        with patch.object(route.ft_layout, "read_conf", return_value={"DASHBOARD_NATIVE_APPS": "konsole"}), \
                patch.object(route, "ask_floatd") as ask:
            route.route(cdp, text)
        ask.assert_not_called()
        cdp.evaluate.assert_called_once_with(f"window.__ftDashRoute.original.call(SteamClient.Apps, {json.dumps(text)})")

    def test_other_apps_keep_frametop_routing(self):
        cdp = Mock()
        with patch.object(route.ft_layout, "read_conf", return_value={"DASHBOARD_NATIVE_APPS": "konsole"}), \
                patch.object(route, "ask_floatd", return_value="ok") as ask:
            route.route(cdp, "firefox")
        ask.assert_called_once_with("firefox")
        cdp.evaluate.assert_not_called()


if __name__ == "__main__":
    unittest.main()
