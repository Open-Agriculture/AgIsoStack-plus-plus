#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/mcp2515_can_interface.hpp"
#include "isobus/hardware_integration/spi_interface_esp.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client_update_helper.hpp"
#include "isobus/utility/iop_file_interface.hpp"

#include "console_logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "objectPoolObjects.h"
#include "calibration_settings.h"
#include "ui_state.h"
#include "lockout_feedback.h"
#include "fold_sequence.h"
#include "plant_control.h"
#include "fan_vac_control.h"
#include "mcp23017_output.h"

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// ─── ISOBUS CLIENT INSTANCES ───────────────────────────────────────────
static std::shared_ptr<isobus::VirtualTerminalClient> virtualTerminalClient = nullptr;
static std::shared_ptr<isobus::VirtualTerminalClientUpdateHelper> virtualTerminalUpdateHelper = nullptr;
static std::shared_ptr<isobus::SPIInterfaceESP> spiInterface = nullptr;

// ─── MCP2515 SPI CAN PIN DEFINITIONS ───────────────────────────────────────
namespace
{
    constexpr gpio_num_t MCP2515_SPI_SCK = GPIO_NUM_18;
    constexpr gpio_num_t MCP2515_SPI_MOSI = GPIO_NUM_23;
    constexpr gpio_num_t MCP2515_SPI_MISO = GPIO_NUM_19;
    constexpr gpio_num_t MCP2515_SPI_CS = GPIO_NUM_5;
    constexpr gpio_num_t MCP2515_INT = GPIO_NUM_4;
    constexpr spi_host_device_t MCP2515_SPI_HOST = VSPI_HOST;

    // 250 kbit/s @ 8 MHz MCP2515 oscillator
    constexpr std::uint8_t MCP2515_250K_8MHZ_CNF1 = 0x80;
    constexpr std::uint8_t MCP2515_250K_8MHZ_CNF2 = 0xE5;
    constexpr std::uint8_t MCP2515_250K_8MHZ_CNF3 = 0x83;
}

// ─── SCREEN TRACKING ─────────────────────────────────────────────────────────────────
// Tracks which screen is currently active on InCommand
enum class ActiveScreen
{
    RUN,
    UNFOLD,
    FOLD,
    CAL
};
static ActiveScreen currentScreen = ActiveScreen::RUN;
static std::atomic<bool> hydraulicActuationInhibited{true};
static bool hardwareStartupReportPrinted = false;

static bool hydraulic_actuation_inhibited()
{
    return hydraulicActuationInhibited.load() || mcp23017_output_health().has_fault();
}

static void log_hardware_startup_report(bool canStarted)
{
    if (hardwareStartupReportPrinted) return;
    hardwareStartupReportPrinted = true;
    ESP_LOGI("ISO1200PT", "Hardware startup report:");
    ESP_LOGI("ISO1200PT", "MCP2515 CAN: %s; SCK=GPIO%d, MOSI=GPIO%d, MISO=GPIO%d, CS=GPIO%d, INT=GPIO%d, 250 kbit/s, 8 MHz crystal",
             canStarted ? "STARTED" : "FAILED",
             static_cast<int>(MCP2515_SPI_SCK), static_cast<int>(MCP2515_SPI_MOSI),
             static_cast<int>(MCP2515_SPI_MISO), static_cast<int>(MCP2515_SPI_CS),
             static_cast<int>(MCP2515_INT));
    ESP_LOGI("ISO1200PT", "I2C: SDA=GPIO%d, SCL=GPIO%d, 100 kHz",
             MCP23017_I2C_SDA_PIN, MCP23017_I2C_SCL_PIN);
    ESP_LOGI("ISO1200PT", "MCP23017 #1 at 0x20: %s",
             mcp23017_output_health().board_found(0) ? "FOUND" : "MISSING");
    ESP_LOGI("ISO1200PT", "MCP23017 #2 at 0x21: %s",
             mcp23017_output_health().board_found(1) ? "FOUND" : "MISSING");
    ESP_LOGI("ISO1200PT", "Relay trigger polarity: active-%s (RELAY_ACTIVE_LOW=%d)",
             RELAY_ACTIVE_LOW ? "LOW" : "HIGH", RELAY_ACTIVE_LOW);
    ESP_LOGI("ISO1200PT", "Fan/vac pins: fan PWM=GPIO%d, vacuum PWM=GPIO%d, fan RPM=GPIO%d, vacuum pressure=GPIO%d, S-bin sensor=GPIO%d",
             PIN_FAN_PWM, PIN_VAC_PWM, PIN_FAN_RPM_SENSOR, PIN_VAC_PRESSURE, PIN_SBIN_SENSOR);
}

// ─── UI STATE ────────────────────────────────────────────────────────────────────────
// VT-only state: which on-screen buttons are lit. The marker model is UI only
// (there is no marker output module); see ui_state.h.
static ui::MarkerModel markerModel;
static ui::ActionSelection actionSelection;
static ui::LockoutFeedback lockoutFeedback;
static std::mutex uiMutex;

// ─── CALIBRATION STATE ───────────────────────────────────────────────────────────────
static std::atomic<uint32_t> calPosition{50};
static calibration::Points calibrationPoints;
static nvs_handle_t calibrationNvsHandle = 0;
static bool calibrationNvsReady = false;
static std::mutex calibrationMutex;
static std::mutex calStatusMutex;
// Bulk fill headland: persisted and displayed only, NOT wired to any output.
static std::atomic<bool> bulkFillHeadland{calibration::BULK_FILL_HEADLAND_DEFAULT};
static int64_t calStatusClearAtUs = 0;

// ─── FORWARD DECLARATIONS ──────────────────────────────────────────────────────────────
void update_display_values();
void handle_softkey_event(const isobus::VirtualTerminalClient::VTKeyEvent &event);
void handle_button_event(const isobus::VirtualTerminalClient::VTKeyEvent &event);
void reset_action_buttons();
bool bulk_fill_headland_enabled();
void handle_unfold_action_button(uint8_t actionIndex);
void handle_fold_action_button(uint8_t actionIndex);

