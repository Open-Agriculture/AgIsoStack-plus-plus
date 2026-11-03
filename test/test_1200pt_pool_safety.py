import re
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
POOL_XML = REPOSITORY_ROOT / "object_pool"
POOL_XML_COPY = REPOSITORY_ROOT / "ISO1200PT"
POOL_IOP = REPOSITORY_ROOT / "examples/1200PT/src/object_pool/object_pool.iop"
POOL_HEADER = REPOSITORY_ROOT / "examples/1200PT/include/objectPoolObjects.h"
MAIN_CPP = (REPOSITORY_ROOT / "examples/1200PT/src/main.cpp").read_text()
FOLD_CPP = (REPOSITORY_ROOT / "examples/1200PT/src/fold_sequence.cpp").read_text()
LOCKOUT_HEADER = (REPOSITORY_ROOT / "examples/1200PT/include/lockout_feedback.h").read_text()


class PoolSafetyTests(unittest.TestCase):
    def test_pool_version_is_derived_from_embedded_bytes(self):
        self.assertIn(
            "hash_object_pool_to_version(\n"
            "        object_pool_start,\n"
            "        objectPoolSize",
            MAIN_CPP,
        )
        self.assertRegex(
            MAIN_CPP,
            r"set_object_pool\(\s*0,\s*object_pool_start,\s*"
            r"static_cast<std::uint32_t>\(objectPoolSize\),\s*objectPoolVersion\s*\)",
        )
        self.assertIn(
            '"Embedded VT object pool: version=%s, size=%u bytes"',
            MAIN_CPP,
        )
        self.assertNotRegex(MAIN_CPP, r'set_object_pool\([\s\S]*?"1200"')

    def test_iop_has_six_individual_actions_per_direction(self):
        root = ET.parse(POOL_XML).getroot()
        names = {element.get("name") for element in root.iter()}
        expected = {
            f"Btn_{direction}Action{index}"
            for direction in ("Fold", "Unfold")
            for index in range(1, 7)
        }
        self.assertTrue(expected.issubset(names))
        iop = POOL_IOP.read_bytes()
        for text in (b"ROTATE BAR", b"SET ROTATE LOCK", b"RETRACT TONGUE", b"SET TONGUE LOCK",
                     b"INNER MARKER", b"BAR RAISE", b"EXTEND TONGUE", b"FORWARD", b"LIMITED",
                     b"FLOAT", b"FULL"):
            self.assertIn(text, iop)

    def test_each_action_clears_outputs_before_energizing(self):
        for direction in ("fold", "unfold"):
            match = re.search(
                rf"esp_err_t fold_sequence_activate_{direction}_action\([^)]*\)\s*\{{(.*?)\n\}}",
                FOLD_CPP,
                re.DOTALL,
            )
            self.assertIsNotNone(match)
            action_body = match.group(1)
            self.assertLess(
                action_body.index("fold_sequence_all_off();"),
                action_body.index("apply_solenoid_states("),
            )

    def test_fan_vac_faults_and_power_states_cannot_gate_hydraulics(self):
        self.assertIn(
            "const bool hydraulicControlAvailable = !mcp23017_output_health().has_fault();",
            MAIN_CPP,
        )
        self.assertNotRegex(
            MAIN_CPP,
            r"hydraulicControlAvailable\s*=\s*[^;]*(?:fan_vac_get_state|FanVacState|fanFault|vacFault)",
        )
        self.assertNotRegex(LOCKOUT_HEADER, r"FanVacState|FanVacStatus|fanFault|vacFault")

    def test_board_fault_inhibit_returns_before_hydraulic_actions(self):
        for group in ("Unfold", "Fold"):
            self.assertIn(
                "if (hydraulic_actuation_inhibited())\n"
                "    {\n"
                f"        show_blocked_press(ui::ActionGroup::{group});\n"
                "        return;\n"
                "    }\n",
                MAIN_CPP,
            )
        self.assertEqual(MAIN_CPP.count("if (hydraulic_actuation_inhibited())"), 2)
        for direction in ("unfold", "fold"):
            body = MAIN_CPP.split(f"void handle_{direction}_action_button(uint8_t actionIndex)\n{{\n", 1)[1].split("\n}\n", 1)[0]
            blocked_return = body.index("return;", body.index("if (hydraulic_actuation_inhibited())"))
            for action in ("is_action_group_active(", "is_action_lit(", "fold_sequence_", "press_action_visual("):
                self.assertGreater(body.index(action), blocked_return, action)
        self.assertIn(
            "hydraulicActuationInhibited.exchange(!hydraulicControlAvailable)",
            MAIN_CPP,
        )
        self.assertIn(
            "if (!hydraulicControlAvailable && !wasActuationInhibited)\n"
            "        {\n"
            "            fold_sequence_cancel();",
            MAIN_CPP,
        )
        self.assertIn("fold_sequence_all_off();", FOLD_CPP)

    def test_periodic_control_does_not_run_on_timer_service_task(self):
        self.assertNotIn("xTimerCreate", MAIN_CPP)
        self.assertIn("fan_vac_update();\n", MAIN_CPP)
        self.assertIn("update_display_values();\n", MAIN_CPP)
        self.assertIn("vTaskDelay(pdMS_TO_TICKS(PID_UPDATE_INTERVAL));", MAIN_CPP)

    def test_calibration_screen_has_four_compact_coloured_set_rows(self):
        self.assertEqual(POOL_XML.read_bytes(), POOL_XML_COPY.read_bytes())
        root = ET.parse(POOL_XML).getroot()
        objects = {
            element.get("name"): element
            for element in root.iter()
            if element.tag != "include_object"
        }
        fonts = {element.get("name"): element for element in root.findall("fontattributes")}
        rows = (
            ("FullRaise", "FULL RAISE", "black", 150, "Btn_CalSetFullRaise", "95"),
            ("LimitedRaise", "LIMITED RAISE", "green", 190, "Btn_CalSetLimitedRaise", "75"),
            ("StartPlant", "START PLANT", "teal", 230, "Btn_CalSetStartPlant", "15"),
            ("StopPlant", "STOP PLANT", "olive", 270, "Btn_CalSetStopPlant", "0"),
        )
        for suffix, label, colour, y, button_name, default_value in rows:
            label_object = objects[f"Label_Cal{suffix}"]
            self.assertEqual(label_object.get("value"), label)
            self.assertEqual(fonts[label_object.get("font_attributes")].get("font_colour"), colour)
            self.assertEqual(int(label_object.get("y")), y)

            number = objects[f"Num_Cal{suffix}"]
            self.assertEqual(number.get("value"), default_value)
            self.assertEqual(number.get("max_value"), "100")
            self.assertEqual(int(number.get("y")), y)

            button = objects[button_name]
            self.assertEqual(button.get("background_colour"), "green")
            self.assertEqual(
                (int(button.get("x")), int(button.get("y")), int(button.get("width")), int(button.get("height"))),
                (340, y, 110, 30),
            )
            button_label = objects[button[0].get("name")]
            self.assertEqual(button_label.get("value"), "SET")

        self.assertEqual(objects["Label_FramePos"].get("value"), "FRAME POSITION:")
        self.assertEqual(objects["Num_CalPosition"].get("x"), "214")
        self.assertEqual(objects["Num_CalPosition"].get("y"), "101")
        self.assertEqual(objects["Divider_Cal2"].get("y"), "352")
        self.assertEqual(objects["Str_CalFaultMessage"].get("y"), "378")

        jog_buttons = {
            name: (element.get("x"), element.get("y"), element.get("width"), element.get("height"))
            for name, element in objects.items()
            if name in {"Btn_PlantLower", "Btn_PlantLimitedLift", "Btn_PlantFullLift"}
        }
        self.assertEqual(
            jog_buttons,
            {
                "Btn_PlantLower": ("15", "390", "150", "65"),
                "Btn_PlantLimitedLift": ("165", "390", "150", "65"),
                "Btn_PlantFullLift": ("315", "390", "150", "65"),
            },
        )
        self.assertFalse(
            any(name and ("Zero" in name or "SensorRaw" in name) for name in objects)
        )
        self.assertNotIn(b"ZERO SENSOR", POOL_IOP.read_bytes())

    def test_four_calibration_handlers_persist_and_validate_plant_order(self):
        header = POOL_HEADER.read_text()
        for button, point in (
            ("FullRaise", "FullRaise"),
            ("LimitedRaise", "LimitedRaise"),
            ("StartPlant", "StartPlant"),
            ("StopPlant", "StopPlant"),
        ):
            self.assertIn(f"Button_CalSet{button}", header)
            self.assertIn(
                f"save_calibration_point(\n                calibration::Point::{point},",
                MAIN_CPP,
            )

        start_handler = MAIN_CPP.split("case Button_CalSetStartPlant:", 1)[1].split("case Button_CalSetStopPlant:", 1)[0]
        stop_handler = MAIN_CPP.split("case Button_CalSetStopPlant:", 1)[1].split("default:", 1)[0]
        self.assertIn("calibration::Point::StartPlant", start_handler)
        self.assertIn("calibration::Point::StopPlant", stop_handler)
        self.assertIn("calibration::load_points", MAIN_CPP)
        self.assertIn("calibration::save_point", MAIN_CPP)
        self.assertIn("nvs_get_u8", MAIN_CPP)
        self.assertIn("nvs_set_u8", MAIN_CPP)
        self.assertIn("nvs_commit", MAIN_CPP)
        self.assertIn("const uint32_t position = 15 + ((step * 80) / FOLD_STEPS);", MAIN_CPP)
        self.assertIn("const uint32_t position = 95 - ((step * 80) / UNFOLD_STEPS);", MAIN_CPP)
        self.assertIn("if (position + 5 >= fullRaise)", MAIN_CPP)
        self.assertNotIn("plant_control_", start_handler + stop_handler)
        for removed_symbol in ("Button_CalZero", "Button_CalSetTransport", "Button_CalSetPlant"):
            self.assertNotIn(removed_symbol, MAIN_CPP)

    def test_marker_and_headland_are_not_wired_to_outputs(self):
        marker = MAIN_CPP.split("static void handle_marker_side_button", 1)[1].split("static void toggle_bulk_fill_headland", 1)[0]
        for forbidden in ("fold_sequence_", "plant_control_", "fan_vac_", "gpio_set_level"):
            self.assertNotIn(forbidden, marker)
        headland = MAIN_CPP.split("static void toggle_bulk_fill_headland", 1)[1].split("bool bulk_fill_headland_enabled()", 1)[0]
        for forbidden in ("fold_sequence_", "plant_control_", "fan_vac_", "gpio_set_level"):
            self.assertNotIn(forbidden, headland)
        self.assertEqual(MAIN_CPP.count("bulk_fill_headland_enabled()"), 2)  # declaration + definition only

    def test_power_buttons_use_existing_toggle_path_with_explicit_state(self):
        self.assertIn("ui::set_power_state(requestedOn, fan_vac_is_vac_on, fan_vac_toggle_vac_power);", MAIN_CPP)
        self.assertIn("ui::set_power_state(requestedOn, fan_vac_is_fan_on, fan_vac_toggle_fan_power);", MAIN_CPP)
        self.assertNotIn("fan_vac_toggle_vac_power();", MAIN_CPP)
        self.assertNotIn("fan_vac_toggle_fan_power();", MAIN_CPP)

    def test_action_button_visuals_are_only_changed_from_stored_selection(self):
        display = MAIN_CPP.split("void update_display_values()", 1)[1].split("// ─── BUTTON VISUALS", 1)[0]
        for forbidden in ("send_change_background_colour", "send_change_attribute", "apply_button_style", "FoldAction", "UnfoldAction"):
            self.assertNotIn(forbidden, display)
        loop = MAIN_CPP.split("// ── MAIN LOOP", 1)[1]
        self.assertEqual(loop.count("reset_action_buttons();"), 1)  # hydraulic inhibit only
        self.assertIn("actionSelection.styles(group, styles);", MAIN_CPP)
        press = MAIN_CPP.split("static void press_action_visual", 1)[1].split("static bool is_action_lit", 1)[0]
        self.assertIn("sync_action_buttons(group);", press)
        self.assertNotIn("fold_sequence_", press)

    def test_hydraulic_call_order_in_action_handlers_is_unchanged(self):
        for direction in ("unfold", "fold"):
            Direction = direction.capitalize()
            body = MAIN_CPP.split(f"void handle_{direction}_action_button(uint8_t actionIndex)\n{{\n", 1)[1].split("\n}\n", 1)[0]
            self.assertLess(
                body.index(f"fold_sequence_activate_{direction}_action(actionIndex)"),
                body.index(f"press_action_visual(ui::ActionGroup::{Direction}, actionIndex);",
                           body.index(f"fold_sequence_activate_{direction}_action")),
            )


if __name__ == "__main__":
    unittest.main()
