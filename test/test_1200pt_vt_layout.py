import importlib.util
import struct
import unittest
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
_spec = importlib.util.spec_from_file_location(
    "render_vt_preview", REPOSITORY_ROOT / "scripts" / "render_vt_preview.py"
)
preview = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(preview)

OBJECTS, FONTS = preview.load_pool()

MAIN_CPP = (REPOSITORY_ROOT / "examples/1200PT/src/main.cpp").read_text()
UI_STATE_H = (REPOSITORY_ROOT / "examples/1200PT/include/ui_state.h").read_text()
POOL_HEADER = (REPOSITORY_ROOT / "examples/1200PT/include/objectPoolObjects.h").read_text()

# Strings written at runtime by examples/1200PT/src/main.cpp and fold_sequence.cpp.
RUNTIME_STRINGS = {
    "Str_CalPositionState": ["MID RANGE", "TRANSPORT RANGE", "PLANT RANGE"],
    "Str_CalFaultMessage": ["STATUS: OK - NO ACTIVE FAULTS", "CALIBRATION SAVE FAILED",
                            "START PLANT MUST BE >= STOP PLANT", "STOP PLANT MUST BE <= START PLANT"],
    "Str_MarkerDetail": ["OFF", "LEFT", "RIGHT", "LEFT + RIGHT", "AUTO"],
    "Str_FoldInstruction": ["SEQUENCE COMPLETE", "SELECT FOLD OR UNFOLD", "HYDRAULICS SAFE - IDLE",
                            "LOCKED: OUTPUT BOARD MISSING"],
    "Str_UnfoldInstruction": ["SEQUENCE COMPLETE", "SELECT FOLD OR UNFOLD", "HYDRAULICS SAFE - IDLE",
                              "LOCKED: OUTPUT BOARD MISSING"],
    "Str_FoldLockReason": [" ", "REASON: OUTPUT BOARD"],
    "Str_UnfoldLockReason": [" ", "REASON: OUTPUT BOARD"],
    "Str_CalBulkHeadland": ["ON", "OFF"],
}

PALETTE_INDEX = {"black": 0, "white": 1, "green": 2, "silver": 7, "red": 12}
# (button, label, dedicated font, default colour state) for every stateful button
STATEFUL_BUTTONS = [
    ("Btn_VacPower", "Label_BtnVacPower", "BtnFont_VacOn", "green"),
    ("Btn_VacPowerOff", "Label_BtnVacPowerOff", "BtnFont_VacOff", "inactive"),
    ("Btn_FanPower", "Label_BtnFanPower", "BtnFont_FanOn", "green"),
    ("Btn_FanPowerOff", "Label_BtnFanPowerOff", "BtnFont_FanOff", "inactive"),
    ("Btn_MarkerPrev", "Label_BtnPrev", "BtnFont_MarkerLeft", "inactive"),
    ("Btn_MarkerToggle", "Str_MarkerStatus", "BtnFont_MarkerAuto", "inactive"),
    ("Btn_MarkerNext", "Label_BtnNext", "BtnFont_MarkerRight", "inactive"),
    ("Btn_CalBulkHeadland", "Str_CalBulkHeadland", "BtnFont_CalBulkHeadland", "inactive"),
] + [
    (f"Btn_{d}Action{i}", f"Str_{d}Action{i}", f"BtnFont_{d}Action{i}", "inactive")
    for d in ("Unfold", "Fold") for i in range(1, 7)
]
UNFOLD_ROWS = [("ROTATE BAR", "BACK"), ("SET ROTATE LOCK", "BACK"), ("RETRACT TONGUE", "FORWARD"),
               ("SET TONGUE LOCK", "FORWARD"), ("INNER MARKER", "FORWARD"), ("BAR RAISE", "LIMITED")]
FOLD_ROWS = [("BAR RAISE", "FULL"), ("INNER MARKER", "BACK"), ("EXTEND TONGUE", "BACK"),
             ("SET TONGUE LOCK", "FLOAT"), ("ROTATE BAR", "FORWARD"), ("SET ROTATE LOCK", "FORWARD")]

NEW_FONTS = {
    "Fit8x12_black": ("23100", "8x12", "black"),
    "Fit8x12_white": ("23101", "8x12", "white"),
    "Fit8x12_green": ("23102", "8x12", "green"),
    "Fit8x12_teal": ("23103", "8x12", "teal"),
    "Fit8x12_olive": ("23104", "8x12", "olive"),
    "Fit12x16_black": ("23105", "12x16", "black"),
    "Fit12x16_white": ("23106", "12x16", "white"),
    "Fit12x16_green": ("23107", "12x16", "green"),
    "Fit12x16_teal": ("23108", "12x16", "teal"),
    "Fit12x16_olive": ("23109", "12x16", "olive"),
}