static void initialize_calibration_storage()
{
    esp_err_t result = nvs_flash_init();
    if ((ESP_ERR_NVS_NO_FREE_PAGES == result) || (ESP_ERR_NVS_NEW_VERSION_FOUND == result))
    {
        result = nvs_flash_erase();
        if (ESP_OK == result)
        {
            result = nvs_flash_init();
        }
    }
    if (ESP_OK != result)
    {
        ESP_LOGE("ISO1200PT", "Failed to initialize calibration NVS: %s", esp_err_to_name(result));
        return;
    }

    result = nvs_open("calibration", NVS_READWRITE, &calibrationNvsHandle);
    if (ESP_OK != result)
    {
        ESP_LOGE("ISO1200PT", "Failed to open calibration NVS: %s", esp_err_to_name(result));
        return;
    }

    calibrationNvsReady = true;
    calibrationPoints = calibration::load_points([](const char *key, std::uint8_t &value) {
        return ESP_OK == nvs_get_u8(calibrationNvsHandle, key, &value);
    });
    bulkFillHeadland.store(calibration::load_bulk_fill_headland([](const char *key, std::uint8_t &value) {
        return ESP_OK == nvs_get_u8(calibrationNvsHandle, key, &value);
    }));
}

static void set_calibration_status(const char *message, bool transient)
{
    std::lock_guard<std::mutex> lock(calStatusMutex);
    calStatusClearAtUs = transient ? esp_timer_get_time() + 3000000 : 0;
    if (virtualTerminalClient != nullptr)
    {
        virtualTerminalClient->send_change_string_value(VarStr_CalFaultMessage, message);
    }
}

static bool persist_calibration_value(const char *key, uint32_t value, uint32_t previousValue)
{
    if (!calibrationNvsReady)
    {
        return false;
    }

    esp_err_t result = nvs_set_u8(calibrationNvsHandle, key, static_cast<uint8_t>(value));
    if (ESP_OK != result)
    {
        ESP_LOGE("ISO1200PT", "Failed to stage calibration %s: %s", key, esp_err_to_name(result));
        return false;
    }
    result = nvs_commit(calibrationNvsHandle);
    if (ESP_OK != result)
    {
        ESP_LOGE("ISO1200PT", "Failed to save calibration %s: %s", key, esp_err_to_name(result));
        if (ESP_OK == nvs_set_u8(calibrationNvsHandle, key, static_cast<uint8_t>(previousValue)))
        {
            nvs_commit(calibrationNvsHandle);
        }
        return false;
    }
    return true;
}

static void save_calibration_point(calibration::Point point, uint16_t objectId, uint32_t newValue)
{
    calibration::SaveResult result;
    {
        std::lock_guard<std::mutex> lock(calibrationMutex);
        const std::uint8_t previousValue = calibration::value(calibrationPoints, point);
        result = calibration::save_point(
            point,
            static_cast<std::uint8_t>(newValue),
            calibrationPoints,
            [previousValue](const char *key, std::uint8_t value) {
                return persist_calibration_value(key, value, previousValue);
            }
        );
    }

    if (calibration::SaveResult::InvalidOrder == result)
    {
        const char *message = (calibration::Point::StartPlant == point)
            ? "START PLANT MUST BE >= STOP PLANT"
            : "STOP PLANT MUST BE <= START PLANT";
        set_calibration_status(message, true);
        return;
    }
    if (calibration::SaveResult::Saved != result)
    {
        set_calibration_status("CALIBRATION SAVE FAILED", true);
        return;
    }
    if (virtualTerminalUpdateHelper != nullptr)
    {
        virtualTerminalUpdateHelper->set_numeric_value(objectId, newValue);
    }
    set_calibration_status("STATUS: OK - NO ACTIVE FAULTS", false);
}

// ─── DISPLAY UPDATE FUNCTION ──────────────────────────────────────────────────────────
// Pushes current sensor values and status to InCommand display
void update_display_values()
{
    if (virtualTerminalUpdateHelper == nullptr) return;

    {
        std::lock_guard<std::mutex> lock(calStatusMutex);
        if ((calStatusClearAtUs > 0) && (esp_timer_get_time() >= calStatusClearAtUs))
        {
            calStatusClearAtUs = 0;
            if (virtualTerminalClient != nullptr)
            {
                virtualTerminalClient->send_change_string_value(
                    VarStr_CalFaultMessage,
                    "STATUS: OK - NO ACTIVE FAULTS"
                );
            }
        }
    }

    FanVacStatus fanVacStatus = fan_vac_get_status();

    // Update fan RPM actual display
    virtualTerminalUpdateHelper->set_numeric_value(
        VarNum_FanRPMActual,
        fanVacStatus.fanRPMActual
    );

    // Update fan RPM target display
    virtualTerminalUpdateHelper->set_numeric_value(
        VarNum_FanRPMTarget,
        fanVacStatus.fanRPMTarget
    );

    // Update vac pressure actual display
    virtualTerminalUpdateHelper->set_numeric_value(
        VarNum_VacActual,
        fanVacStatus.vacPressureActual
    );

    // Update vac pressure target display
    virtualTerminalUpdateHelper->set_numeric_value(
        VarNum_VacTarget,
        fanVacStatus.vacPressureTarget
    );

    // Update fold step display if sequence active
    if (fold_sequence_get_state() == SequenceState::FOLD_ACTIVE)
    {
        uint8_t step = fold_sequence_get_active_action();
        virtualTerminalUpdateHelper->set_numeric_value(
            VarNum_FoldStep,
            step
        );
        const uint32_t position = 15 + ((step * 80) / FOLD_STEPS);
        calPosition.store(position);
        virtualTerminalUpdateHelper->set_numeric_value(VarNum_CalPosition, position);
    }
    else if (fold_sequence_get_state() == SequenceState::UNFOLD_ACTIVE)
    {
        uint8_t step = fold_sequence_get_active_action();
        virtualTerminalUpdateHelper->set_numeric_value(
            VarNum_UnfoldStep,
            step
        );
        const uint32_t position = 95 - ((step * 80) / UNFOLD_STEPS);
        calPosition.store(position);
        virtualTerminalUpdateHelper->set_numeric_value(VarNum_CalPosition, position);
    }

    if (virtualTerminalClient != nullptr)
    {
        const char *stateStr = "MID RANGE";
        const uint32_t position = calPosition.load();
        uint8_t fullRaise = calibration::FULL_RAISE_DEFAULT;
        uint8_t startPlant = calibration::START_PLANT_DEFAULT;
        {
            std::lock_guard<std::mutex> lock(calibrationMutex);
            fullRaise = calibrationPoints.fullRaise;
            startPlant = calibrationPoints.startPlant;
        }
        if (position + 5 >= fullRaise)
        {
            stateStr = "TRANSPORT RANGE";
        }
        else if (position <= startPlant + 5)
        {
            stateStr = "PLANT RANGE";
        }
        virtualTerminalClient->send_change_string_value(VarStr_CalPositionState, stateStr);
    }
}

