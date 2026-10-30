// Apsilume UI: Own focus, text editing, validation feedback and UI-only interaction routing.
// Public interface: include/editor_ui.hpp. Ownership and reading order: docs/architecture.md.
#include "editor_ui.hpp"
#include "editor_ui_detail.hpp"
#include <SFML/Window/Clipboard.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>
namespace orbitsimlite {
using ui_detail::footer_height;
using ui_detail::number;
using ui_detail::panel_top;
namespace {
std::string property_value(Property property, const SceneDocument &document) {
    const auto *body = document.selected_body();
    switch (property) {
    case Property::Timestep:
        return number(document.simulation.get_dt(), 17);
    case Property::Substeps:
        return std::to_string(document.simulation.get_substeps());
    case Property::Gravity:
        return number(document.simulation.get_gravity(), 17);
    default:
        break;
    }
    if (!body)
        return {};
    switch (property) {
    case Property::Name:
        return body->name;
    case Property::Mass:
        return number(body->mass, 17);
    case Property::Radius:
        return number(body->radius, 17);
    case Property::PositionX:
        return number(body->pos.x, 17);
    case Property::PositionY:
        return number(body->pos.y, 17);
    case Property::PositionZ:
        return number(body->pos.z, 17);
    case Property::VelocityX:
        return number(body->vel.x, 17);
    case Property::VelocityY:
        return number(body->vel.y, 17);
    case Property::VelocityZ:
        return number(body->vel.z, 17);
    default:
        return {};
    }
}

bool body_property(Property p) {
    return p != Property::Timestep && p != Property::Substeps && p != Property::Gravity;
}

std::size_t previous_character(const std::string &text, std::size_t cursor) {
    if (!cursor)
        return 0;
    --cursor;
    while (cursor && (static_cast<unsigned char>(text[cursor]) & 0xC0) == 0x80)
        --cursor;
    return cursor;
}

std::size_t next_character(const std::string &text, std::size_t cursor) {
    if (cursor >= text.size())
        return text.size();
    ++cursor;
    while (cursor < text.size() && (static_cast<unsigned char>(text[cursor]) & 0xC0) == 0x80)
        ++cursor;
    return cursor;
}

std::string utf8(std::uint32_t codepoint) {
    const sf::String value(static_cast<char32_t>(codepoint));
    const auto data = value.toUtf8();
    return {data.begin(), data.end()};
}
} // namespace

void EditorUI::focus_property(Property property, const SceneDocument &document) {
    focus_ = Focus{property, body_property(property) ? document.selection : std::nullopt,
                   property_value(property, document), 0, true};
    focus_->caret = focus_->value.size();
    error_.clear();
}

void EditorUI::begin_save(const std::string &name) {
    scene_name = name;
    dialog = UiDialog::Save;
    focus_ = Focus{{}, {}, name, name.size(), true};
    error_.clear();
}

void EditorUI::cancel_edit() {
    focus_.reset();
    error_.clear();
}

// Keep invalid input focused with its error. Only successful controller edits leave the field.
bool EditorUI::commit(SceneDocument &document, EditorController &editor) {
    if (!focus_)
        return true;
    if (!focus_->property) {
        scene_name = focus_->value;
        focus_.reset();
        return true;
    }
    if (focus_->body && focus_->body != document.selection) {
        error_ = "Selection changed. Re-select the field to edit this body.";
        return false;
    }
    error_ = editor.set_property(document, *focus_->property, focus_->value);
    if (!error_.empty())
        return false;
    focus_.reset();
    return true;
}

// Reverse order matches visual stacking; the modal replaces the underlying hit-target list.
bool EditorUI::click(sf::Vector2f point, SceneDocument &document, EditorController &editor,
                     const std::function<void(Command)> &dispatch) {
    for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
        const auto widget = *it;
        if (!widget.bounds.contains(point) || !widget.enabled)
            continue;
        if (widget.panel && (point.y < panel_top || point.y > window_height_ - footer_height))
            continue;
        if (widget.property) {
            if (!focus_ || focus_->property != widget.property) {
                if (!commit(document, editor))
                    return true;
                focus_property(*widget.property, document);
            }
            return true;
        }
        if (widget.name_field) {
            focus_ = Focus{{}, {}, scene_name, scene_name.size(), true};
            return true;
        }
        if (!commit(document, editor))
            return true;
        if (widget.id.rfind("section_", 0) == 0) {
            sections_[std::size_t(widget.command.argument)] =
                !sections_[std::size_t(widget.command.argument)];
            return true;
        }
        if (widget.command.action == Action::None &&
            widget.command.argument == ui_detail::previous_body_page) {
            body_scroll_ = body_scroll_ >= 5 ? body_scroll_ - 5 : 0;
            return true;
        }
        if (widget.command.action == Action::None && widget.command.argument == ui_detail::next_body_page) {
            body_scroll_ += 5;
            return true;
        }
        if (widget.command.action == Action::None &&
            widget.command.argument == ui_detail::previous_help_page) {
            help_page_ = std::max(0, help_page_ - 1);
            return true;
        }
        if (widget.command.action == Action::None && widget.command.argument == ui_detail::next_help_page) {
            ++help_page_;
            return true;
        }
        dispatch(widget.command);
        return true;
    }
    if (dialog != UiDialog::None)
        return true;
    if (!commit(document, editor))
        return true;
    return false;
}