TABS = [("Run", 16), ("Unfold", 130), ("Fold", 244), ("Cal", 358)]


def overlaps(a, b):
    return a[0] < b[0] + b[2] and b[0] < a[0] + a[2] and a[1] < b[1] + b[3] and b[1] < a[1] + a[3]


def geometry(name):
    return preview.rect(OBJECTS[name])


def output_objects_in_masks():
    """Yield (mask, element, absolute rect) for every output object drawn in a mask."""
    for mask in preview.MASKS.values():
        for element, box in preview.mask_items(OBJECTS, mask):
            if element.tag == "button":
                for child, child_box in preview.button_children(OBJECTS, element):
                    yield mask, child, child_box
            else:
                yield mask, element, box


class VtLayoutTests(unittest.TestCase):
    def test_fit_fonts_exist_with_specified_ids(self):
        for name, (font_id, size, colour) in NEW_FONTS.items():
            font = FONTS[name]
            self.assertEqual(
                (font.get("id"), font.get("font_size"), font.get("font_colour"),
                 font.get("font_type"), font.get("font_style")),
                (font_id, size, colour, "latin1", "normal"),
            )

    def test_included_outputs_use_only_fit_fonts(self):
        button_fonts = {font for _, _, font, _ in STATEFUL_BUTTONS}
        for mask, element, _ in output_objects_in_masks():
            font = element.get("font_attributes")
            self.assertTrue(font in NEW_FONTS or font in button_fonts, f"{mask}:{element.get('name')}")
            if font in button_fonts:
                size = FONTS[font].get("font_size")
                if size == "24x32":
                    # the only allowed legacy 24x32 font: the marker "<" and ">"
                    self.assertIn(element.get("value"), ("<", ">"), element.get("name"))
                else:
                    self.assertEqual(size, "12x16", f"{mask}:{element.get('name')}")

    def test_text_fits_object_width_and_height(self):
        for mask, element, box in output_objects_in_masks():
            font = FONTS[element.get("font_attributes")]
            char_w, char_h = preview.font_cell(font)
            name = element.get("name")
            if element.tag == "outputstring":
                texts = [element.get("value")] + RUNTIME_STRINGS.get(name, [])
            else:
                texts = [str(max(int(element.get("max_value")), int(element.get("value"))))]
            for text in texts:
                self.assertLessEqual(len(text) * char_w, box[2], f"{mask}:{name}:{text!r}")
            if not name.startswith("Divider_"):
                self.assertLessEqual(char_h, box[3], f"{mask}:{name}")

    def test_no_overlaps_within_each_data_mask(self):
        for mask in preview.MASKS.values():
            items = preview.mask_items(OBJECTS, mask)
            for i, (a, box_a) in enumerate(items):
                for b, box_b in items[i + 1:]:
                    self.assertFalse(
                        overlaps(box_a, box_b),
                        f"{mask}: {a.get('name')} {box_a} overlaps {b.get('name')} {box_b}",
                    )
                self.assertTrue(0 <= box_a[0] and box_a[0] + box_a[2] <= 480, a.get("name"))
                self.assertTrue(0 <= box_a[1] and box_a[1] + box_a[3] <= 480, a.get("name"))

    def test_button_children_fill_their_button(self):
        for mask in preview.MASKS.values():
            for element, box in preview.mask_items(OBJECTS, mask):
                if element.tag != "button":
                    continue
                children = preview.button_children(OBJECTS, element)
                self.assertEqual(len(children), 1, element.get("name"))
                self.assertEqual(children[0][1], box, element.get("name"))

    def test_tabs_and_dividers(self):
        for name, x in TABS:
            for suffix in ("", "_Active"):
                self.assertEqual(geometry(f"Btn_Tab{name}{suffix}"), (x, 16, 106, 44))
                label = OBJECTS[f"Label_Tab{name}{suffix}"]
                self.assertEqual(preview.rect(label), (0, 0, 106, 44))
                self.assertEqual(label.get("horizontal_justification"), "centred")
                self.assertEqual(label.get("vertical_justification"), "middle")
        self.assertEqual(geometry("Divider_Top"), (0, 68, 480, 3))
        self.assertEqual(geometry("Divider_Fold"), (0, 404, 480, 3))
        self.assertEqual(geometry("Divider_Cal1"), (0, 140, 480, 3))
        self.assertEqual(geometry("Divider_Cal2"), (0, 352, 480, 3))

    def test_run_layout(self):
        sections = (
            (81, "VacSection", "Btn_VacPower", "Btn_VacPressureDown", "Num_VacTarget", "Label_VacUnit",
             "Btn_VacPressureUp", "Label_VacSetpoint", "VACUUM RATE", "Label_VacActual", "VACUUM ACTUAL",
             "Num_VacActual", "Label_VacActualUnit"),
            (214, "FanSection", "Btn_FanPower", "Btn_FanSpeedDown", "Num_FanRPMTarget", "Label_FanUnit",
             "Btn_FanSpeedUp", "Label_FanSetpoint", "BULK FILL RATE", "Label_FanActual", "BULK FILL ACTUAL",
             "Num_FanRPMActual", "Label_FanActualUnit"),
        )
        for (y0, title, power, minus, target, unit, plus, rate, rate_text, actual, actual_text,
             actual_num, actual_unit) in sections:
            self.assertEqual(geometry(f"Label_{title}"), (20, y0, 211, 18))
            self.assertEqual(geometry(power), (20, y0 + 22, 102, 32))
            self.assertEqual(geometry(power + "Off"), (129, y0 + 22, 102, 32))
            self.assertEqual(preview.rect(OBJECTS[f"Label_Btn{title[:-7]}Power"]), (0, 0, 102, 32))
            self.assertEqual(preview.rect(OBJECTS[f"Label_Btn{title[:-7]}PowerOff"]), (0, 0, 102, 32))
            self.assertEqual(OBJECTS[f"Label_Btn{title[:-7]}Power"].get("value"), "ON")
            self.assertEqual(OBJECTS[f"Label_Btn{title[:-7]}PowerOff"].get("value"), "OFF")
            self.assertEqual(geometry(rate), (20, y0 + 64, 211, 16))
            self.assertEqual(OBJECTS[rate].get("value"), rate_text)
            self.assertEqual(geometry(minus), (20, y0 + 84, 30, 38))
            self.assertEqual(geometry(target), (54, y0 + 84, 80, 38))
            self.assertEqual(geometry(unit), (134, y0 + 84, 63, 38))
            self.assertEqual(geometry(plus), (201, y0 + 84, 30, 38))
            self.assertEqual(geometry(actual), (249, y0 + 64, 211, 16))
            self.assertEqual(OBJECTS[actual].get("value"), actual_text)
            self.assertEqual(geometry(actual_num), (249, y0 + 84, 100, 38))
            self.assertEqual(geometry(actual_unit), (349, y0 + 84, 111, 38))
        self.assertEqual(geometry("Btn_MarkerPrev"), (75, 412, 80, 42))
        self.assertEqual(geometry("Btn_MarkerToggle"), (165, 412, 150, 42))
        self.assertEqual(geometry("Btn_MarkerNext"), (325, 412, 80, 42))
        self.assertEqual(geometry("Str_MarkerDetail"), (20, 458, 440, 16))
        self.assertEqual(OBJECTS["Label_BtnPrev"].get("value"), "<")
        self.assertEqual(OBJECTS["Str_MarkerStatus"].get("value"), "MARKER")
        self.assertEqual(OBJECTS["Label_BtnNext"].get("value"), ">")
        self.assertEqual(OBJECTS["Str_MarkerStatus"].get("font_attributes"), "BtnFont_MarkerAuto")
        self.assertEqual(FONTS["BtnFont_MarkerAuto"].get("font_size"), "12x16")
        self.assertEqual(FONTS["BtnFont_MarkerLeft"].get("font_size"), "24x32")
        self.assertEqual(FONTS["BtnFont_MarkerRight"].get("font_size"), "24x32")
        included = {c.get("name") for c in OBJECTS["DataMask_Run"] if c.tag == "include_object"}
        for removed in ("Divider_Mid1", "Divider_Mid2", "Label_MarkerSection"):
            self.assertNotIn(removed, included)
            self.assertIn(removed, OBJECTS)

    def test_fold_and_unfold_layout(self):
        for direction in ("Fold", "Unfold"):
            self.assertEqual(geometry(f"Label_{direction}Title"), (20, 76, 200, 20))
            self.assertEqual(geometry(f"Str_{direction}Instruction"), (230, 78, 240, 16))
            for i in range(6):
                ry = 100 + 50 * i
                self.assertEqual(geometry(f"Label_{direction}Step{i + 1}"), (26, ry + 10, 320, 16))
                self.assertEqual(geometry(f"Btn_{direction}Action{i + 1}"), (354, ry + 8, 100, 34))
                self.assertEqual(geometry(f"Str_{direction}Action{i + 1}"), (0, 0, 100, 34))
            rows = UNFOLD_ROWS if direction == "Unfold" else FOLD_ROWS
            for i, (label, text) in enumerate(rows):
                self.assertEqual(OBJECTS[f"Label_{direction}Step{i + 1}"].get("value"), label)
                self.assertEqual(OBJECTS[f"Str_{direction}Action{i + 1}"].get("value"), text)
                self.assertEqual(FONTS[OBJECTS[f"Str_{direction}Action{i + 1}"].get("font_attributes")]
                                 .get("font_size"), "12x16")

    def test_cal_layout(self):
        self.assertEqual(geometry("Label_CalTitle"), (20, 79, 240, 20))
        self.assertEqual(geometry("Label_FramePos"), (20, 105, 190, 26))
        self.assertEqual(geometry("Num_CalPosition"), (214, 101, 70, 32))
        self.assertEqual(geometry("Label_CalPercent"), (288, 105, 24, 26))
        rows = (("FullRaise", "black", 150, "Label_CalFullRaisePercent"),
                ("LimitedRaise", "green", 190, "Label_CalLimitedPercent"),
                ("StartPlant", "teal", 230, "Label_CalStartPlantPercent"),
                ("StopPlant", "olive", 270, "Label_CalStopPlantPercent"))
        for key, colour, y, percent in rows:
            self.assertEqual(geometry(f"Label_Cal{key}"), (20, y, 170, 30))
            self.assertEqual(OBJECTS[f"Label_Cal{key}"].get("font_attributes"), f"Fit12x16_{colour}")
            self.assertEqual(geometry(f"Num_Cal{key}"), (200, y, 90, 30))
            self.assertEqual(geometry(percent), (294, y, 24, 30))
            self.assertEqual(geometry(f"Btn_CalSet{key}"), (340, y, 110, 30))
            self.assertEqual(geometry(f"Label_BtnSet{key}"), (0, 0, 110, 30))
        self.assertEqual(geometry("Label_CalBulkHeadland"), (20, 310, 220, 30))
        self.assertEqual(OBJECTS["Label_CalBulkHeadland"].get("value"), "BULK FILL HEADLAND")
        self.assertEqual(OBJECTS["Label_CalBulkHeadland"].get("font_attributes"), "Fit12x16_black")
        self.assertEqual(geometry("Btn_CalBulkHeadland"), (340, 310, 110, 30))
        self.assertEqual(geometry("Str_CalBulkHeadland"), (0, 0, 110, 30))
        self.assertEqual(OBJECTS["Str_CalBulkHeadland"].get("value"), "OFF")
        self.assertEqual(geometry("Label_Diagnostics"), (20, 358, 300, 18))
        self.assertEqual(geometry("Str_CalFaultMessage"), (20, 378, 440, 30))
        included = {c.get("name") for c in OBJECTS["DataMask_Cal"] if c.tag == "include_object"}
        for removed in ("Btn_PlantLower", "Btn_PlantLimitedLift", "Btn_PlantFullLift"):
            self.assertNotIn(removed, included)
            self.assertIn(removed, OBJECTS)
        self.assertEqual(geometry("Btn_PlantLower"), (15, 390, 150, 65))
        self.assertEqual(geometry("Btn_PlantLimitedLift"), (165, 390, 150, 65))
        self.assertEqual(geometry("Btn_PlantFullLift"), (315, 390, 150, 65))
        self.assertEqual(OBJECTS["Label_BtnLower"].get("font_attributes"), "Fit12x16_black")

    def test_stateful_buttons_have_dedicated_label_font_and_palette_defaults(self):
        used_fonts = []
        for button, label, font, default in STATEFUL_BUTTONS:
            element = OBJECTS[button]
            self.assertEqual(element.get("border_colour"), "black", button)
            children = [c.get("name") for c in element if c.tag == "include_object"]
            self.assertEqual(children, [label], button)
            self.assertEqual(OBJECTS[label].get("font_attributes"), font, button)
            used_fonts.append(font)
            background, text = {"green": ("green", "white"), "inactive": ("silver", "black")}[default]
            self.assertEqual(element.get("background_colour"), background, button)
            self.assertEqual(OBJECTS[label].get("background_colour"), background, button)
            self.assertEqual(FONTS[font].get("font_colour"), text, button)
        # fonts are never shared, so a colour change only affects its own button
        self.assertEqual(len(used_fonts), len(set(used_fonts)))
        users = [e.get("name") for e in OBJECTS.values()
                 if e.get("font_attributes") in used_fonts]
        self.assertEqual(len(users), len(used_fonts))

    def test_runtime_palette_constants_match_the_button_palette(self):
        for name, colour in (("SILVER", "silver"), ("BLACK", "black"), ("GREEN", "green"),
                             ("WHITE", "white"), ("RED", "red")):
            self.assertIn(f"COLOUR_{name} = {PALETTE_INDEX[colour]};", UI_STATE_H)
        self.assertIn("return { COLOUR_GREEN, COLOUR_WHITE };", UI_STATE_H)
        self.assertIn("return { COLOUR_RED, COLOUR_WHITE };", UI_STATE_H)
        self.assertIn("return { COLOUR_SILVER, COLOUR_BLACK };", UI_STATE_H)

    def test_every_stateful_button_has_a_handler_and_runtime_colour_sync(self):
        handler = MAIN_CPP.split("void handle_button_event(", 1)[1].split("// ─── MAIN ENTRY POINT", 1)[0]
        for case in ("Button_VacPower", "Button_VacPowerOff", "Button_FanPower", "Button_FanPowerOff",
                     "Button_MarkerPrev", "Button_MarkerToggle", "Button_MarkerNext",
                     "Button_CalBulkHeadland"):
            self.assertIn(f"case {case}:", handler)
        for i in range(1, 7):
            self.assertIn(f"case Button_UnfoldAction{i}:", handler)
            self.assertIn(f"case Button_FoldAction{i}:", handler)
        for visual in ("VAC_ON_VISUAL", "VAC_OFF_VISUAL", "FAN_ON_VISUAL", "FAN_OFF_VISUAL",
                       "MARKER_LEFT_VISUAL", "MARKER_AUTO_VISUAL", "MARKER_RIGHT_VISUAL", "HEADLAND_VISUAL"):
            self.assertIn(f"static const ButtonVisual {visual}", MAIN_CPP)
        # colour sync: button + label background, font colour, fallback command
        for call in ("send_change_background_colour(visual.button", "send_change_background_colour(visual.label",
                     "send_change_attribute(", "send_change_font_attributes("):
            self.assertIn(call, MAIN_CPP)
        # resync on boot/reconnect/pool upload (connected edge), screen shown, VT mask change
        self.assertIn("if (vtConnected && !wasVtConnected)", MAIN_CPP)
        self.assertEqual(MAIN_CPP.count("sync_screen_buttons(ActiveScreen::"), 8 + 4)
        self.assertIn("add_listener(handle_active_mask_event)", MAIN_CPP)

    def test_new_ids_are_in_the_object_pool_header(self):
        for name in ("Button_VacPowerOff", "Button_FanPowerOff", "Button_CalBulkHeadland",
                     "OutStr_BtnVacPowerOff", "OutStr_BtnFanPowerOff", "OutStr_CalBulkHeadland",
                     "VarStr_CalBulkHeadland", "FontAttr_BtnVacOn", "FontAttr_BtnCalBulkHeadland"):
            self.assertRegex(POOL_HEADER, rf"#define {name}\s+\d+")
        for name, button_id in (("Btn_VacPowerOff", "6053"), ("Btn_FanPowerOff", "6043"),
                                ("Btn_CalBulkHeadland", "6074")):
            self.assertEqual(OBJECTS[name].get("id"), button_id)
        for _, _, font, _ in STATEFUL_BUTTONS:
            self.assertIn(f'name="{font}"', (REPOSITORY_ROOT / "object_pool").read_text())

    def test_object_pool_copies_and_iop_are_in_sync(self):
        self.assertEqual((REPOSITORY_ROOT / "object_pool").read_bytes(),
                         (REPOSITORY_ROOT / "ISO1200PT").read_bytes())
        iop = (REPOSITORY_ROOT / "examples/1200PT/src/object_pool/object_pool.iop").read_bytes()
        for name, (font_id, size, _) in NEW_FONTS.items():
            self.assertIn(struct.pack("<HB", int(font_id), 23), iop, name)

    def test_previews_exist(self):
        for key in list(preview.MASKS) + list(preview.VARIANTS):
            data = (REPOSITORY_ROOT / "docs" / "vt_preview" / f"{key}.png").read_bytes()
            self.assertEqual(struct.unpack(">II", data[16:24]), (480, 480))


if __name__ == "__main__":
    unittest.main()