// ─── BUTTON VISUALS ────────────────────────────────────────────────────────────────────
// Every stateful button owns a dedicated label and a dedicated font object, so a
// state change is: Change Background Colour (button + label) and Change
// Attribute AID 1 (font colour). If the VT reports Change Attribute as
// unsupported, Change Font Attributes is used instead. Object pool defaults
// already show the boot state; these commands re-assert it at runtime.
struct ButtonVisual
{
    uint16_t button;
    uint16_t label;
    uint16_t font;
    isobus::VirtualTerminalClient::FontSize fontSize;
};

static constexpr uint8_t FONT_COLOUR_ATTRIBUTE_ID = 1;
static constexpr uint8_t FONT_TYPE_LATIN1 = 0;
static constexpr auto FONT_12X16 = isobus::VirtualTerminalClient::FontSize::Size12x16;
static constexpr auto FONT_24X32 = isobus::VirtualTerminalClient::FontSize::Size24x32;

static const ButtonVisual VAC_ON_VISUAL = { Button_VacPower, OutStr_BtnVacPower, FontAttr_BtnVacOn, FONT_12X16 };
static const ButtonVisual VAC_OFF_VISUAL = { Button_VacPowerOff, OutStr_BtnVacPowerOff, FontAttr_BtnVacOff, FONT_12X16 };
static const ButtonVisual FAN_ON_VISUAL = { Button_FanPower, OutStr_BtnFanPower, FontAttr_BtnFanOn, FONT_12X16 };
static const ButtonVisual FAN_OFF_VISUAL = { Button_FanPowerOff, OutStr_BtnFanPowerOff, FontAttr_BtnFanOff, FONT_12X16 };
static const ButtonVisual MARKER_LEFT_VISUAL = { Button_MarkerPrev, OutStr_BtnPrev, FontAttr_BtnMarkerLeft, FONT_24X32 };
static const ButtonVisual MARKER_AUTO_VISUAL = { Button_MarkerToggle, VarStr_MarkerStatus, FontAttr_BtnMarkerAuto, FONT_12X16 };
static const ButtonVisual MARKER_RIGHT_VISUAL = { Button_MarkerNext, OutStr_BtnNext, FontAttr_BtnMarkerRight, FONT_24X32 };
static const ButtonVisual HEADLAND_VISUAL = { Button_CalBulkHeadland, VarStr_CalBulkHeadland, FontAttr_BtnCalBulkHeadland, FONT_12X16 };

// ─── HYDRAULIC ACTION BUTTON HELPERS ────────────────────────────────────────────────
static const uint16_t UNFOLD_BTN_IDS[6] = {
    Button_UnfoldAction1, Button_UnfoldAction2, Button_UnfoldAction3,
    Button_UnfoldAction4, Button_UnfoldAction5, Button_UnfoldAction6
};
static const uint16_t UNFOLD_STR_IDS[6] = {
    VarStr_UnfoldAction1, VarStr_UnfoldAction2, VarStr_UnfoldAction3,
    VarStr_UnfoldAction4, VarStr_UnfoldAction5, VarStr_UnfoldAction6
};

static const uint16_t FOLD_BTN_IDS[6] = {
    Button_FoldAction1, Button_FoldAction2, Button_FoldAction3,
    Button_FoldAction4, Button_FoldAction5, Button_FoldAction6
};
static const uint16_t FOLD_STR_IDS[6] = {
    VarStr_FoldAction1, VarStr_FoldAction2, VarStr_FoldAction3,
    VarStr_FoldAction4, VarStr_FoldAction5, VarStr_FoldAction6
};

static ButtonVisual action_visual(ui::ActionGroup group, uint8_t actionIndex)
{
    const uint8_t idx = actionIndex - 1;
    if (ui::ActionGroup::Unfold == group)
    {
        return { UNFOLD_BTN_IDS[idx], UNFOLD_STR_IDS[idx],
                 static_cast<uint16_t>(FontAttr_BtnUnfoldAction1 + idx), FONT_12X16 };
    }
    return { FOLD_BTN_IDS[idx], FOLD_STR_IDS[idx],
             static_cast<uint16_t>(FontAttr_BtnFoldAction1 + idx), FONT_12X16 };
}

static void apply_button_style(const ButtonVisual &visual, ui::ButtonStyle style)
{
    if (virtualTerminalClient == nullptr) return;

    const ui::ButtonColours colours = ui::colours_for(style);
    virtualTerminalClient->send_change_background_colour(visual.button, colours.background);
    virtualTerminalClient->send_change_background_colour(visual.label, colours.background);
    if (!virtualTerminalClient->send_change_attribute(
            visual.font, FONT_COLOUR_ATTRIBUTE_ID, static_cast<std::uint32_t>(colours.text)))
    {
        virtualTerminalClient->send_change_font_attributes(
            visual.font, colours.text, visual.fontSize, FONT_TYPE_LATIN1, 0);
    }
}

// Sends only the action buttons whose style differs between two selections.
static void apply_action_changes(const ui::ActionSelection &before, const ui::ActionSelection &after)
{
    for (uint8_t index = 1; index <= ui::ACTION_COUNT; index++)
    {
        for (ui::ActionGroup group : { ui::ActionGroup::Unfold, ui::ActionGroup::Fold })
        {
            if (before.style(group, index) != after.style(group, index))
            {
                apply_button_style(action_visual(group, index), after.style(group, index));
            }
        }
    }
}

static void sync_action_buttons(ui::ActionGroup group)
{
    ui::ButtonStyle styles[ui::ACTION_COUNT];
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        actionSelection.styles(group, styles);
    }
    for (uint8_t index = 1; index <= ui::ACTION_COUNT; index++)
    {
        apply_button_style(action_visual(group, index), styles[index - 1]);
    }
}