bool EditorUI::wheel(sf::Vector2f point, float delta, const Renderer3D &world) {
    if (dialog != UiDialog::None)
        return true;
    if (point.x < world.canvas_width())
        return false;
    scroll_ = std::clamp(scroll_ - delta * 54, 0.0f, maximum_scroll_);
    return true;
}

bool EditorUI::text(std::uint32_t codepoint) {
    if (!focus_)
        return false;
    if (codepoint < 32 || codepoint == 127)
        return true;
    if (focus_->selected) {
        focus_->value.clear();
        focus_->caret = 0;
        focus_->selected = false;
    }
    if (focus_->value.size() < 512) {
        const auto value = utf8(codepoint);
        focus_->value.insert(focus_->caret, value);
        focus_->caret += value.size();
    }
    return true;
}

bool EditorUI::key(sf::Keyboard::Key key, bool ctrl, bool shift, SceneDocument &document,
                   EditorController &editor, const std::function<void(Command)> &dispatch) {
    using K = sf::Keyboard::Key;
    if (key == K::Escape) {
        if (focus_)
            cancel_edit();
        else if (dialog != UiDialog::None) {
            dialog = UiDialog::None;
            dispatch({Action::Cancel});
        } else
            return false;
        return true;
    }
    if (focus_) {
        auto &edit = *focus_;
        if (ctrl && key == K::A)
            edit.selected = true;
        else if (ctrl && key == K::C)
            sf::Clipboard::setString(sf::String::fromUtf8(edit.value.begin(), edit.value.end()));
        else if (ctrl && key == K::V) {
            if (edit.selected) {
                edit.value.clear();
                edit.caret = 0;
                edit.selected = false;
            }
            const auto bytes = sf::Clipboard::getString().toUtf8();
            const std::string pasted(bytes.begin(), bytes.end());
            if (edit.value.size() + pasted.size() <= 512) {
                edit.value.insert(edit.caret, pasted);
                edit.caret += pasted.size();
            }
        }
#if SFML_VERSION_MAJOR >= 3
        else if (key == K::Backspace) {
#else
        else if (key == sf::Keyboard::BackSpace) {
#endif
            if (edit.selected) {
                edit.value.clear();
                edit.caret = 0;
            } else if (edit.caret) {
                const auto before = previous_character(edit.value, edit.caret);
                edit.value.erase(before, edit.caret - before);
                edit.caret = before;
            }
            edit.selected = false;
        } else if (key == K::Delete) {
            if (edit.selected) {
                edit.value.clear();
                edit.caret = 0;
            } else
                edit.value.erase(edit.caret, next_character(edit.value, edit.caret) - edit.caret);
            edit.selected = false;
        } else if (key == K::Left) {
            edit.caret = previous_character(edit.value, edit.caret);
            edit.selected = false;
        } else if (key == K::Right) {
            edit.caret = next_character(edit.value, edit.caret);
            edit.selected = false;
        } else if (key == K::Home) {
            edit.caret = 0;
            edit.selected = false;
        } else if (key == K::End) {
            edit.caret = edit.value.size();
            edit.selected = false;
        } else if (key == K::Enter) {
            const bool scene_field = !edit.property;
            if (commit(document, editor) && scene_field)
                dispatch({Action::SaveNamed});
        } else if (key == K::Tab) {
            const auto current = edit.property;
            if (commit(document, editor) && current) {
                int next = int(*current) + (shift ? -1 : 1);
                next = std::clamp(next, 0, int(Property::Gravity));
                focus_property(Property(next), document);
            }
        }
        return true;
    }
    if (dialog != UiDialog::None) {
        if (key == K::Enter) {
            if (dialog == UiDialog::Startup)
                dispatch({Action::StartScene});
            else if (dialog == UiDialog::Load)
                dispatch({Action::LoadSaved});
            else if (dialog == UiDialog::Save)
                dispatch({Action::SaveNamed});
        }
        return true;
    }
    if (key == K::Enter && document.selected_body()) {
        focus_property(Property::PositionX, document);
        return true;
    }
    return false;
}
} // namespace orbitsimlite
