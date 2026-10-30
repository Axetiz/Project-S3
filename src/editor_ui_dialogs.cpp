// Apsilume UI: Lay out modal scene dialogs and help, reserving space for footer actions.
// Public interface: include/editor_ui.hpp. Ownership and reading order: docs/architecture.md.
#include "editor_ui.hpp"
#include "editor_ui_detail.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace orbitsimlite {
using ui_detail::box;
using ui_detail::rectangle;

void EditorUI::overlay(sf::RenderWindow &window, const UiModel &model) {
    if (dialog == UiDialog::None)
        return;
    widgets_.clear();
    bounds_.clear();
    box(window, rectangle(0, 0, float(model.world.width), float(model.world.height)),
        sf::Color(5, 10, 18, 225));
    float desired = 580;
    if (dialog == UiDialog::Save)
        desired = 340;
    else if (dialog == UiDialog::ConfirmOverwrite || dialog == UiDialog::ConfirmDelete)
        desired = 260;
    else if (dialog == UiDialog::ConfirmDiscard)
        desired = 280;
    else if (dialog == UiDialog::Help)
        desired = 780;
    const float w = std::min(740.0f, float(model.world.width) - 40),
                h = std::min(desired, float(model.world.height) - 40);
    const float x = std::round((model.world.width - w) * 0.5f),
                y = std::round((model.world.height - h) * 0.5f);
    const float inset = 24, inner = w - 2 * inset, left = x + inset;
    box(window, rectangle(x, y, w, h), sf::Color(19, 30, 47), sf::Color(64, 104, 141));
    struct FooterButton {
        std::string id, text;
        Command command;
        bool enabled{true};
    };
    std::vector<FooterButton> footer;
    const auto cancel = [&] { footer.push_back({"dialog_cancel", "Cancel", {Action::Cancel}}); };
    const auto primary = [&](const std::string &text, Action action) {
        footer.push_back({"dialog_primary", text, {action}});
    };
    switch (dialog) {
    case UiDialog::Startup:
        footer.push_back({"startup_open", "Open saved scene", {Action::Load}});
        primary("Start scene", Action::StartScene);
        break;
    case UiDialog::Save:
        cancel();
        primary("Save", Action::SaveNamed);
        break;
    case UiDialog::Load:
        cancel();
        footer.push_back({"saved_previous",
                          "Previous",
                          {Action::SelectSaved, std::max(0, model.selected_saved - 1)},
                          model.selected_saved > 0});
        footer.push_back(
            {"saved_next",
             "Next",
             {Action::SelectSaved, std::min(int(model.saved_files.size()) - 1, model.selected_saved + 1)},
             model.selected_saved + 1 < int(model.saved_files.size())});
        footer.push_back(
            {"delete_saved", "Delete selected", {Action::DeleteSaved}, !model.saved_files.empty()});
        primary("Open", Action::LoadSaved);
        break;
    case UiDialog::ConfirmOverwrite:
        cancel();
        primary("Replace", Action::ConfirmOverwrite);
        break;
    case UiDialog::ConfirmDelete:
        cancel();
        primary("Delete", Action::ConfirmDelete);
        break;
    case UiDialog::ConfirmDiscard:
        cancel();
        footer.push_back({"discard", "Discard changes", {Action::Discard}});
        primary("Save changes", Action::Save);
        break;
    case UiDialog::Help:
        footer.push_back({"help_previous", "Previous", {Action::None, ui_detail::previous_help_page}});
        footer.push_back({"help_next", "Next", {Action::None, ui_detail::next_help_page}});
        primary("Close", Action::Cancel);
        break;
    default:
        break;
    }
    const int columns = std::max(1, std::min(int(footer.size()), int((inner + 8) / 128)));
    const int footer_rows = (int(footer.size()) + columns - 1) / columns;
    // Reserve action/status space before laying out body content; narrow dialogs add footer rows.
    const float footer_top = y + h - inset - footer_rows * 32 - (footer_rows - 1) * 8;
    const float status_top = footer_top - 42, body_bottom = status_top - 12;
    float row = y + inset;
    const auto heading = [&](const std::string &text) {
        label(window, text, rectangle(left, row, inner, 30), 23, sf::Color(230, 240, 251), true, 18);
        row += 42;
    };
    const auto line = [&](const std::string &text, int maximum_lines = 2) {
        const float available = std::max(0.0f, std::min(float(maximum_lines * 21), body_bottom - row));
        row +=
            paragraph(window, text, rectangle(left, row, inner, available), 13, sf::Color(169, 191, 218)) + 8;
    };
    if (dialog == UiDialog::Startup) {
        heading("Choose your experiment");
        line("Open a scene, explore in 3D, and edit any body directly.");
        const float count = float(std::max<std::size_t>(1, model.scenes.size()));
        const float scene_height = std::clamp((body_bottom - row - 118) / count - 8, 28.0f, 38.0f);
        for (std::size_t i = 0; i < model.scenes.size(); ++i) {
            button(window, "scene_" + std::to_string(i), model.scenes[i].name,
                   rectangle(left, row, inner, scene_height), {Action::SelectScene, int(i)}, true,
                   int(i) == model.startup_scene,
                   std::to_string(model.scenes[i].bodies.size()) + " bodies / " + model.scenes[i].units);
            row += scene_height + 8;
        }
        row += 4;
        const float half = (inner - 8) * 0.5f;
        button(window, "startup_json", "Live JSON", rectangle(left, row, half, 30), {Action::ToggleJson},
               true, model.outputs.enable_json);
        button(window, "startup_csv", "Live CSV", rectangle(left + half + 8, row, half, 30),
               {Action::ToggleCsv}, true, model.outputs.enable_csv);
        row += 42;
        line("Right drag: orbit  •  Middle drag: pan  •  Wheel: zoom");
        line("Select a body, then drag its colored handles or edit labeled values.");
    } else if (dialog == UiDialog::Save) {
        heading("Save scene");
        line("Scene name", 1);
        const auto rect = rectangle(left, row, inner, 34);
        const auto value = focus_ && !focus_->property ? focus_->value : scene_name;
        box(window, rect, sf::Color(10, 18, 31), sf::Color(90, 162, 218));
        input_value(window, value, rect, focus_ && !focus_->property, 14);
        Widget widget{"scene_name", "Scene name", "Enter a name; spaces and Unicode are supported.", rect, {},
                      {},           true};
        widgets_.push_back(widget);
        bounds_[widget.id] = rect;
        row += 50;
        line("Saved in your application data folder:", 1);
        line(model.paths.scenes.string(), 2);
    } else if (dialog == UiDialog::Load) {
        heading("Open scene");
        line("Select a saved scene, then choose Open.", 1);
        const float path_height = 40;
        const int maximum = std::max(1, int((body_bottom - row - path_height) / 35));
        const int first = std::max(0, model.selected_saved - maximum + 1);
        if (model.saved_files.empty())
            line("No saved scenes yet. Use Save in the editor first.");
        for (int i = first; i < int(model.saved_files.size()) && i < first + maximum; ++i) {
            button(window, "saved_" + std::to_string(i),
                   model.saved_files[std::size_t(i)].filename().string(), rectangle(left, row, inner, 28),
                   {Action::SelectSaved, i}, true, i == model.selected_saved);
            row += 35;
        }
        row = std::min(row + 8, body_bottom - path_height);
        line(model.paths.scenes.string(), 2);
    } else if (dialog == UiDialog::ConfirmOverwrite || dialog == UiDialog::ConfirmDelete) {
        heading(dialog == UiDialog::ConfirmOverwrite ? "Replace saved scene?" : "Delete saved scene?");
        line("This changes the selected file in your scene folder.");
    } else if (dialog == UiDialog::ConfirmDiscard) {
        heading("Unsaved scene changes");
        line("Save your changes before continuing, or discard them.");
    } else {
        heading("Camera and editing guide");
        line("Select a body, then drag its red X, green Y or blue Z handle. Labeled fields accept scientific "
             "notation.");
        line("Enter applies a field; Escape cancels. Right drag orbits, middle drag pans, and the wheel "
             "zooms.");
        const int help_columns = inner >= 560 ? 2 : 1;
        const int rows = std::max(1, int((body_bottom - row - 24) / 21));
        const int per_page = rows * help_columns;
        std::vector<const ActionInfo *> shortcuts;
        for (const auto &info : ActionRegistry::all())
            if (*info.shortcut)
                shortcuts.push_back(&info);
        const int pages = std::max(1, (int(shortcuts.size()) + per_page - 1) / per_page);
        help_page_ = std::clamp(help_page_, 0, pages - 1);
        const float cell = (inner - 20 * (help_columns - 1)) / help_columns;
        for (int i = 0; i < per_page && help_page_ * per_page + i < int(shortcuts.size()); ++i) {
            const auto &info = *shortcuts[std::size_t(help_page_ * per_page + i)];
            label(window, std::string(info.shortcut) + "  —  " + info.label,
                  rectangle(left + (i / rows) * (cell + 20), row + (i % rows) * 21, cell, 19), 11,
                  sf::Color(179, 201, 226), false);
        }
        label(window, "Page " + std::to_string(help_page_ + 1) + " / " + std::to_string(pages),
              rectangle(left, body_bottom - 20, inner, 18), 11, sf::Color(144, 164, 188));
        footer[0].enabled = help_page_ > 0;
        footer[1].enabled = help_page_ + 1 < pages;
    }
    std::string status = error_.empty() ? model.status : error_;
    if (dialog == UiDialog::Startup &&
        status == "Click a body, drag a colored handle, or edit a labeled field.")
        status.clear();
    paragraph(window, status, rectangle(left, status_top, inner, 34), 12, sf::Color(255, 193, 133));
    for (int index = 0; index < int(footer.size()); ++index) {
        const int row_index = index / columns;
        const int count = std::min(columns, int(footer.size()) - row_index * columns);
        const float width = (inner - (count - 1) * 8) / count;
        const auto &item = footer[std::size_t(index)];
        button(window, item.id, item.text,
               rectangle(left + (index % columns) * (width + 8), footer_top + row_index * 40, width, 32),
               item.command, item.enabled);
    }
}
} // namespace orbitsimlite