static void sync_power_buttons()
{
    const bool vacOn = fan_vac_is_vac_on();
    const bool fanOn = fan_vac_is_fan_on();
    apply_button_style(VAC_ON_VISUAL, ui::power_on_style(vacOn));
    apply_button_style(VAC_OFF_VISUAL, ui::power_off_style(vacOn));
    apply_button_style(FAN_ON_VISUAL, ui::power_on_style(fanOn));
    apply_button_style(FAN_OFF_VISUAL, ui::power_off_style(fanOn));
}

static void sync_marker_buttons()
{
    ui::MarkerModel snapshot;
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        snapshot = markerModel;
    }
    apply_button_style(MARKER_LEFT_VISUAL, snapshot.left_style());
    apply_button_style(MARKER_AUTO_VISUAL, snapshot.auto_style());
    apply_button_style(MARKER_RIGHT_VISUAL, snapshot.right_style());
    if (virtualTerminalClient != nullptr)
    {
        const auto sideCommand = snapshot.side_buttons_enabled()
            ? isobus::VirtualTerminalClient::EnableDisableObjectCommand::EnableObject
            : isobus::VirtualTerminalClient::EnableDisableObjectCommand::DisableObject;
        virtualTerminalClient->send_enable_disable_object(Button_MarkerPrev, sideCommand);
        virtualTerminalClient->send_enable_disable_object(Button_MarkerNext, sideCommand);
        virtualTerminalClient->send_change_string_value(VarStr_MarkerDetail, snapshot.detail_text());
    }
}

static void sync_headland_button()
{
    const bool enabled = bulkFillHeadland.load();
    apply_button_style(HEADLAND_VISUAL, ui::headland_style(enabled));
    if (virtualTerminalClient != nullptr)
    {
        virtualTerminalClient->send_change_string_value(VarStr_CalBulkHeadland, ui::headland_text(enabled));
    }
}

static void sync_screen_buttons(ActiveScreen screen)
{
    switch (screen)
    {
        case ActiveScreen::RUN:
            sync_power_buttons();
            sync_marker_buttons();
            break;
        case ActiveScreen::UNFOLD:
            sync_action_buttons(ui::ActionGroup::Unfold);
            break;
        case ActiveScreen::FOLD:
            sync_action_buttons(ui::ActionGroup::Fold);
            break;
        case ActiveScreen::CAL:
            sync_headland_button();
            break;
    }
}

static void sync_all_buttons()
{
    sync_power_buttons();
    sync_marker_buttons();
    sync_action_buttons(ui::ActionGroup::Unfold);
    sync_action_buttons(ui::ActionGroup::Fold);
    sync_headland_button();
}

// The VT told us which mask is now active (for example after an alarm mask or a
// working set switch): re-assert the button visuals of that screen.
void handle_active_mask_event(const isobus::VirtualTerminalClient::VTChangeActiveMaskEvent &event)
{
    switch (event.maskObjectID)
    {
        case DataMask_Run:
            sync_screen_buttons(ActiveScreen::RUN);
            break;
        case DataMask_Unfold:
            sync_screen_buttons(ActiveScreen::UNFOLD);
            break;
        case DataMask_Fold:
            sync_screen_buttons(ActiveScreen::FOLD);
            break;
        case DataMask_Cal:
            sync_screen_buttons(ActiveScreen::CAL);
            break;
        default:
            break;
    }
}

// Applies a press to the selection model, then re-sends every button of that
// screen from the stored selection so a lost or reverted command is restored.
static void press_action_visual(ui::ActionGroup group, uint8_t actionIndex)
{
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        actionSelection.press(group, actionIndex);
    }
    sync_action_buttons(group);
}

static bool is_action_lit(ui::ActionGroup group, uint8_t actionIndex)
{
    std::lock_guard<std::mutex> lock(uiMutex);
    return actionSelection.is_lit(group, actionIndex);
}

static bool is_action_group_active(ui::ActionGroup group)
{
    std::lock_guard<std::mutex> lock(uiMutex);
    return (0 != actionSelection.active_index()) && (group == actionSelection.active_group());
}

void reset_action_buttons()
{
    ui::ActionSelection before;
    ui::ActionSelection after;
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        before = actionSelection;
        actionSelection.clear();
        after = actionSelection;
    }
    apply_action_changes(before, after);
}

// ─── LOCKOUT FEEDBACK ────────────────────────────────────────────────────────────────
// Display only. The hydraulicActuationInhibited interlock is not touched here.
static void send_lockout_text(ui::ActionGroup group, ui::InhibitReason reason)
{
    if (virtualTerminalClient == nullptr) return;
    const bool unfold = (ui::ActionGroup::Unfold == group);
    virtualTerminalClient->send_change_string_value(unfold ? VarStr_UnfoldInstruction : VarStr_FoldInstruction,
                                                    ui::lockout_message(reason));
    virtualTerminalClient->send_change_string_value(unfold ? VarStr_UnfoldLockReason : VarStr_FoldLockReason,
                                                    ui::lockout_reason_text(reason));
}

static void restore_lockout_text(ui::ActionGroup group)
{
    if (virtualTerminalClient == nullptr) return;
    const bool unfold = (ui::ActionGroup::Unfold == group);
    virtualTerminalClient->send_change_string_value(unfold ? VarStr_UnfoldInstruction : VarStr_FoldInstruction,
                                                    ui::LOCKOUT_RESTORE_TEXT);
    virtualTerminalClient->send_change_string_value(unfold ? VarStr_UnfoldLockReason : VarStr_FoldLockReason,
                                                    ui::lockout_reason_text(ui::InhibitReason::None));
}

// Called for an action press ignored by the inhibit. The button is neither
// selected nor actuated.
static void show_blocked_press(ui::ActionGroup group)
{
    const ui::InhibitReason reason = ui::classify_inhibit(mcp23017_output_health().has_fault());
    bool show;
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        show = lockoutFeedback.on_blocked_press(group, reason);
    }
    if (show)
    {
        send_lockout_text(group, reason);
    }
}

// Called every main-loop iteration; logs only when the reason changes.
static void update_lockout_feedback()
{
    const ui::InhibitReason reason = ui::classify_inhibit(mcp23017_output_health().has_fault());
    ui::LockoutFeedback::Update update;
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        update = lockoutFeedback.update(reason);
    }
    if (update.reasonChanged)
    {
        if (ui::InhibitReason::None == reason)
        {
            ESP_LOGI("ISO1200PT", "Hydraulic actuation inhibit ended: %s", ui::lockout_reason_log_name(reason));
        }
        else
        {
            ESP_LOGW("ISO1200PT", "Hydraulic actuation inhibited: %s", ui::lockout_reason_log_name(reason));
        }
    }
    if (update.refreshUnfold) send_lockout_text(ui::ActionGroup::Unfold, reason);
    if (update.refreshFold) send_lockout_text(ui::ActionGroup::Fold, reason);
    if (update.restoreUnfold) restore_lockout_text(ui::ActionGroup::Unfold);
    if (update.restoreFold) restore_lockout_text(ui::ActionGroup::Fold);
}

void handle_unfold_action_button(uint8_t actionIndex)
{
    if (actionIndex < 1 || actionIndex > 6) return;
    if (hydraulic_actuation_inhibited())
    {
        show_blocked_press(ui::ActionGroup::Unfold);
        return;
    }

    if (is_action_group_active(ui::ActionGroup::Fold))
    {
        reset_action_buttons();
        if (ESP_OK != fold_sequence_cancel())
        {
            show_blocked_press(ui::ActionGroup::Unfold);
            return;
        }
    }

    if (is_action_lit(ui::ActionGroup::Unfold, actionIndex))
    {
        // Safe-stop toggle: pressing active action de-energizes it
        if (ESP_OK != fold_sequence_cancel())
        {
            show_blocked_press(ui::ActionGroup::Unfold);
            return;
        }
        press_action_visual(ui::ActionGroup::Unfold, actionIndex);
        if (virtualTerminalClient != nullptr)
        {
            virtualTerminalClient->send_change_string_value(
                VarStr_UnfoldInstruction,
                "HYDRAULICS SAFE - IDLE"
            );
        }
        if (virtualTerminalUpdateHelper != nullptr)
        {
            virtualTerminalUpdateHelper->set_numeric_value(VarNum_UnfoldStep, 0);
        }
    }
    else
    {
        // Activate new action (calls fold_sequence_all_off internally to guarantee mutual exclusion)
        if (ESP_OK != fold_sequence_activate_unfold_action(actionIndex))
        {
            show_blocked_press(ui::ActionGroup::Unfold);
            return;
        }
        press_action_visual(ui::ActionGroup::Unfold, actionIndex);

        if (virtualTerminalClient != nullptr)
        {
            virtualTerminalClient->send_change_string_value(
                VarStr_UnfoldInstruction,
                fold_sequence_get_instruction()
            );
        }
        if (virtualTerminalUpdateHelper != nullptr)
        {
            virtualTerminalUpdateHelper->set_numeric_value(VarNum_UnfoldStep, actionIndex);
        }
    }
}

void handle_fold_action_button(uint8_t actionIndex)
{
    if (actionIndex < 1 || actionIndex > 6) return;
    if (hydraulic_actuation_inhibited())
    {
        show_blocked_press(ui::ActionGroup::Fold);
        return;
    }

    if (is_action_group_active(ui::ActionGroup::Unfold))
    {
        reset_action_buttons();
        if (ESP_OK != fold_sequence_cancel())
        {
            show_blocked_press(ui::ActionGroup::Fold);
            return;
        }
    }

    if (is_action_lit(ui::ActionGroup::Fold, actionIndex))
    {
        // Safe-stop toggle: pressing active action de-energizes it
        if (ESP_OK != fold_sequence_cancel())
        {
            show_blocked_press(ui::ActionGroup::Fold);
            return;
        }
        press_action_visual(ui::ActionGroup::Fold, actionIndex);
        if (virtualTerminalClient != nullptr)
        {
            virtualTerminalClient->send_change_string_value(
                VarStr_FoldInstruction,
                "HYDRAULICS SAFE - IDLE"
            );
        }
        if (virtualTerminalUpdateHelper != nullptr)
        {
            virtualTerminalUpdateHelper->set_numeric_value(VarNum_FoldStep, 0);
        }
    }
    else
    {
        // Activate new action (calls fold_sequence_all_off internally to guarantee mutual exclusion)
        if (ESP_OK != fold_sequence_activate_fold_action(actionIndex))
        {
            show_blocked_press(ui::ActionGroup::Fold);
            return;
        }
        press_action_visual(ui::ActionGroup::Fold, actionIndex);

        if (virtualTerminalClient != nullptr)
        {
            virtualTerminalClient->send_change_string_value(
                VarStr_FoldInstruction,
                fold_sequence_get_instruction()
            );
        }
        if (virtualTerminalUpdateHelper != nullptr)
        {
            virtualTerminalUpdateHelper->set_numeric_value(VarNum_FoldStep, actionIndex);
        }
    }
}

// ─── POWER / MARKER / HEADLAND HANDLERS ───────────────────────────────────────────
// Explicit ON/OFF requests go through the existing toggle functions, which are
// only called when the current state differs from the requested one.
static void request_vac_power(bool requestedOn)
{
    ui::set_power_state(requestedOn, fan_vac_is_vac_on, fan_vac_toggle_vac_power);
    sync_power_buttons();
}

static void request_fan_power(bool requestedOn)
{
    ui::set_power_state(requestedOn, fan_vac_is_fan_on, fan_vac_toggle_fan_power);
    sync_power_buttons();
}

static void handle_marker_side_button(bool leftSide)
{
    bool changed;
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        changed = leftSide ? markerModel.press_left() : markerModel.press_right();
    }
    if (changed)
    {
        sync_marker_buttons();
    }
}

static void handle_marker_auto_button()
{
    {
        std::lock_guard<std::mutex> lock(uiMutex);
        markerModel.press_auto();
    }
    sync_marker_buttons();
}

static void toggle_bulk_fill_headland()
{
    const bool previousState = bulkFillHeadland.load();
    bool newState = previousState;
    calibration::SaveResult result;
    {
        std::lock_guard<std::mutex> lock(calibrationMutex);
        result = calibration::save_bulk_fill_headland(
            !previousState,
            newState,
            [previousState](const char *key, std::uint8_t value) {
                return persist_calibration_value(key, value, previousState ? 1 : 0);
            }
        );
    }
    if (calibration::SaveResult::Saved == result)
    {
        bulkFillHeadland.store(newState);
    }
    else
    {
        set_calibration_status("CALIBRATION SAVE FAILED", true);
    }
    sync_headland_button();
}

// Getter only: the setting is not wired to any output.
bool bulk_fill_headland_enabled()
{
    return bulkFillHeadland.load();
}

// ─── SOFTKEY EVENT HANDLER ──────────────────────────────────────────────────────────
// Handles navigation soft keys on InCommand display
void handle_softkey_event(
    const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
    if (event.keyEvent !=
        isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased)
    {
        return;
    }

    switch (event.objectID)
    {
        case SoftKey_Run:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Run
            );
            currentScreen = ActiveScreen::RUN;
            sync_screen_buttons(ActiveScreen::RUN);
            break;

        case SoftKey_Unfold:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Unfold
            );
            currentScreen = ActiveScreen::UNFOLD;
            sync_screen_buttons(ActiveScreen::UNFOLD);
            break;

        case SoftKey_Fold:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Fold
            );
            currentScreen = ActiveScreen::FOLD;
            sync_screen_buttons(ActiveScreen::FOLD);
            break;

        case SoftKey_Cal:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Cal
            );
            currentScreen = ActiveScreen::CAL;
            sync_screen_buttons(ActiveScreen::CAL);
            break;

        default:
            break;
    }
}

// ─── BUTTON EVENT HANDLER ─────────────────────────────────────────────────────────────
// Handles all button presses on InCommand display
void handle_button_event(
    const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
    if (event.keyEvent !=
        isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased)
    {
        return;
    }

    switch (event.objectID)
    {
        // ── TOP TAB NAVIGATION ───────────────────────────────────────────────
        case Button_TabRun:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Run
            );
            currentScreen = ActiveScreen::RUN;
            sync_screen_buttons(ActiveScreen::RUN);
            break;

        case Button_TabUnfold:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Unfold
            );
            currentScreen = ActiveScreen::UNFOLD;
            sync_screen_buttons(ActiveScreen::UNFOLD);
            break;

        case Button_TabFold:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Fold
            );
            currentScreen = ActiveScreen::FOLD;
            sync_screen_buttons(ActiveScreen::FOLD);
            break;

        case Button_TabCal:
            fold_sequence_cancel();
            reset_action_buttons();
            virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(
                WorkingSet_1200PT,
                DataMask_Cal
            );
            currentScreen = ActiveScreen::CAL;
            sync_screen_buttons(ActiveScreen::CAL);
            break;

        // ── FOLD ACTION BUTTONS ─────────────────────────────────────────────────────
        case Button_FoldAction1:
            handle_fold_action_button(1);
            break;

        case Button_FoldAction2:
            handle_fold_action_button(2);
            break;

        case Button_FoldAction3:
            handle_fold_action_button(3);
            break;

        case Button_FoldAction4:
            handle_fold_action_button(4);
            break;

        case Button_FoldAction5:
            handle_fold_action_button(5);
            break;

        case Button_FoldAction6:
            handle_fold_action_button(6);
            break;

        // ── UNFOLD ACTION BUTTONS ───────────────────────────────────────────────────
        case Button_UnfoldAction1:
            handle_unfold_action_button(1);
            break;

        case Button_UnfoldAction2:
            handle_unfold_action_button(2);
            break;

        case Button_UnfoldAction3:
            handle_unfold_action_button(3);
            break;

        case Button_UnfoldAction4:
            handle_unfold_action_button(4);
            break;

        case Button_UnfoldAction5:
            handle_unfold_action_button(5);
            break;

        case Button_UnfoldAction6:
            handle_unfold_action_button(6);
            break;

        // ── PLANT MODE BUTTONS ─────────────────────────────────────────────────
        case Button_PlantLimitedLift:
            if (mcp23017_output_health().has_fault())
            {
                show_blocked_press(ui::ActionGroup::Fold);
            }
            else
            {
                if (ESP_OK != plant_control_set_limited_lift())
                {
                    show_blocked_press(ui::ActionGroup::Fold);
                }
            }
            break;

        case Button_PlantFullLift:
            if (mcp23017_output_health().has_fault())
            {
                show_blocked_press(ui::ActionGroup::Fold);
            }
            else
            {
                if (ESP_OK != plant_control_set_full_lift())
                {
                    show_blocked_press(ui::ActionGroup::Fold);
                }
            }
            break;

        case Button_PlantLower:
            if (mcp23017_output_health().has_fault())
            {
                show_blocked_press(ui::ActionGroup::Fold);
            }
            else
            {
                if (ESP_OK != plant_control_set_lowered())
                {
                    show_blocked_press(ui::ActionGroup::Fold);
                }
            }
            break;

        // ── BULK FILL (FAN) BUTTONS ───────────────────────────────────────────
        case Button_FanSpeedUp:
            fan_vac_fan_speed_up();
            virtualTerminalUpdateHelper->set_numeric_value(
                VarNum_FanRPMTarget,
                fan_vac_get_status().fanRPMTarget
            );
            break;

        case Button_FanSpeedDown:
            fan_vac_fan_speed_down();
            virtualTerminalUpdateHelper->set_numeric_value(
                VarNum_FanRPMTarget,
                fan_vac_get_status().fanRPMTarget
            );
            break;

        case Button_FanPower:
            request_fan_power(true);
            break;

        case Button_FanPowerOff:
            request_fan_power(false);
            break;

        // ── VACUUM BUTTONS ────────────────────────────────────────────────────
        case Button_VacPressureUp:
            fan_vac_vac_pressure_up();
            virtualTerminalUpdateHelper->set_numeric_value(
                VarNum_VacTarget,
                fan_vac_get_status().vacPressureTarget
            );
            break;

        case Button_VacPressureDown:
            fan_vac_vac_pressure_down();
            virtualTerminalUpdateHelper->set_numeric_value(
                VarNum_VacTarget,
                fan_vac_get_status().vacPressureTarget
            );
            break;

        case Button_VacPower:
            request_vac_power(true);
            break;

        case Button_VacPowerOff:
            request_vac_power(false);
            break;

        // ── MARKER CONTROL BUTTONS ────────────────────────────────────────────
        // UI state only: no marker output exists in this firmware.
        case Button_MarkerPrev:
            handle_marker_side_button(true);
            break;

        case Button_MarkerNext:
            handle_marker_side_button(false);
            break;

        case Button_MarkerToggle:
            handle_marker_auto_button();
            break;

        // ── CALIBRATION BUTTONS ───────────────────────────────────────────────
        case Button_CalSetFullRaise:
            save_calibration_point(
                calibration::Point::FullRaise,
                VarNum_CalFullRaise,
                calPosition.load()
            );
            break;

        case Button_CalSetLimitedRaise:
            save_calibration_point(
                calibration::Point::LimitedRaise,
                VarNum_CalLimitedRaise,
                calPosition.load()
            );
            break;

        case Button_CalSetStartPlant:
            save_calibration_point(
                calibration::Point::StartPlant,
                VarNum_CalStartPlant,
                calPosition.load()
            );
            break;

        case Button_CalSetStopPlant:
            save_calibration_point(
                calibration::Point::StopPlant,
                VarNum_CalStopPlant,
                calPosition.load()
            );
            break;

        case Button_CalBulkHeadland:
            toggle_bulk_fill_headland();
            break;

        default:
            break;
    }
}

// ─── MAIN ENTRY POINT ───────────────────────────────────────────────────────
extern "C" const std::uint8_t object_pool_start[] asm("_binary_object_pool_iop_start");
extern "C" const std::uint8_t object_pool_end[]   asm("_binary_object_pool_iop_end");

extern "C" void app_main()
{
    constexpr auto TAG = "ISO1200PT_MCP2515";

    initialize_calibration_storage();

    const esp_err_t outputInitResult = fold_sequence_init();
    if (ESP_OK != outputInitResult)
    {
        ESP_LOGE("ISO1200PT", "MCP23017 output setup failed; hydraulic output lockout is active.");
    }
    gpio_config_t sBinGpioConfig = {};
    sBinGpioConfig.pin_bit_mask = (1ULL << PIN_SBIN_SENSOR);
    sBinGpioConfig.mode = GPIO_MODE_INPUT;
    sBinGpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    sBinGpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    sBinGpioConfig.intr_type = GPIO_INTR_DISABLE;
    const esp_err_t sBinGpioResult = gpio_config(&sBinGpioConfig);
    if (ESP_OK != sBinGpioResult)
    {
        ESP_LOGE("ISO1200PT", "Could not configure S-bin sensor GPIO%d as an input: %s",
                 PIN_SBIN_SENSOR, esp_err_to_name(sBinGpioResult));
    }

    // ── MCP2515 SPI CAN HARDWARE SETUP ──────────────────────────────────────────
    ESP_LOGI(TAG, "Configuring MCP2515 over SPI (host=%d, SCK=%d, MOSI=%d, MISO=%d, CS=%d, INT=%d, bitrate=250000, osc=8MHz)",
              static_cast<int>(MCP2515_SPI_HOST),
              static_cast<int>(MCP2515_SPI_SCK),
              static_cast<int>(MCP2515_SPI_MOSI),
              static_cast<int>(MCP2515_SPI_MISO),
              static_cast<int>(MCP2515_SPI_CS),
              static_cast<int>(MCP2515_INT));

    spi_bus_config_t spiBusConfig = {};
    spiBusConfig.mosi_io_num = MCP2515_SPI_MOSI;
    spiBusConfig.miso_io_num = MCP2515_SPI_MISO;
    spiBusConfig.sclk_io_num = MCP2515_SPI_SCK;
    spiBusConfig.quadwp_io_num = -1;
    spiBusConfig.quadhd_io_num = -1;
    spiBusConfig.max_transfer_sz = 16;

    const auto spiBusInitResult = spi_bus_initialize(MCP2515_SPI_HOST, &spiBusConfig, SPI_DMA_CH_AUTO);
    if ((ESP_OK != spiBusInitResult) && (ESP_ERR_INVALID_STATE != spiBusInitResult))
    {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(spiBusInitResult));
        log_hardware_startup_report(false);
        return;
    }

    gpio_config_t intGpioConfig = {};
    intGpioConfig.pin_bit_mask = (1ULL << MCP2515_INT);
    intGpioConfig.mode = GPIO_MODE_INPUT;
    intGpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
    intGpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    intGpioConfig.intr_type = GPIO_INTR_DISABLE;
    const auto gpioInitResult = gpio_config(&intGpioConfig);
    if (ESP_OK != gpioInitResult)
    {
        ESP_LOGE(TAG, "Failed to configure MCP2515 INT pin: %s", esp_err_to_name(gpioInitResult));
        log_hardware_startup_report(false);
        return;
    }

    spi_device_interface_config_t spiDeviceConfig = {};
    spiDeviceConfig.command_bits = 0;
    spiDeviceConfig.address_bits = 0;
    spiDeviceConfig.dummy_bits = 0;
    spiDeviceConfig.mode = 0;
    spiDeviceConfig.duty_cycle_pos = 128;
    spiDeviceConfig.cs_ena_posttrans = 0;
    spiDeviceConfig.cs_ena_pretrans = 0;
    spiDeviceConfig.clock_speed_hz = 1 * 1000 * 1000;
    spiDeviceConfig.input_delay_ns = 0;
    spiDeviceConfig.spics_io_num = MCP2515_SPI_CS;
    spiDeviceConfig.queue_size = 1;
    spiDeviceConfig.flags = 0;
    spiDeviceConfig.pre_cb = nullptr;
    spiDeviceConfig.post_cb = nullptr;

    spiInterface = std::make_shared<isobus::SPIInterfaceESP>(&spiDeviceConfig, MCP2515_SPI_HOST);
    if (!spiInterface->init())
    {
        ESP_LOGE(TAG, "Failed to initialize SPI device for MCP2515");
        log_hardware_startup_report(false);
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    std::shared_ptr<isobus::CANHardwarePlugin> canDriver = std::make_shared<isobus::MCP2515CANInterface>(
        spiInterface.get(),
        MCP2515_250K_8MHZ_CNF1,
        MCP2515_250K_8MHZ_CNF2,
        MCP2515_250K_8MHZ_CNF3
    );

    ESP_LOGI(TAG, "Using MCP2515 bit timing CNF1=0x%02X CNF2=0x%02X CNF3=0x%02X",
              MCP2515_250K_8MHZ_CNF1, MCP2515_250K_8MHZ_CNF2, MCP2515_250K_8MHZ_CNF3);

    // ── ISOBUS STACK SETUP ────────────────────────────────────────────────────
    isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
    isobus::CANStackLogger::set_log_level(
        isobus::CANStackLogger::LoggingLevel::Info
    );
    isobus::CANHardwareInterface::set_number_of_can_channels(1);
    isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

    if (!isobus::CANHardwareInterface::start() || !canDriver->get_is_valid())
    {
        ESP_LOGE(TAG, "Failed to start CAN hardware interface, the CAN driver might be invalid");
        isobus::CANHardwareInterface::stop();
        log_hardware_startup_report(false);
        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    else
    {
        ESP_LOGI(TAG, "MCP2515 CAN interface started in normal mode");
        log_hardware_startup_report(true);
    }

    // ── ECU IDENTITY SETUP ────────────────────────────────────────────────────
    // Identifies this ECU on the ISOBUS network
    // Function code 25 = Tillage - closest match for planter frame control
    isobus::NAME ecuNAME(0);
    ecuNAME.set_arbitrary_address_capable(true);
    ecuNAME.set_industry_group(2);              // Agriculture
    ecuNAME.set_device_class(4);               // Seeding
    ecuNAME.set_function_code(25);             // Tillage/Planter Frame
    ecuNAME.set_identity_number(1200);         // 1200PT identifier
    ecuNAME.set_ecu_instance(0);
    ecuNAME.set_function_instance(0);
    ecuNAME.set_device_class_instance(0);
    ecuNAME.set_manufacturer_code(1407);       // Keep from example

    // ── VIRTUAL TERMINAL SETUP ────────────────────────────────────────────────────
    const isobus::NAMEFilter filterVT(
        isobus::NAME::NAMEParameters::FunctionCode,
        static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal)
    );
    const std::vector<isobus::NAMEFilter> vtFilters = { filterVT };

    auto internalECU = isobus::CANNetworkManager::CANNetwork
                           .create_internal_control_function(ecuNAME, 0);
    auto partnerVT   = isobus::CANNetworkManager::CANNetwork
                           .create_partnered_control_function(0, vtFilters);

    virtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(
        partnerVT,
        internalECU
    );
    const std::size_t objectPoolSize = static_cast<std::size_t>(object_pool_end - object_pool_start);
    std::string objectPoolVersion = isobus::IOPFileInterface::hash_object_pool_to_version(
        object_pool_start,
        objectPoolSize
    );
    objectPoolVersion.resize(7, '0');
    ESP_LOGI(TAG, "Embedded VT object pool: version=%s, size=%u bytes",
             objectPoolVersion.c_str(), static_cast<unsigned int>(objectPoolSize));
    virtualTerminalClient->set_object_pool(
        0,
        object_pool_start,
        static_cast<std::uint32_t>(objectPoolSize),
        objectPoolVersion
    );
    virtualTerminalClient->get_vt_soft_key_event_dispatcher()
                          .add_listener(handle_softkey_event);
    virtualTerminalClient->get_vt_button_event_dispatcher()
                          .add_listener(handle_button_event);
    virtualTerminalClient->get_vt_change_active_mask_event_dispatcher()
                          .add_listener(handle_active_mask_event);
    virtualTerminalClient->initialize(true);

    virtualTerminalUpdateHelper =
        std::make_shared<isobus::VirtualTerminalClientUpdateHelper>(
            virtualTerminalClient
        );

    // Track all numeric values we will update at runtime
    virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_FoldStep,          1);
    virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_UnfoldStep,        1);
    virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_FanRPMTarget,      FAN_RPM_DEFAULT);
    virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_FanRPMActual,      0);
    virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_VacTarget,         VAC_PRESSURE_DEFAULT);
    virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_VacActual,         0);
    virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_CalPosition,       calPosition.load());
    {
        std::lock_guard<std::mutex> lock(calibrationMutex);
        virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_CalFullRaise,      calibrationPoints.fullRaise);
        virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_CalLimitedRaise,   calibrationPoints.limitedRaise);
        virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_CalStartPlant,     calibrationPoints.startPlant);
        virtualTerminalUpdateHelper->add_tracked_numeric_value(VarNum_CalStopPlant,      calibrationPoints.stopPlant);
    }
    virtualTerminalUpdateHelper->initialize();

    // ── MODULE INITIALIZATION ─────────────────────────────────────────────────────
    plant_control_init();   // Initialize plant mode, all solenoids off
    fan_vac_init();         // Initialize PWM, RPM sensor, ADC, PID controllers
    fan_vac_start();        // Start fan and vac control loops

    // ── PERIODIC CONTROL UPDATE ─────────────────────────────────────────────────────────
    ESP_LOGI("ISO1200PT", "System initialized successfully");

    // ── MAIN LOOP ─────────────────────��────────────────────────────────────────
    // Periodic control and VT updates run here, not on the FreeRTOS timer service task.
    bool wasVtConnected = false;
    while (true)
    {
        // The VT restores object defaults whenever the pool is (re)uploaded, so
        // push every runtime button colour/state again once connected.
        const bool vtConnected = virtualTerminalClient->get_is_connected();
        if (vtConnected && !wasVtConnected)
        {
            sync_all_buttons();
        }
        wasVtConnected = vtConnected;

        fan_vac_update();
        const bool hydraulicControlAvailable = !mcp23017_output_health().has_fault();
        const bool wasActuationInhibited = hydraulicActuationInhibited.exchange(!hydraulicControlAvailable);
        if (!hydraulicControlAvailable && !wasActuationInhibited)
        {
            fold_sequence_cancel();
            reset_action_buttons();
        }
        update_lockout_feedback();
        update_display_values();

        // Read S bin sensor and update plant control
        bool sBinEmpty = gpio_get_level((gpio_num_t)PIN_SBIN_SENSOR) == 0;
        plant_control_update_sbin(sBinEmpty);

        vTaskDelay(pdMS_TO_TICKS(PID_UPDATE_INTERVAL));
    }

    // ── CLEANUP (never reached in normal operation) ───────────────────────────
    virtualTerminalClient->terminate();
    isobus::CANHardwareInterface::stop();
}
